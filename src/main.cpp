#include <cassert>
#include <fstream>
#include <chrono>
#include <thread>
#include <numeric>

#include "script/script.hpp"

#include <Windows.h>

#include <fcntl.h>

#include "math/math.hpp"
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
	std::shared_ptr<xs::script_context> script_ctx = std::make_shared<xs::script_context>(ctx);

	std::unique_ptr<xs::renderer> renderer = std::make_unique<xs::renderer>(device, window);

	mvp_buffer mvp_buf; // TODO: init
	mvp_buf.model = (Eigen::Translation3f(0.f, 0.f, 0.f) * Eigen::AngleAxisf(0.f, Eigen::Vector3f(0.f, 0.f, 1.f))).matrix();
	//xs::mth::to_mat(xs::mth::transform(xs::mth::pos(0.f, 0.f, 0.f), xs::mth::to_quat(0.f, xs::mth::dir(0.f, 0.f, 1.f))));
	mvp_buf.view = xs::mth::look_at(Eigen::Vector3f(2.f, 0.f, 0.f), Eigen::Vector3f(0.f, 0.f, 0.f), Eigen::Vector3f(0.f, -1.f, 0.f));
	//mvp_buf.view = xs::mth::look_at(xs::mth::pos(0.f, -40.f, -60.f), xs::mth::pos(0.f, 0.f, 0.f), xs::mth::dir(0.f, 1.f, 0.f));
	mvp_buf.proj = xs::mth::perspective(xs::mth::to_rad(60.f), window->get_width(), window->get_height(), .1f, 100.f);
	auto d_mvp_buf = device->create_buffer_unique(xs::rhi::buffer_type::uniform, sizeof(mvp_buf), &mvp_buf);

	script_ctx->parse("../shaders/test.xs");

	xs::ast::types::none none_type = xs::ast::types::none();
	xs::ast::types::real f32_type = xs::ast::types::real(32);
	auto call_node = std::make_unique<xs::ast::nodes::call>(&f32_type, xs::ast::binding("bar"), 
		std::vector<std::unique_ptr<xs::ast::node>>{});
	auto root_node = std::make_unique<xs::ast::nodes::function>(&none_type, xs::ast::binding("main"), 
		std::move(call_node), std::vector<xs::ast::nodes::variable>{});
	xs::fvector<std::uint32_t> spirv = script_ctx->compile(std::move(root_node));
	script_ctx->add(std::move(spirv));

	//xs::script_instance script_inst = xs::script_instance(script_ctx, { "test" }, nullptr);

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

	//xs::draw_item cloth_draw_item = cloth->draw_item(device.get(), d_mvp_buf.get());
	//xs::draw_item rig_draw_item = skinned_mesh->draw_item(device.get(), d_mvp_buf.get());
	//xs::draw_item skel_draw_item = skel->draw_item(device.get(), d_mvp_buf.get());
	//xs::draw_item sph_draw_item = sph_sim->draw_item(device.get(), d_mvp_buf.get());

	//renderer->update_draw_list({ rig_draw_item, skel_draw_item });
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
