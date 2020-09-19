#pragma once

#include <vulkan/vulkan.hpp>

namespace xs
{
namespace rhi
{
namespace impl_
{

struct context_gfx_data
{
	vk::UniqueInstance instance;
};

struct surface_gfx_data
{
	vk::SurfaceKHR surface;
	size_t swapchain_image_count;
};

} // namespace impl_
} // namespace rhi
} // namespace xs