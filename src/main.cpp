#include <cassert>
#include <fstream>
#include <chrono>
#include <thread>
#include <numeric>

#include <Windows.h>

#include <fcntl.h>

#include "meth/math.hpp"
#include "rhi/rhi.hpp"
#include "renderer.hpp"
#include "skel.hpp"
#include "asset_loaders.hpp"
//#include "sim.hpp"

struct mvp_buffer
{
	Eigen::Matrix4f model;
	Eigen::Matrix4f view;
	Eigen::Matrix4f proj;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	LPWSTR* argv;
	int argc;

	argv = CommandLineToArgvW(GetCommandLine(), &argc);
	assert(argv != nullptr);

	std::string skel_filename = "../assets/wasp.skel";
	std::string skin_filename = "../assets/wasp.skin";
	std::string anim_filename = "../assets/wasp_walk.anim";
	if (argc > 1)
	{
		std::wstring temp_filename = std::wstring(argv[1]); // really dumb
		skel_filename = std::string(std::begin(temp_filename), std::end(temp_filename));

		temp_filename = std::wstring(argv[2]);
		skin_filename = std::string(std::begin(temp_filename), std::end(temp_filename));

		temp_filename = std::wstring(argv[3]);
		anim_filename = std::string(std::begin(temp_filename), std::end(temp_filename));
	}
	
	LocalFree(argv);

	xs::rhi::context ctx = xs::rhi::context(&hInstance); 
	std::shared_ptr<xs::rhi::surface> window = std::make_unique<xs::rhi::surface>(ctx, L"test window", 1920, 1080);
	std::shared_ptr<xs::rhi::device> device = std::make_shared<xs::rhi::device>(ctx, window.get());

	std::shared_ptr<xs::skeleton> skel = xs::loaders::load_skeleton(skel_filename);
	std::shared_ptr<xs::skinned_mesh> skinned_mesh = xs::loaders::load_skinned_mesh(skin_filename);
	std::shared_ptr<xs::rig> rig = std::make_shared<xs::rig>(skel, skinned_mesh);
	
	/*const std::size_t cloth_res = 32;
	std::vector<xs::sim::size2_t> fixed_idxs = { {0, 0}, { cloth_res - 1, 0 } };
	std::shared_ptr<xs::cloth> cloth = std::make_shared<xs::cloth>(xs::sim::size2_t{ cloth_res, cloth_res },
		xs::mth::pos(-2.f, -2.f, 0.f), xs::mth::pos(2.f, 2.f, 0.f), fixed_idxs
	);

	std::shared_ptr<xs::sph_sim> sph_sim = std::make_shared<xs::sph_sim>(
		xs::sim::range3_t{ xs::mth::pos(-.5f), xs::mth::pos(.5f) }, 800, .2f, 1000000.f, 1.f // h, p0, k
	);*/
	std::shared_ptr<xs::skeletal_anim> skel_anim = xs::loaders::load_skeletal_anim(anim_filename);
	xs::player anim_player = xs::player(skel_anim);
	anim_player.add_rig(rig);
	anim_player.play();

	std::unique_ptr<xs::renderer> renderer = std::make_unique<xs::renderer>(device, window);

	mvp_buffer mvp_buf; // TODO: init
	mvp_buf.model = (Eigen::Translation3f(0.f, 0.f, 0.f) * Eigen::AngleAxisf(0.f, Eigen::Vector3f(0.f, 0.f, 1.f))).matrix();
	//xs::mth::to_mat(xs::mth::transform(xs::mth::pos(0.f, 0.f, 0.f), xs::mth::to_quat(0.f, xs::mth::dir(0.f, 0.f, 1.f))));
	mvp_buf.view = xs::mth::look_at(Eigen::Vector3f(2.f, 0.f, 0.f), Eigen::Vector3f(0.f, 0.f, 0.f), Eigen::Vector3f(0.f, -1.f, 0.f));
	//mvp_buf.view = xs::mth::look_at(xs::mth::pos(0.f, -40.f, -60.f), xs::mth::pos(0.f, 0.f, 0.f), xs::mth::dir(0.f, 1.f, 0.f));
	mvp_buf.proj = xs::mth::perspective(xs::mth::to_rad(60.f), window->get_width(), window->get_height(), .1f, 100.f);
	auto d_mvp_buf = device->create_buffer_unique(xs::rhi::buffer_type::uniform, sizeof(mvp_buf), &mvp_buf);

	xs::renderer::stage_params skinned_vs = {
		.stage = xs::rhi::shader_stage::vertex,
		.filename = "../shaders/spirv/vert_skin.spv"
	};

	xs::renderer::stage_params skinned_fs = {
		.stage = xs::rhi::shader_stage::fragment,
		.filename = "../shaders/spirv/frag_skin.spv"
	};
	const xs::render_pass& skinned_pass = xs::render_pass_registry::get().add("skinned", 
		renderer->create_graphics_pass({ skinned_vs, skinned_fs }, xs::rhi::primitive_topology::triangles)
	);

	xs::renderer::stage_params simple_vs = {
		.stage = xs::rhi::shader_stage::vertex,
		.filename = "../shaders/spirv/vert.spv"
	};
	xs::renderer::stage_params simple_fs = {
		.stage = xs::rhi::shader_stage::fragment,
		.filename = "../shaders/spirv/frag.spv"
	};
	const xs::render_pass& simple_pass = xs::render_pass_registry::get().add("simple",
		renderer->create_graphics_pass({ simple_vs, simple_fs }, xs::rhi::primitive_topology::triangles)
	);

	xs::renderer::stage_params skel_vs = {
		.stage = xs::rhi::shader_stage::vertex,
		.filename = "../shaders/spirv/vert_skel.spv"
	};
	xs::renderer::stage_params skel_fs = {
		.stage = xs::rhi::shader_stage::fragment,
		.filename = "../shaders/spirv/frag_skel.spv"
	};
	const xs::render_pass& skel_pass = xs::render_pass_registry::get().add("lines",
		renderer->create_graphics_pass({ skel_vs, skel_fs }, xs::rhi::primitive_topology::lines)
	);

	xs::renderer::stage_params points_vs = {
		.stage = xs::rhi::shader_stage::vertex,
		.filename = "../shaders/spirv/vert_sph.spv"
	};
	xs::renderer::stage_params points_fs = {
		.stage = xs::rhi::shader_stage::fragment,
		.filename = "../shaders/spirv/frag_sph.spv"
	};
	const xs::render_pass& simple_points_pass = xs::render_pass_registry::get().add("simple_points",
		renderer->create_graphics_pass({ points_vs, points_fs }, xs::rhi::primitive_topology::points)
	);

	//xs::draw_item cloth_draw_item = cloth->draw_item(device.get(), d_mvp_buf.get());
	xs::draw_item rig_draw_item = skinned_mesh->draw_item(device.get(), d_mvp_buf.get());
	xs::draw_item skel_draw_item = skel->draw_item(device.get(), d_mvp_buf.get());
	//xs::draw_item sph_draw_item = sph_sim->draw_item(device.get(), d_mvp_buf.get());

	renderer->update_draw_list({ rig_draw_item, skel_draw_item });
	MSG msg = { };
	while (GetMessage(&msg, NULL, 0, 0))
	{
		//anim_player.update();
		//rig->evaluate();

		/*for (std::size_t i = 0; i < 1; i++) 
		{
			cloth->update(0.001f);
		}*/
		/*for (std::size_t i = 0; i < 100; i++)
		{
			sph_sim->update(0.0005f);
		}*/

		renderer->render();
		device->next_frame();

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}
