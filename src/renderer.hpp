#pragma once

#include <vector>
#include <memory>
#include <unordered_map>

#include "rhi/rhi.hpp"
#include "draw_item.hpp"

struct SpvReflectShaderModule;

namespace xs
{
namespace rhi
{
	class device;
}

class renderer
{
public:
	renderer(std::shared_ptr<rhi::device> device, std::shared_ptr<rhi::surface> surface); // Setup, etc
	void render();

	void update_draw_list(std::vector<draw_item> draw_list);

	struct stage_params
	{
		rhi::shader_stage stage;
		std::string filename;
	};

	// assumes vertex type is mth::pos!!
	render_pass create_graphics_pass(const std::vector<stage_params>& stages, rhi::primitive_topology topology, 
		std::optional<rhi::framebuffer*> framebuffer = std::optional<rhi::framebuffer*>(),
		const rhi::device::pipeline_rasterization_state& rasterization_state = rhi::device::pipeline_rasterization_state(),
		const rhi::device::pipeline_multisample_state& multisample_state = rhi::device::pipeline_multisample_state(),
		const rhi::device::pipeline_depth_stencil_state& depth_stencil_state = rhi::device::pipeline_depth_stencil_state(),
		const rhi::device::pipeline_color_blend_state& color_blend_state = rhi::device::pipeline_color_blend_state());

	render_pass create_compute_pass(const stage_params& stage);

	std::shared_ptr<rhi::device> device() const { return device_; }

private:

	void parse_shader_uniforms(const SpvReflectShaderModule& spv_module, const rhi::shader_stage stage, std::unordered_map<std::uint32_t, std::vector<rhi::device::uniform_info>>& uniform_set_infos,
		std::unordered_map<std::uint32_t, std::unordered_map<std::string, std::uint32_t>>& uniform_set_descriptors);

	std::shared_ptr<rhi::device> device_;
	std::vector<draw_item> draw_list_;
};

}
