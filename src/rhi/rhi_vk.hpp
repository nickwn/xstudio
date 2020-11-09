#pragma once

#define VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL 0
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
	vk::UniqueDebugUtilsMessengerEXT debug_utils_messenger;
};

struct surface_gfx_data
{
	vk::SurfaceKHR surface;
};

} // namespace impl_
} // namespace rhi
} // namespace xs