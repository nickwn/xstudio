#include "scene.hpp"

#include <queue>

namespace xs
{

mesh::mesh(rhi::device* device, std::vector<mth::pos> vertex_buffer_, std::vector<uint16_t> index_buffer_) :
	vertex_buffer(std::move(vertex_buffer_)),
	index_buffer(std::move(index_buffer_)),
	device_vertex_buffer(),
	device_index_buffer(),
	draw_items()
{
	device_vertex_buffer = !vertex_buffer.empty() ? 
		device->create_buffer_unique(xs::rhi::buffer_type::vertex, sizeof(mth::pos) * vertex_buffer.size(), &vertex_buffer[0]) :
		nullptr;
	device_index_buffer = !index_buffer.empty() ?
		device->create_buffer_unique(xs::rhi::buffer_type::index, sizeof(uint16_t) * index_buffer.size(), &index_buffer[0]) :
		nullptr;
	draw_items = { draw_item(index_buffer.size() ? index_buffer.size() : vertex_buffer.size() , device_vertex_buffer.get(), device_index_buffer.get()) };
}

void mesh::upload(rhi::device* device)
{
	if (!vertex_buffer.empty())
	{
		device->update_buffer(device_vertex_buffer.get(), 0, sizeof(mth::pos) * vertex_buffer.size(), &vertex_buffer[0]);
	}

	if (!index_buffer.empty())
	{
		device->update_buffer(device_index_buffer.get(), 0, sizeof(uint16_t) * index_buffer.size(), &index_buffer[0]);
	}
}

void scene::evaluate(const float dt)
{
	// TODO: iterative sort for cache coherency

	std::queue<entt::entity> bfs_queue;
	for (entt::entity root : root_nodes_)
	{
		bfs_queue.push(root);
	}

	while (!bfs_queue.empty())
	{
		const entt::entity cur = bfs_queue.front();
		const eval_item& item = node_registry_.get<eval_item>(cur);
		eval_context cur_ctx = eval_context{ node_registry_, cur, item.parent, dt };
		system_registry_[item.system](cur_ctx);

		entt::entity child_itr = item.first;
		while (child_itr != entt::null)
		{
			bfs_queue.push(child_itr);
			child_itr = node_registry_.get<eval_item>(child_itr).next;
		}

		bfs_queue.pop();
	}

	// TODO: sort these too
	const auto applicators = node_registry_.view<applicator_id>();
	applicators.each([this, dt](const entt::entity node, const applicator_id& aid) {
		applicator_registry_[aid](this, node, dt);
	});
}

}