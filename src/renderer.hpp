#pragma once

#include <vector>
#include <memory>

#include "rhi/rhi.hpp"

namespace xs
{
namespace rhi
{
	class device;
}

class scene;

class renderer
{
public:
	renderer(std::shared_ptr<rhi::device> device, std::shared_ptr<scene> scene, std::shared_ptr<rhi::surface> surface); // Setup, etc

	void render();

private:
	std::shared_ptr<rhi::device> device_;
	std::weak_ptr<scene> scene_;
	std::unique_ptr<rhi::graphics_pipeline, rhi::device::graphics_pipeline_deleter> gfx_pipeline_;
	std::unique_ptr<rhi::buffer, rhi::device::buffer_deleter> mvp_buf_;
	std::unique_ptr<rhi::uniform_set, rhi::device::uniform_set_deleter> uniform_set_;
};

}
