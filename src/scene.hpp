#pragma once

#include <memory>
#include <utility>
#include <vector>

#include <entt/entt.hpp>

#include "meth/meth.hpp"
#include "rhi/rhi.hpp"

namespace xs
{

struct draw_item
{
	draw_item(size_t elem_count_, rhi::buffer* device_vertex_buffer_, rhi::buffer* device_index_buffer_) : 
		elem_count(elem_count_), 
		vertex_offset(0), // TODO
		index_offset(0), // TODO
		device_vertex_buffer(device_vertex_buffer_), 
		device_index_buffer(device_index_buffer_) 
	{}

	size_t elem_count;
	uint32_t vertex_offset;
	uint32_t index_offset;
	rhi::buffer* device_vertex_buffer;
	rhi::buffer* device_index_buffer;
};

struct mesh
{
	mesh() = default;
	mesh(rhi::device* device, std::vector<mth::pos> vertex_buffer_, std::vector<uint16_t> index_buffer_);

	void upload(rhi::device* device);

	std::vector<mth::pos> vertex_buffer;
	std::vector<uint16_t> index_buffer;
	std::unique_ptr<rhi::buffer, rhi::device::buffer_deleter> device_vertex_buffer;
	std::unique_ptr<rhi::buffer, rhi::device::buffer_deleter> device_index_buffer;
	std::vector<draw_item> draw_items;
};

template<typename FuncType>
class function_ptr;

template<typename Ret, typename... Params>
class function_ptr<Ret(Params...)>
{
private:
	template<typename Functor, typename FuncType>
	struct caller;

	template<typename Functor, typename... Params>
	struct caller<Functor, void(Params...)>
	{
		static void call(void* obj, Params&... params) 
		{ 
			(*reinterpret_cast<Functor*>(obj))(std::forward<Params>(params)...); 
		}
	};

	template<typename Functor, typename Ret, typename... Params>
	struct caller<Functor, Ret(Params...)>
	{
		static void call(void* obj, Params&... params)
		{
			(*reinterpret_cast<Functor*>(obj))(std::forward<Params>(params)...);
		}
	};

public:

	template<typename Functor>
	function_ptr(Functor&& functor)
	{
		using decayed_type = std::decay_t<Functor>;

		ptr_ = reinterpret_cast<void*>(&functor);
		callable_ = &caller<decayed_type, Ret(Params...)>::call;
	}

	Ret operator()(Params... params) const
	{
		return callable_(ptr_, params...);
	}

private:

	void* ptr_;
	Ret(*callable_)(void*, Params&...);
};

// TODO: just use virtual functions?
template<typename FuncType>
class function_registry
{
public:
	struct func_id
	{
		uint32_t idx;

		inline bool operator==(const func_id& other) const 
		{
			return idx == other.idx;
		}
	};

	template<typename Functor>
	inline func_id register_function(Functor&& func)
	{
		const func_id new_id = func_id{ static_cast<uint32_t>(registry_.size()) };
		registry_.push_back(function_ptr<FuncType>(std::forward<Functor>(func)));
		return new_id;
	}

	inline const function_ptr<FuncType>& operator[](const func_id id) const
	{  
		return registry_[id.idx];
	}

private:
	std::vector<function_ptr<FuncType>> registry_;
};

class scene;

using system_registry = function_registry<void(entt::registry&, entt::entity, entt::entity)>;
using applicator_registry = function_registry<void(scene*, entt::entity)>;
using system_id = system_registry::func_id;
using applicator_id = applicator_registry::func_id;

// Based off https://skypjack.github.io/2019-06-25-ecs-baf-part-4/
// 4 per cache line?
struct alignas(64) eval_item
{
	entt::entity first{ entt::null };
	entt::entity next{ entt::null };
	entt::entity parent{ entt::null };
	system_id system{};
};

// TODO: add synchronization barriers
class scene
{
public:
	scene() = default;
	
	std::vector<draw_item> get_draw_list() const { return draw_list_; }

	system_registry& get_system_registry() { return system_registry_; }
	const system_registry& get_system_registry() const { return system_registry_; }
	applicator_registry& get_applicator_registry() { return applicator_registry_; }
	const applicator_registry& get_applicator_registry() const { return applicator_registry_; }

	const entt::registry& get_node_registry() const { return node_registry_; }
	entt::registry& get_node_registry() { return node_registry_; }

	void add_draw_item(const draw_item& item) { draw_list_.push_back(item); }

	template<typename T>
	inline void assign_node_data(std::vector<entt::entity> nodes, T data)
	{
		for (size_t i = 0; i < nodes.size(); i++)
		{
			node_registry_.emplace<T>(nodes[i], data);
		}
	}

	template<typename T = int, bool is_root = false, bool assure_top = true>
	std::vector<entt::entity> add_eval_nodes(const std::vector<entt::entity>& parents, const system_id system_id, std::vector<T> data = {})
	{
		std::vector<entt::entity> new_entities;
		new_entities.reserve(parents.size());
		entt::entity next = entt::null;
		for (size_t i = 0; i < parents.size(); i++)
		{
			const entt::entity parent = parents[i];
			if constexpr (assure_top && !is_root)
			{
				if (node_registry_.get<eval_item>(parent).system == system_id) [[likely]]
				{
					// Re-initialize output type?
					continue;
				}
			}

			const eval_item new_eval = {
				entt::null,
				next,
				parent,
				system_id
			};

			const entt::entity new_node = node_registry_.create();
			node_registry_.emplace<eval_item>(new_node, new_eval);
			if (data.size() > 0)
			{
				node_registry_.emplace<T>(new_node, data[i]);
			}

			if constexpr (!is_root)
			{
				node_registry_.get<eval_item>(parent).first = new_node;
			}

			new_entities.push_back(new_node);
			next = new_node;
		}

		if constexpr (is_root)
		{
			root_nodes_.insert(std::end(root_nodes_), std::begin(new_entities), std::end(new_entities));
		}

		return new_entities;
	}

	template<typename DataType = int>
	std::vector<entt::entity> add_source_nodes(mesh* mesh, const std::vector<DataType>& extra_data = {})
	{
		std::vector<entt::entity> new_entities;
		new_entities.reserve(mesh->vertex_buffer.size());
		draw_list_.insert(std::end(draw_list_), std::begin(mesh->draw_items), std::end(mesh->draw_items));
		for (size_t i = 0; i < mesh->vertex_buffer.size(); i++)
		{
			const auto new_node = node_registry_.create();
			
			if (!extra_data.empty())
			{
				node_registry_.emplace<DataType>(new_node, extra_data[i]);
			}

			new_entities.push_back(new_node);
		}

		return new_entities;
	}

	template<typename DataType>
	std::vector<entt::entity> add_source_nodes(const std::vector<DataType>& extra_data = {})
	{
		std::vector<entt::entity> new_entities;
		new_entities.reserve(extra_data.size());
		for (size_t i = 0; i < extra_data.size(); i++)
		{
			const auto new_node = node_registry_.create();
			node_registry_.emplace<DataType>(new_node, extra_data[i]);
			new_entities.push_back(new_node);
		}

		return new_entities;
	}

	void evaluate();

private:
	std::vector<draw_item> draw_list_;
	std::vector<entt::entity> root_nodes_;
	entt::registry node_registry_;
	system_registry system_registry_;
	applicator_registry applicator_registry_;
	uint8_t sort_level_;
};

}

