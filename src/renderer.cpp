#include "renderer.hpp"

#include <array>
#include <cassert>

#include "spirv_reflect.h"

#include "util.hpp"
#include "math/math.hpp"
#include "rhi/rhi.hpp"

namespace xs
{

renderer::renderer(std::shared_ptr<rhi::device> device, std::shared_ptr<rhi::surface> surface) :
	device_(device),
	draw_list_()
{}

void renderer::render()
{
	if (draw_list_.empty())
	{
		return;
	}

	for (const draw_item& item : draw_list_)
	{
		item.update(device_.get());
	}

	device_->swap_buffers();
}

static void process_command(const std::vector<draw_item>& draw_list, std::size_t item_idx, rhi::device::cmd_buf_id cmd_buf_id, rhi::device* device)
{
	const std::size_t prev_item_idx = std::max<std::int64_t>(0, item_idx - 1);

	static const rhi::device::color clear_color = { 0.f, 0.f, 0.f, 1.f };
	/*static const rhi::device::bind_pipeline_params bind_pipeline_params = {
		.clear_color_values = {clear_color},
	};*/
	
	if (item_idx == 0 || draw_list[item_idx].pass->pipeline != draw_list[prev_item_idx].pass->pipeline)
	{
		if (!draw_list[item_idx].dispatch)
		{
			if (item_idx == 0 || draw_list[item_idx].pass->framebuffer != draw_list[prev_item_idx].pass->framebuffer)
			{
				rhi::device::bind_framebuffer_params params = {
					.clear_color_values = { clear_color },
					.clear_depth_stencil = true,
					.clear_depth_value = 1.f,
					.clear_stencil_value = 0
				};
				device->cmd_buf_bind_framebuffer(cmd_buf_id, draw_list[item_idx].pass->framebuffer ? *draw_list[item_idx].pass->framebuffer : device->get_surface_framebuffer(), params);
			}
			device->cmd_buf_bind_gfx_pipeline(cmd_buf_id, draw_list[item_idx].pass->pipeline.get());
		}
		else
		{
			device->cmd_buf_bind_cpu_pipeline(cmd_buf_id, draw_list[item_idx].pass->pipeline.get());
		}
	}

	if (draw_list[item_idx].device_vertex_buffers.size() > 0 && 
		(item_idx == 0 || draw_list[item_idx].device_vertex_buffers != draw_list[prev_item_idx].device_vertex_buffers))
	{
		device->cmd_buf_bind_vertex_array(cmd_buf_id, draw_list[item_idx].device_vertex_buffers);
	}
	if (draw_list[item_idx].device_index_buffer && 
		(item_idx == 0 || draw_list[item_idx].device_index_buffer != draw_list[prev_item_idx].device_index_buffer))
	{
		device->cmd_buf_bind_index_array(cmd_buf_id, draw_list[item_idx].device_index_buffer, xs::rhi::index_type::uint32);
	}

	// TODO: fix
	for (const auto& [idx, uniform_set] : draw_list[item_idx].uniform_sets)
	{
		device->cmd_buf_bind_uniform_set(cmd_buf_id, uniform_set, draw_list[item_idx].pass->pipeline.get(), idx);
	}

	if (!draw_list[item_idx].dispatch)
	{
		device->cmd_buf_draw(cmd_buf_id, draw_list[item_idx].device_index_buffer ? true : false, draw_list[item_idx].elem_count);
	}
	else
	{
		const std::array<std::uint32_t, 3>& dispatch_size = *draw_list[item_idx].dispatch;
		device->cmd_buf_dispatch(cmd_buf_id, dispatch_size[0], dispatch_size[1], dispatch_size[2]);
	}
}

void renderer::update_draw_list(std::vector<draw_item> draw_list)
{
	std::sort(std::begin(draw_list), std::end(draw_list), [](const auto& a, const auto& b) {
		return a.pass->pipeline < b.pass->pipeline || (a.pass->pipeline == b.pass->pipeline && a.device_vertex_buffers < b.device_vertex_buffers);
	});

	// TODO: might not set all cmd bufs
	do
	{
		rhi::device::cmd_buf_id cmd_buf_id = device_->begin_cmd_buf();

		// first draw command
		process_command(draw_list, 0, cmd_buf_id, device_.get());
		for (std::size_t i = 1; i < draw_list.size(); i++) // TODO: indirect draw
		{
			process_command(draw_list, i, cmd_buf_id, device_.get());
		}
		device_->cmd_buf_end(cmd_buf_id);

		device_->next_frame();
	} while (device_->get_frame());

	draw_list_ = std::move(draw_list);
}

void renderer::parse_shader_uniforms(const SpvReflectShaderModule& spv_module, const rhi::shader_stage stage, std::unordered_map<std::uint32_t, std::vector<rhi::device::uniform_info>>& uniform_set_infos, 
	std::unordered_map<std::uint32_t, std::unordered_map<std::string, std::uint32_t>>& uniform_set_descriptors)
{
	std::uint32_t binding_count;
	SpvReflectResult result = spvReflectEnumerateDescriptorBindings(&spv_module, &binding_count, nullptr);
	assert(result == SPV_REFLECT_RESULT_SUCCESS);

	if (binding_count == 0)
	{
		return;
	}

	std::vector<SpvReflectDescriptorBinding*> descriptor_bindings = std::vector<SpvReflectDescriptorBinding*>(binding_count);
	result = spvReflectEnumerateDescriptorBindings(&spv_module, &binding_count, descriptor_bindings.data());
	assert(result == SPV_REFLECT_RESULT_SUCCESS);

	for (SpvReflectDescriptorBinding*& binding : descriptor_bindings)
	{
		rhi::device::uniform_info temp_info;
		switch (binding->descriptor_type)
		{
		case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
			temp_info.type = rhi::uniform_type::sampler;
			break;
		case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			temp_info.type = rhi::uniform_type::combined_image_sampler;
			break;
		case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			temp_info.type = rhi::uniform_type::sampled_image;
			break;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
			temp_info.type = rhi::uniform_type::storage_image;
			break;
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
			temp_info.type = rhi::uniform_type::uniform_texel_buffer;
			break;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
			temp_info.type = rhi::uniform_type::storage_texel_buffer;
			break;
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			temp_info.type = rhi::uniform_type::uniform_buffer;
			break;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			temp_info.type = rhi::uniform_type::storage_buffer;
			break;
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
			assert(false && "Dynamic uniform buffer not supported.");
			continue;
		} break;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
			assert(false && "Dynamic storage buffer not supported.");
			continue;
		case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
			temp_info.type = rhi::uniform_type::input_attachment;
			break;
		}

		const std::uint32_t set = binding->set;
		uniform_set_descriptors[set][std::string(binding->name)] = binding->binding;
		temp_info.binding = binding->binding;
		temp_info.stages = std::vector<rhi::shader_stage>{ stage };
		uniform_set_infos[set].push_back(std::move(temp_info));
	}
}

