#include "renderer.hpp"

#include <fstream>
#include <array>

#include "rhi/rhi.hpp"
#include "scene.hpp"

namespace xs
{

struct mvp_buffer 
{
    mth::mat model;
	mth::mat view;
	mth::mat proj;
};

std::string read_file(std::string filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open file!");
	}

	size_t file_size = (size_t)file.tellg();
	std::string buffer = std::string(file_size, ' ');
	file.seekg(0);
	file.read(buffer.data(), file_size);
	file.close();

	return buffer;
}

renderer::renderer(std::shared_ptr<rhi::device> device, std::shared_ptr<scene> scene, std::shared_ptr<rhi::surface> surface) :
	device_(device),
	scene_(scene),
	gfx_pipeline_(),
	mvp_buf_(),
	uniform_set_()
{
	rhi::device::create_shader_params vs_create_params = {
		rhi::shader_stage::vertex,
		read_file(std::move("../shaders/spirv/vert.spv"))
	};
	std::unique_ptr<rhi::shader, rhi::device::shader_deleter> vertex_shader = device->create_shader_unique(vs_create_params);

	rhi::device::create_shader_params fs_create_params = {
		rhi::shader_stage::fragment,
		read_file(std::move("../shaders/spirv/frag.spv"))
	};
	std::unique_ptr<rhi::shader, rhi::device::shader_deleter> fragment_shader = device->create_shader_unique(fs_create_params);

	mvp_buffer mvp_buf; // TODO: init
	mvp_buf.model = mth::to_mat(mth::transform(mth::pos(0.f, 0.f, 0.f), mth::to_quat(0.f, mth::dir(0.f, 0.f, 1.f))));
	mvp_buf.view = mth::look_at(mth::pos(2.f, 2.f, 2.f), mth::pos(0.f, 0.f, 0.f), mth::dir(0.f, 0.f, 1.f));
	mvp_buf.proj = mth::perspective(mth::to_rad(45.f), surface->get_width(), surface->get_height(), .1f, 10.f);
	mvp_buf_ = device->create_buffer_unique(rhi::buffer_type::uniform, sizeof(mvp_buf), &mvp_buf);

	std::vector<rhi::device::uniform_info> vs_uniforms = {
		rhi::device::uniform_info{ { rhi::shader_stage::vertex }, rhi::uniform_type::uniform_buffer, 0, { mvp_buf_.get() } }
	};
	uniform_set_ = device->create_uniform_set_unique(vs_uniforms, vertex_shader.get());

	rhi::device::vertex_attribute pos_attrib = {
		0, 0, rhi::format::R32G32B32_sfloat, sizeof(mth::pos), xs::rhi::vertex_input_rate::vertex
	};
	rhi::device::vertex_format_id vfid = device->create_vertex_format({pos_attrib});

	std::vector<draw_item> draw_list = scene->get_draw_list();

	rhi::device::attachment_format color_output = {
		device->get_surface_format(),
		rhi::sample_counts::e1_bit,
		rhi::image_usage::color_attachment_bit
	};
	rhi::device::framebuffer_format_id fb_id = device->create_framebuffer_format({ color_output });

	rhi::device::pipeline_rasterization_state rasterization_state;
	rasterization_state.enable_depth_clamp = false;
	rasterization_state.enable_rasterizer_discard = false;
	rasterization_state.wireframe = false;
	rasterization_state.cull_mode = rhi::cull_mode::none;
	rasterization_state.front_face = rhi::front_face::clockwise;

	rhi::device::pipeline_multisample_state multisample_state;
	multisample_state.sample_count = rhi::sample_counts::e1_bit;
	multisample_state.enable_sample_shading = false;
	multisample_state.enable_alpha_to_one = false;
	multisample_state.enable_alpha_to_coverage = false;
	// TODO: rest

	rhi::device::pipeline_depth_stencil_state depth_stencil_state;
	depth_stencil_state.enable_depth_test = true;
	depth_stencil_state.enable_depth_write = true;
	depth_stencil_state.depth_compare_operator = rhi::compare_op::less;
	depth_stencil_state.enable_depth_range = false;
	depth_stencil_state.min_depth_range = 0.f;
	depth_stencil_state.max_depth_range = 1.f;
	depth_stencil_state.enable_stencil_test = false;

	rhi::device::pipeline_color_blend_state color_blend_state;
	color_blend_state.enable_logic_op = false;

	gfx_pipeline_ = device->create_graphics_pipeline_unique(std::vector<rhi::shader*>{ vertex_shader.get(), fragment_shader.get() }, 
		std::vector<rhi::uniform_set*>{uniform_set_.get()}, rhi::device::surface_ffid, vfid, rhi::primitive_topology::points, rasterization_state, multisample_state, depth_stencil_state,
		color_blend_state, rhi::dynamic_state::none);

	// If I wanted to allow a proper opengl implementation of the rhi in the future, I would do this in the render() function.
	// But an opengl implementation is unlikely so I won't
	do
	{
		static const rhi::device::color clear_color = { 0.f, 0.f, 0.f, 1.f };
		rhi::device::cmd_buf_id cmd_buf_id = device->begin_gfx_cmd_buf_for_surface(clear_color);
		device->gfx_cmd_buf_bind_pipeline(cmd_buf_id, gfx_pipeline_.get());
		for (const draw_item& item : draw_list) // TODO: indirect draw
		{
			device->gfx_cmd_buf_bind_vertex_array(cmd_buf_id, item.device_vertex_buffer);
			device->gfx_cmd_buf_bind_index_array(cmd_buf_id, item.device_index_buffer, xs::rhi::index_type::uint16);
			device->gfx_cmd_buf_bind_uniform_set(cmd_buf_id, uniform_set_.get(), gfx_pipeline_.get(), 0);
			device->gfx_cmd_buf_draw(cmd_buf_id, true, item.elem_count);
		}
		device->gfx_cmd_buf_end(cmd_buf_id);

		device->next_frame();
	} while (device->get_frame());

}

void renderer::render()
{
	device_->swap_buffers();
}

} // namespace xs
