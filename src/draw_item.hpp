#pragma once

#include <unordered_map>
#include <algorithm>
#include <functional>
#include <optional>

#include "rhi/rhi.hpp"

// small header for including draw_item when the full renderer isn't needed

namespace xs
{

struct render_pass;

struct draw_item
{
	draw_item(const render_pass* pass_) :
		device_index_buffer(nullptr),
		update([](rhi::device*) {}),
		pass(pass_)
	{}

	std::size_t elem_count;
	std::uint32_t vertex_offset;
	std::uint32_t index_offset;
	std::vector<rhi::buffer*> device_vertex_buffers;
	rhi::buffer* device_index_buffer;
	std::unordered_map<std::uint32_t, rhi::uniform_set*> uniform_sets;

	std::function<void(rhi::device*)> update;
	std::optional<std::array<std::uint32_t, 3>> dispatch;

	const render_pass* pass;
};

struct render_pass
{
	rhi::device::ptr<rhi::graphics_pipeline> pipeline;
	std::optional<rhi::framebuffer*> framebuffer;
	std::unordered_map<std::uint32_t, rhi::device::uniform_set_format_id> uniform_set_fids;
	std::unordered_map<std::uint32_t, std::unordered_map<std::string, std::uint32_t>> uniform_set_bindings;

	struct draw_item_builder_
	{
		draw_item working_item;

		draw_item_builder_& elem_count(std::size_t elem_count) { working_item.elem_count = elem_count; return *this; }
		draw_item_builder_& vertex_buffers(std::vector<rhi::buffer*> bufs) { working_item.device_vertex_buffers = std::move(bufs); return *this; }
		draw_item_builder_& index_buffer(rhi::buffer* buf) { working_item.device_index_buffer = buf; return *this; }
		draw_item_builder_& uniform_sets(std::unordered_map<std::uint32_t, rhi::uniform_set*> uniform_set) { working_item.uniform_sets = std::move(uniform_set); return *this; };
		draw_item_builder_& update(std::function<void(rhi::device*)>&& update) { working_item.update = std::move(update); return *this; }

		draw_item&& produce_draw() { return std::move(working_item); }
		draw_item&& produce_dispatch(std::uint32_t x_groups, std::uint32_t y_groups, std::uint32_t z_groups)
		{
			working_item.dispatch = { x_groups, y_groups, z_groups };
			return std::move(working_item);
		}
	};

	draw_item_builder_ draw_item_builder() const
	{
		draw_item temp_item = draw_item(this);
		return draw_item_builder_{ std::move(temp_item) };
	}

	struct uniform_set_builder_
	{
		std::uint32_t set;
		const render_pass* pass;
		rhi::device* device;
		std::vector<rhi::device::uniform_set_write> writes;

		uniform_set_builder_& uniform(std::string_view name, std::vector<rhi::device::type_variant> data) 
		{
			writes.emplace_back(pass->uniform_set_bindings.at(set).at(std::string(name)), std::move(data));
			return *this;
		}

		rhi::device::ptr<rhi::uniform_set> produce()
		{
			auto uniform_set = device->create_uniform_set_unique(pass->uniform_set_fids.at(set));
			device->update_uniform_set(uniform_set.get(), writes);
			return std::move(uniform_set);
		}
	};

	uniform_set_builder_ uniform_set_builder(rhi::device* device, const std::uint32_t set) const
	{
		uniform_set_builder_ builder = {
			.set = set,
			.pass = this,
			.device = device
		};

		return builder;
	}
};

class render_pass_registry
{
public:

	static render_pass_registry& get()
	{
		static render_pass_registry singleton;
		return singleton;
	}

	render_pass& add(std::string_view name, render_pass&& pass)
	{
		passes_[std::string(name)] = std::move(pass);
		return passes_[std::string(name)];
	}

	const render_pass& pass(std::string_view name) const
	{
		return passes_.at(std::string(name));
	}

private:
	std::unordered_map<std::string, render_pass> passes_;
};

}