render_pass renderer::create_graphics_pass(const std::vector<stage_params>& stages, rhi::primitive_topology topology, 
	std::optional<rhi::framebuffer*> framebuffer,
	const rhi::device::pipeline_rasterization_state& rasterization_state,
	const rhi::device::pipeline_multisample_state& multisample_state,
	const rhi::device::pipeline_depth_stencil_state& depth_stencil_state,
	const rhi::device::pipeline_color_blend_state& color_blend_state)
{
	std::vector<rhi::device::ptr<rhi::shader>> shaders;
	std::unordered_map<std::uint32_t, std::vector<rhi::device::uniform_info>> uniform_set_infos;
	std::unordered_map<std::uint32_t, std::unordered_map<std::string, std::uint32_t>> uniform_set_bindings;
	shaders.reserve(stages.size());
	rhi::device::vertex_format_id vfid = -1;
	for (const stage_params& params : stages)
	{
		const auto temp_shader_params = rhi::device::create_shader_params{
			params.stage,
			util::read_file(params.filename),
		};

		auto shader = device_->create_shader_unique(temp_shader_params);
		shaders.push_back(std::move(shader));

		SpvReflectShaderModule spv_module;
		SpvReflectResult result = spvReflectCreateShaderModule(temp_shader_params.bytecode.size(), temp_shader_params.bytecode.data(), &spv_module);
		assert(result == SPV_REFLECT_RESULT_SUCCESS && "failed to create shader module");
		
		// --- handle vertex inputs --- //
		if (params.stage == rhi::shader_stage::vertex)
		{
			uint32_t iv_count = 0;
			result = spvReflectEnumerateInputVariables(&spv_module, &iv_count, nullptr);
			assert(result == SPV_REFLECT_RESULT_SUCCESS);

			if (iv_count > 0)
			{
				std::vector<rhi::device::vertex_attribute> vtx_attribs;
				vtx_attribs.reserve(iv_count);

				std::vector<SpvReflectInterfaceVariable*> vtx_input = std::vector<SpvReflectInterfaceVariable*>(iv_count);
				result = spvReflectEnumerateInputVariables(&spv_module, &iv_count, vtx_input.data());
				assert(result == SPV_REFLECT_RESULT_SUCCESS);

				std::sort(std::begin(vtx_input), std::end(vtx_input), [](const auto& a, const auto& b) { return a->location < b->location; });

				for (SpvReflectInterfaceVariable*& var : vtx_input)
				{
					rhi::device::vertex_attribute tmp_vtx_attrib;
					tmp_vtx_attrib.location = var->location;
					tmp_vtx_attrib.offset = 0;
					tmp_vtx_attrib.input_rate = rhi::vertex_input_rate::vertex;

					switch (var->format)
					{
					case SPV_REFLECT_FORMAT_R32_SFLOAT: case SPV_REFLECT_FORMAT_R64_SFLOAT:
						tmp_vtx_attrib.stride = sizeof(float);
						tmp_vtx_attrib.format = rhi::format::R32_sfloat;
						break;
					case SPV_REFLECT_FORMAT_R32G32_SFLOAT: case SPV_REFLECT_FORMAT_R64G64_SFLOAT:
						tmp_vtx_attrib.stride = sizeof(float) * 2;
						tmp_vtx_attrib.format = rhi::format::R32G32_sfloat;
						break;
					case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT: case SPV_REFLECT_FORMAT_R64G64B64_SFLOAT:
						tmp_vtx_attrib.stride = sizeof(Eigen::Vector3f);
						tmp_vtx_attrib.format = rhi::format::R32G32B32_sfloat;
						break;
					case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT: case SPV_REFLECT_FORMAT_R64G64B64A64_SFLOAT:
						tmp_vtx_attrib.stride = sizeof(Eigen::Vector4f);
						tmp_vtx_attrib.format = rhi::format::R32G32B32A32_sfloat;
						break;
					case SPV_REFLECT_FORMAT_R32_UINT: case SPV_REFLECT_FORMAT_R64_UINT:
						tmp_vtx_attrib.stride = sizeof(std::uint32_t);
						tmp_vtx_attrib.format = rhi::format::R32_uint;
						break;
					case SPV_REFLECT_FORMAT_R32G32_UINT: case SPV_REFLECT_FORMAT_R64G64_UINT:
						tmp_vtx_attrib.stride = sizeof(std::uint32_t) * 2;
						tmp_vtx_attrib.format = rhi::format::R32G32_uint;
						break;
					default:
						assert(false && "vertex format not supported");
					}

					for (std::size_t i = 0; i < var->array.dims_count; i++)
					{
						tmp_vtx_attrib.stride *= var->array.dims[i];
					}

					vtx_attribs.push_back(tmp_vtx_attrib);
				}

				vfid = device_->create_vertex_format(vtx_attribs);
			}
		}

		// --- handle uniforms --- //
		parse_shader_uniforms(spv_module, params.stage, uniform_set_infos, uniform_set_bindings);
	}

	assert(vfid != -1 && "no vertex shader");

	std::vector<rhi::shader*> shader_ptrs = std::vector<rhi::shader*>(shaders.size());
	std::transform(std::begin(shaders), std::end(shaders), std::begin(shader_ptrs),
		[](const auto& a) { return a.get(); }
	);

	std::unordered_map<std::uint32_t, rhi::device::uniform_set_format_id> uniform_set_fids;
	std::vector<rhi::device::uniform_set_format_id> ufids_vec;
	ufids_vec.reserve(uniform_set_infos.size());
	for (const auto& infos : uniform_set_infos)
	{
		const rhi::device::uniform_set_format_id ufid = device_->create_uniform_set_format(infos.second);
		uniform_set_fids[infos.first] = ufid;
		ufids_vec.push_back(ufid);
	}

	const rhi::device::framebuffer_format_id ffid = device_->get_framebuffer_format(framebuffer ? *framebuffer : device_->get_surface_framebuffer());
	auto pipeline = device_->create_graphics_pipeline_unique(shader_ptrs, ufids_vec, ffid, vfid, 
		topology, rasterization_state, multisample_state, depth_stencil_state, color_blend_state, rhi::dynamic_state::none);

	render_pass simple_pass;
	simple_pass.pipeline = std::move(pipeline);
	simple_pass.uniform_set_fids = std::move(uniform_set_fids);
	simple_pass.uniform_set_bindings = std::move(uniform_set_bindings);
	simple_pass.framebuffer = framebuffer;

	return simple_pass;
}

