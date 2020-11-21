#include <fstream>
#include <chrono>
#include <thread>

#define NOMINMAX
#include <Windows.h>

#include <fcntl.h>

#include "meth/meth.hpp"
#include "rhi/rhi.hpp"
#include "renderer.hpp"
#include "scene.hpp"
#include "particles.hpp"

std::vector<xs::mth::pos> vertices = {
	{-0.5f, -0.5f, 0.f},
	{0.5f, -0.5f, 0.f},
	{0.5f, 0.5f, 0.f},
	{-0.5f, 0.5f, 0.f}
};

std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0
};

xs::particle_system init_particle_system(const size_t num_particles, float spacing, xs::rhi::device* device, std::vector<size_t>& out_particle_idxs)
{
	const size_t cbrt_num_particles = std::cbrt(num_particles);
	out_particle_idxs.reserve(num_particles);

	std::vector<xs::particle> particles;
	particles.reserve(num_particles);

	std::vector<xs::mth::pos> mesh_verts;
	mesh_verts.reserve(num_particles);

	const xs::mth::pos center = xs::mth::pos(0.f, 0.f, 1.f);
	for (size_t i = 0; i < num_particles; i++)
	{
		float x_off = float(i % cbrt_num_particles) * spacing;
		float y_off = float(i / cbrt_num_particles % cbrt_num_particles) * spacing;
		float z_off = float(i / (cbrt_num_particles * cbrt_num_particles)) * spacing;
		const xs::mth::dir offset = xs::mth::pos(x_off, y_off, z_off);
		out_particle_idxs.push_back(i);
		xs::particle temp_particle = xs::particle{
			center + offset,
			xs::mth::dir::zero,
			0, // uninitialized, to be calculated
			0.f // pressure to be calculated
		};

		particles.push_back(temp_particle);
		mesh_verts.push_back(temp_particle.pos);
	}

	static constexpr float h = .5f; // TODO: set
	xs::particle_system particle_system = xs::particle_system(particles, device, spacing, 10.f, 1.f); // placeholders
	return particle_system;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	using namespace std::chrono_literals;
	//std::this_thread::sleep_for(30s);

	xs::rhi::context ctx = xs::rhi::context(&hInstance);
	std::shared_ptr<xs::rhi::surface> window = std::make_unique<xs::rhi::surface>(ctx, L"test window", 512, 512);
	std::shared_ptr<xs::rhi::device> device = std::make_shared<xs::rhi::device>(ctx, window.get());

	std::vector<size_t> particle_idxs;
	xs::particle_system particle_system = init_particle_system(125, 0.07f, device.get(), particle_idxs);

	using namespace std::placeholders;  // for _1, _2, _3...
	std::shared_ptr<xs::scene> scene = std::make_shared<xs::scene>();
	const xs::system_id eval_pressure_sid = scene->get_system_registry().register_function(
		std::bind(&xs::particle_system::evaluate_pressure, &particle_system, _1)
	);
	const xs::system_id eval_pressure_force_sid = scene->get_system_registry().register_function(
		std::bind(&xs::particle_system::evaluate_pressure_force, &particle_system, _1)
	);
	const xs::system_id eval_friction_force_sid = scene->get_system_registry().register_function(
		std::bind(&xs::particle_system::evaluate_friction_force, &particle_system, _1)
	);
	const xs::system_id eval_gravity_force_sid = scene->get_system_registry().register_function(
		std::bind(&xs::particle_system::evaluate_gravity_force, &particle_system, _1)
	);
	const xs::system_id eval_collision_force_sid = scene->get_system_registry().register_function(
		std::bind(&xs::particle_system::evaluate_collision_force, &particle_system, _1)
	);
	const xs::applicator_id particle_system_aid = scene->get_applicator_registry().register_function(
		std::bind(&xs::particle_system::apply_particle_forces, &particle_system, _1, _2, _3)
	);

	scene->add_draw_item(particle_system.get_mesh().draw_items.front());

	xs::mesh triangle = xs::mesh(device.get(), vertices, indices);

	//std::vector<entt::entity> source_tri_verts = scene->add_source_nodes(&triangle);
	std::vector<entt::entity> source_verts = scene->add_source_nodes(particle_idxs);
	std::vector<entt::entity> eval_pressure_nodes = scene->add_eval_nodes<int, true>(source_verts, eval_pressure_sid);
	std::vector<entt::entity> eval_pressure_force_nodes = scene->add_eval_nodes(eval_pressure_nodes, eval_pressure_force_sid);
	//std::vector<entt::entity> eval_friction_force_nodes = scene->add_eval_nodes(eval_pressure_force_nodes, eval_friction_force_sid);
	std::vector<entt::entity> eval_gravity_force_nodes = scene->add_eval_nodes(eval_pressure_force_nodes, eval_gravity_force_sid);
	std::vector<entt::entity> eval_collision_force_nodes = scene->add_eval_nodes(eval_gravity_force_nodes, eval_collision_force_sid);
	scene->assign_node_data(eval_collision_force_nodes, particle_system_aid);

	std::unique_ptr<xs::renderer> renderer = std::make_unique<xs::renderer>(device, scene, window);

	std::chrono::time_point last_eval = std::chrono::steady_clock::now();

	MSG msg = { };
	while (GetMessage(&msg, NULL, 0, 0))
	{
		renderer->render();
		device->next_frame();

		std::chrono::time_point cur_time = std::chrono::steady_clock::now();
		std::chrono::duration<float> dt = cur_time - last_eval;
		last_eval = cur_time;
		scene->evaluate(dt.count() * .05f);
		particle_system.get_mesh().upload(device.get());
		particle_system.get_grid().optimize();

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}