render_pass renderer::create_compute_pass(const stage_params& stage)
{
	std::unordered_map<std::uint32_t, rhi::device::uniform_set_format_id> uniform_set_fids;
	std::unordered_map<std::uint32_t, std::unordered_map<std::string, std::uint32_t>> uniform_set_bindings;

	const auto temp_shader_params = rhi::device::create_shader_params{
		.stage = stage.stage,
		.bytecode = util::read_file(stage.filename),
	};

	auto shader = device_->create_shader_unique(temp_shader_params);

	SpvReflectShaderModule spv_module;
	SpvReflectResult result = spvReflectCreateShaderModule(temp_shader_params.bytecode.size(), temp_shader_params.bytecode.data(), &spv_module);
	assert(result == SPV_REFLECT_RESULT_SUCCESS && "failed to create shader module");

	// --- handle uniforms --- //
	std::unordered_map<std::uint32_t, std::vector<rhi::device::uniform_info>> uniform_set_infos;
	parse_shader_uniforms(spv_module, stage.stage, uniform_set_infos, uniform_set_bindings);

	assert(!uniform_set_infos.empty());

	const rhi::device::uniform_set_format_id ufid = device_->create_uniform_set_format(uniform_set_infos.begin()->second);
	uniform_set_fids[uniform_set_infos.begin()->first] = ufid;

	auto pipeline = device_->create_compute_pipeline_unique(shader.get(), std::vector<rhi::device::uniform_set_format_id>{ ufid });

	render_pass simple_pass;
	simple_pass.pipeline = std::move(pipeline);
	simple_pass.uniform_set_fids = std::move(uniform_set_fids);
	simple_pass.uniform_set_bindings = std::move(uniform_set_bindings);
	simple_pass.framebuffer = nullptr;

	return simple_pass;
}

} // namespace xs
