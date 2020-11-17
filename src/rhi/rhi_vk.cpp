#include "rhi.hpp"
#include "rhi_vk.hpp"
#include <vector>

// TODOS:
// vma, spirv reflection?
// fix framebuffers
// resources stored in pools instead of fragmented individually across memory

namespace xs
{
namespace rhi
{
namespace impl_
{
	struct framebuffer_format
	{
		uint32_t num_color_attachments;
		vk::RenderPass render_pass;
		sample_counts samples;
	};

	struct vertex_format
	{
		std::vector<vk::VertexInputBindingDescription> bindings;
		std::vector<vk::VertexInputAttributeDescription> attributes;
		vk::PipelineVertexInputStateCreateInfo input_state_create_info;
	};

	struct frame_data
	{
		vk::UniqueCommandPool cmd_pool;
		device::cmd_buf_id cmd_buf_id;
		vk::Framebuffer bound_framebuffer;
		vk::UniqueSemaphore render_finished_semaphore; // only if surface provided
		vk::UniqueSemaphore image_available_semaphore; // only if surface provided
		vk::UniqueFence fence; // only if surface provided
	};

	struct surface_frame_data
	{
		vk::UniqueImageView image_view;
		vk::UniqueFramebuffer framebuffer;
	};

	struct device_data
	{
		vk::UniqueDevice device;
		vk::PhysicalDevice physical_device;
		vk::UniqueSwapchainKHR swapchain; // only set if window is specified in ctor
		std::vector<framebuffer_format> framebuffer_formats;
		std::vector<vertex_format> vertex_formats;
		std::vector<vk::UniqueDescriptorPool> desc_pools; // TODO: pool reference count
		std::vector<vk::UniqueCommandBuffer> cmd_bufs;
		std::vector<frame_data> frames;
		std::vector<surface_frame_data> surface_frame_data; // only if surface provided
		uint32_t frame;
		uint32_t gfx_queue_fam_idx;
		uint32_t compute_queue_fam_idx;
		uint32_t present_queue_fam_idx; // only if surface provided
		format surface_format; // only if surface provided
		vk::Extent2D surface_extent; // only if surface provided
		vk::UniqueRenderPass surface_render_pass; // only if surface provided
		texture* depth_texture; // only if surface provided
	};

	// public interface objects

	struct shader
	{
		vk::PipelineShaderStageCreateInfo pipeline_stage_create_info;
	};

	struct graphics_pipeline
	{
		vk::Pipeline pipeline;
		vk::PipelineLayout layout;
	};

	struct texture
	{
		device::create_texture_params create_params; // should be on class but am lazy
		vk::Image image;
		vk::ImageView image_view;
		vk::DeviceMemory memory;
	};

	struct sampler
	{
		vk::Sampler sampler;
	};

	struct framebuffer
	{
		device::framebuffer_format_id format_id;
		std::vector<texture*> attachments;
		uint32_t width;
		uint32_t height;
	};

	struct buffer
	{
		vk::Buffer buffer;
		vk::DeviceMemory memory;
		size_t size;
	};

	struct uniform_set
	{
		vk::DescriptorSetLayout desc_set_layout;
		vk::DescriptorSet desc_set;
		vk::DescriptorPool owning_pool;
	};

	const vk::ShaderStageFlagBits shader_stage_bits[] = {
		vk::ShaderStageFlagBits::eVertex,
		vk::ShaderStageFlagBits::eFragment,
		vk::ShaderStageFlagBits::eTessellationControl,
		vk::ShaderStageFlagBits::eTessellationEvaluation,
		vk::ShaderStageFlagBits::eCompute,
	};

    const vk::Format vulkan_formats[] = {
        vk::Format::eR8Unorm,
        vk::Format::eR8Snorm,
        vk::Format::eR8Uscaled,
        vk::Format::eR8Sscaled,
        vk::Format::eR8Uint,
        vk::Format::eR8Sint,
        vk::Format::eR8Srgb,
        vk::Format::eR8G8Unorm,
        vk::Format::eR8G8Snorm,
        vk::Format::eR8G8Uscaled,
        vk::Format::eR8G8Sscaled,
        vk::Format::eR8G8Uint,
        vk::Format::eR8G8Sint,
        vk::Format::eR8G8Srgb,
        vk::Format::eR8G8B8Unorm,
        vk::Format::eR8G8B8Snorm,
        vk::Format::eR8G8B8Uscaled,
        vk::Format::eR8G8B8Sscaled,
        vk::Format::eR8G8B8Uint,
        vk::Format::eR8G8B8Sint,
        vk::Format::eR8G8B8Srgb,
        vk::Format::eB8G8R8Unorm,
        vk::Format::eB8G8R8Snorm,
        vk::Format::eB8G8R8Uscaled,
        vk::Format::eB8G8R8Sscaled,
        vk::Format::eB8G8R8Uint,
        vk::Format::eB8G8R8Sint,
        vk::Format::eB8G8R8Srgb,
        vk::Format::eR8G8B8A8Unorm,
        vk::Format::eR8G8B8A8Snorm,
        vk::Format::eR8G8B8A8Uscaled,
        vk::Format::eR8G8B8A8Sscaled,
        vk::Format::eR8G8B8A8Uint,
        vk::Format::eR8G8B8A8Sint,
        vk::Format::eR8G8B8A8Srgb,
        vk::Format::eB8G8R8A8Unorm,
        vk::Format::eB8G8R8A8Snorm,
        vk::Format::eB8G8R8A8Uscaled,
        vk::Format::eB8G8R8A8Sscaled,
        vk::Format::eB8G8R8A8Uint,
        vk::Format::eB8G8R8A8Sint,
        vk::Format::eB8G8R8A8Srgb,
        vk::Format::eR16Unorm,
        vk::Format::eR16Snorm,
        vk::Format::eR16Uscaled,
        vk::Format::eR16Sscaled,
        vk::Format::eR16Uint,
        vk::Format::eR16Sint,
        vk::Format::eR16Sfloat,
        vk::Format::eR16G16Unorm,
        vk::Format::eR16G16Snorm,
        vk::Format::eR16G16Uscaled,
        vk::Format::eR16G16Sscaled,
        vk::Format::eR16G16Uint,
        vk::Format::eR16G16Sint,
        vk::Format::eR16G16Sfloat,
        vk::Format::eR16G16B16Unorm,
        vk::Format::eR16G16B16Snorm,
        vk::Format::eR16G16B16Uscaled,
        vk::Format::eR16G16B16Sscaled,
        vk::Format::eR16G16B16Uint,
        vk::Format::eR16G16B16Sint,
        vk::Format::eR16G16B16Sfloat,
        vk::Format::eR16G16B16A16Unorm,
        vk::Format::eR16G16B16A16Snorm,
        vk::Format::eR16G16B16A16Uscaled,
        vk::Format::eR16G16B16A16Sscaled,
        vk::Format::eR16G16B16A16Uint,
        vk::Format::eR16G16B16A16Sint,
        vk::Format::eR16G16B16A16Sfloat,
        vk::Format::eR32Uint,
        vk::Format::eR32Sint,
        vk::Format::eR32Sfloat,
        vk::Format::eR32G32Uint,
        vk::Format::eR32G32Sint,
        vk::Format::eR32G32Sfloat,
        vk::Format::eR32G32B32Uint,
        vk::Format::eR32G32B32Sint,
        vk::Format::eR32G32B32Sfloat,
        vk::Format::eR32G32B32A32Uint,
        vk::Format::eR32G32B32A32Sint,
        vk::Format::eR32G32B32A32Sfloat,
        vk::Format::eR64Uint,
        vk::Format::eR64Sint,
        vk::Format::eR64Sfloat,
        vk::Format::eR64G64Uint,
        vk::Format::eR64G64Sint,
        vk::Format::eR64G64Sfloat,
        vk::Format::eR64G64B64Uint,
        vk::Format::eR64G64B64Sint,
        vk::Format::eR64G64B64Sfloat,
        vk::Format::eR64G64B64A64Uint,
        vk::Format::eR64G64B64A64Sint,
        vk::Format::eR64G64B64A64Sfloat,
		vk::Format::eD32Sfloat
    };

	const vk::CompareOp compare_operators[] = {
		vk::CompareOp::eNever,
		vk::CompareOp::eLess,
		vk::CompareOp::eEqual,
		vk::CompareOp::eLessOrEqual,
		vk::CompareOp::eGreater,
		vk::CompareOp::eNotEqual ,
		vk::CompareOp::eGreaterOrEqual,
		vk::CompareOp::eAlways
	};

	// TODO: stencil ops not in rhi.hpp
	const vk::StencilOp stencil_operations[] = {
		vk::StencilOp::eKeep,
		vk::StencilOp::eZero,
		vk::StencilOp::eReplace,
		vk::StencilOp::eIncrementAndClamp,
		vk::StencilOp::eDecrementAndClamp,
		vk::StencilOp::eInvert,
		vk::StencilOp::eIncrementAndWrap,
		vk::StencilOp::eDecrementAndWrap
	};

	const vk::SampleCountFlagBits rasterization_sample_count[] = {
		vk::SampleCountFlagBits::e1,
		vk::SampleCountFlagBits::e2,
		vk::SampleCountFlagBits::e4,
		vk::SampleCountFlagBits::e8,
		vk::SampleCountFlagBits::e16,
		vk::SampleCountFlagBits::e32,
		vk::SampleCountFlagBits::e64,
	};

	const vk::LogicOp logic_operations[] = {
		vk::LogicOp::eClear,
		vk::LogicOp::eAnd,
		vk::LogicOp::eAndReverse,
		vk::LogicOp::eCopy,
		vk::LogicOp::eAndInverted,
		vk::LogicOp::eNoOp,
		vk::LogicOp::eXor,
		vk::LogicOp::eOr,
		vk::LogicOp::eNor,
		vk::LogicOp::eEquivalent,
		vk::LogicOp::eInvert,
		vk::LogicOp::eOrReverse,
		vk::LogicOp::eCopyInverted,
		vk::LogicOp::eOrInverted,
		vk::LogicOp::eNand,
		vk::LogicOp::eSet
	};

	const vk::BlendFactor blend_factors[] = {
		vk::BlendFactor::eZero,
		vk::BlendFactor::eOne,
		vk::BlendFactor::eSrcColor,
		vk::BlendFactor::eOneMinusSrcColor,
		vk::BlendFactor::eDstColor,
		vk::BlendFactor::eOneMinusDstColor,
		vk::BlendFactor::eSrcAlpha,
		vk::BlendFactor::eOneMinusSrcAlpha,
		vk::BlendFactor::eDstAlpha,
		vk::BlendFactor::eOneMinusDstAlpha,
		vk::BlendFactor::eConstantColor,
		vk::BlendFactor::eOneMinusConstantColor,
		vk::BlendFactor::eConstantAlpha,
		vk::BlendFactor::eOneMinusConstantAlpha,
		vk::BlendFactor::eSrcAlphaSaturate,
		vk::BlendFactor::eSrc1Alpha,
		vk::BlendFactor::eOneMinusSrc1Color,
		vk::BlendFactor::eSrc1Alpha,
		vk::BlendFactor::eOneMinusSrc1Alpha
	};
	const vk::BlendOp blend_operations[] = {
		vk::BlendOp::eAdd,
		vk::BlendOp::eSubtract,
		vk::BlendOp::eReverseSubtract,
		vk::BlendOp::eMin,
		vk::BlendOp::eMax
	};

	const vk::SamplerAddressMode address_modes[] = {
		vk::SamplerAddressMode::eRepeat,
		vk::SamplerAddressMode::eMirroredRepeat,
		vk::SamplerAddressMode::eClampToEdge,
		vk::SamplerAddressMode::eClampToBorder,
		// vk::SamplerAddressMode::eMirrorClampToEdge TODO
	};

	const vk::BorderColor sampler_border_colors[] = {
		vk::BorderColor::eFloatTransparentBlack,
		vk::BorderColor::eIntTransparentBlack,
		vk::BorderColor::eFloatOpaqueBlack,
		vk::BorderColor::eIntOpaqueBlack,
		vk::BorderColor::eFloatOpaqueWhite,
		vk::BorderColor::eIntOpaqueWhite
	};

	const vk::ImageType vulkan_image_type[] = {
		vk::ImageType::e1D,
		vk::ImageType::e2D,
		vk::ImageType::e3D,
		vk::ImageType::e2D,
		vk::ImageType::e1D,
		vk::ImageType::e2D,
		vk::ImageType::e2D
	};

	static const vk::ImageViewType view_types[] = {
		vk::ImageViewType::e1D,
		vk::ImageViewType::e2D,
		vk::ImageViewType::e3D,
		vk::ImageViewType::eCube,
		vk::ImageViewType::e1DArray,
		vk::ImageViewType::e2DArray,
		vk::ImageViewType::eCubeArray,
	};

	static const vk::DescriptorType vulkan_uniform_types[] = {
		vk::DescriptorType::eSampler,
		vk::DescriptorType::eCombinedImageSampler,
		vk::DescriptorType::eSampledImage,
		vk::DescriptorType::eStorageImage,
		vk::DescriptorType::eUniformTexelBuffer,
		vk::DescriptorType::eStorageTexelBuffer,
		vk::DescriptorType::eUniformBuffer,
		vk::DescriptorType::eStorageBuffer,
		vk::DescriptorType::eUniformBufferDynamic,
		vk::DescriptorType::eStorageBufferDynamic,
		vk::DescriptorType::eInputAttachment
	};

	static const vk::PrimitiveTopology topology_list[] = {
		vk::PrimitiveTopology::ePointList,
		vk::PrimitiveTopology::eLineList,
		vk::PrimitiveTopology::eLineListWithAdjacency,
		vk::PrimitiveTopology::eLineStrip,
		vk::PrimitiveTopology::eLineStripWithAdjacency,
		vk::PrimitiveTopology::eTriangleList,
		vk::PrimitiveTopology::eTriangleListWithAdjacency,
		vk::PrimitiveTopology::eTriangleStrip,
		vk::PrimitiveTopology::eTriangleStripWithAdjacency,
		vk::PrimitiveTopology::eTriangleStrip,
		vk::PrimitiveTopology::ePatchList
	};

	size_t get_format_vertex_size(format fmt)
	{
		switch (fmt)
		{
		case format::R8_unorm:
		case format::R8_snorm:
		case format::R8_uint:
		case format::R8_sint:
		case format::R8G8_unorm:
		case format::R8G8_snorm:
		case format::R8G8_uint:
		case format::R8G8_sint:
		case format::R8G8B8_unorm:
		case format::R8G8B8_snorm:
		case format::R8G8B8_uint:
		case format::R8G8B8_sint:
		case format::B8G8R8_unorm:
		case format::B8G8R8_snorm:
		case format::B8G8R8_uint:
		case format::B8G8R8_sint:
		case format::R8G8B8A8_unorm:
		case format::R8G8B8A8_snorm:
		case format::R8G8B8A8_uint:
		case format::R8G8B8A8_sint:
		case format::B8G8R8A8_unorm:
		case format::B8G8R8A8_snorm:
		case format::B8G8R8A8_uint:
		case format::B8G8R8A8_sint:
			return 4;
		case format::R16_unorm:
		case format::R16_snorm:
		case format::R16_uint:
		case format::R16_sint:
		case format::R16_sfloat:
			return 4;
		case format::R16G16_unorm:
		case format::R16G16_snorm:
		case format::R16G16_uint:
		case format::R16G16_sint:
		case format::R16G16_sfloat:
			return 4;
		case format::R16G16B16_unorm:
		case format::R16G16B16_snorm:
		case format::R16G16B16_uint:
		case format::R16G16B16_sint:
		case format::R16G16B16_sfloat:
			return 8;
		case format::R16G16B16A16_unorm:
		case format::R16G16B16A16_snorm:
		case format::R16G16B16A16_uint:
		case format::R16G16B16A16_sint:
		case format::R16G16B16A16_sfloat:
			return 8;
		case format::R32_uint:
		case format::R32_sint:
		case format::R32_sfloat:
			return 4;
		case format::R32G32_uint:
		case format::R32G32_sint:
		case format::R32G32_sfloat:
			return 8;
		case format::R32G32B32_uint:
		case format::R32G32B32_sint:
		case format::R32G32B32_sfloat:
			return 12;
		case format::R32G32B32A32_uint:
		case format::R32G32B32A32_sint:
		case format::R32G32B32A32_sfloat:
			return 16;
		case format::R64_uint:
		case format::R64_sint:
		case format::R64_sfloat:
			return 8;
		case format::R64G64_uint:
		case format::R64G64_sint:
		case format::R64G64_sfloat:
			return 16;
		case format::R64G64B64_uint:
		case format::R64G64B64_sint:
		case format::R64G64B64_sfloat:
			return 24;
		case format::R64G64B64A64_uint:
		case format::R64G64B64A64_sint:
		case format::R64G64B64A64_sfloat:
			return 32;
		default:
			return 0;
		}
	}
	// TODO: for use later, doesn't work
	vk::SampleCountFlags vulkan_sample_counts(sample_counts scs)
	{
		vk::SampleCountFlags res;
		if (uint8_t(scs) & uint8_t(sample_counts::e1_bit))
		{
			res |= vk::SampleCountFlagBits::e1;
		}
		if (uint8_t(scs) & uint8_t(sample_counts::e2_bit))
		{
			res |= vk::SampleCountFlagBits::e2;
		}
		if (uint8_t(scs) & uint8_t(sample_counts::e4_bit))
		{
			res |= vk::SampleCountFlagBits::e4;
		}
		if (uint8_t(scs) & uint8_t(sample_counts::e8_bit))
		{
			res |= vk::SampleCountFlagBits::e8;
		}
		if (uint8_t(scs) & uint8_t(sample_counts::e16_bit))
		{
			res |= vk::SampleCountFlagBits::e16;
		}
		if (uint8_t(scs) & uint8_t(sample_counts::e32_bit))
		{
			res |= vk::SampleCountFlagBits::e32;
		}
		if (uint8_t(scs) & uint8_t(sample_counts::e64_bit))
		{
			res |= vk::SampleCountFlagBits::e64;
		}
		return res;
	}

	// TODO: get_image_format_pixel_size

	// from Overv's repo
	uint32_t find_memory_type(uint32_t type_filter, vk::MemoryPropertyFlags properties, vk::PhysicalDevice physical_device) {
		vk::PhysicalDeviceMemoryProperties mem_properties = physical_device.getMemoryProperties();
		for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
			if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

	vk::Format find_supported_format(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features, vk::PhysicalDevice physical_device)
	{
		for (vk::Format format : candidates) 
		{
			vk::FormatProperties props = physical_device.getFormatProperties(format);

			if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) 
			{
				return format;
			}
			else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) 
			{
				return format;
			}
		}

		throw std::runtime_error("failed to find supported format!");
	}

} // namespace impl_

device::device(const context& ctx, surface* surface) : 
	std::enable_shared_from_this<device>(),
	device_data_ptr_(std::make_unique<impl_::device_data>())
{
	std::vector<vk::PhysicalDevice> physical_devices = ctx.context_gfx_data_ptr_->instance->enumeratePhysicalDevices();
	for (const vk::PhysicalDevice& physical_device : physical_devices)
	{
		bool has_gfx_or_compute = false;
		std::vector<vk::QueueFamilyProperties> queue_fam_props = physical_device.getQueueFamilyProperties();
		for (uint32_t i = 0; i < queue_fam_props.size(); i++)
		{
			has_gfx_or_compute |= (queue_fam_props[i].queueFlags & vk::QueueFlagBits::eCompute || queue_fam_props[i].queueFlags & vk::QueueFlagBits::eGraphics);
			if (queue_fam_props[i].queueFlags & vk::QueueFlagBits::eCompute)
			{
				device_data_ptr_->compute_queue_fam_idx = i;
			}
			if (queue_fam_props[i].queueFlags & vk::QueueFlagBits::eGraphics)
			{
				device_data_ptr_->gfx_queue_fam_idx = i;
			}
		}

		bool good_device = has_gfx_or_compute;
		if (surface)
		{
			bool device_supports_surface = false;
			for (uint32_t queue_fam_idx = 0; queue_fam_idx < queue_fam_props.size(); queue_fam_idx++)
			{
				vk::Bool32 queue_fam_supports_surface = physical_device.getSurfaceSupportKHR(queue_fam_idx, surface->surface_gfx_data_ptr_->surface);
				if (queue_fam_supports_surface)
				{
					device_data_ptr_->present_queue_fam_idx = queue_fam_idx;
				}
				device_supports_surface |= bool(queue_fam_supports_surface);
			}

			good_device &= device_supports_surface;
		}

		if (good_device)
		{
			device_data_ptr_->physical_device = physical_device;
			break;
		}
	}

	std::vector<vk::QueueFamilyProperties> queue_fam_props = device_data_ptr_->physical_device.getQueueFamilyProperties();
	std::vector<vk::DeviceQueueCreateInfo> queue_create_infos(queue_fam_props.size());
	std::vector<std::vector<float>> queue_priorities(queue_fam_props.size());
	for (size_t i = 0; i < queue_fam_props.size(); i++)
	{
		queue_priorities[i] = std::vector<float>(queue_fam_props[i].queueCount, 1.f);
		queue_create_infos[i] = vk::DeviceQueueCreateInfo()
			.setQueueFamilyIndex(i)
			.setQueueCount(queue_fam_props[i].queueCount)
			.setPQueuePriorities(queue_priorities[i].data());
	}

	static std::vector<const char*> layers = {};
	static std::vector<const char*> extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	vk::DeviceCreateInfo device_create_info = vk::DeviceCreateInfo()
		.setQueueCreateInfoCount(queue_create_infos.size())
		.setPQueueCreateInfos(queue_create_infos.data())
		.setEnabledLayerCount(layers.size())
		.setPpEnabledLayerNames(layers.data())
		.setEnabledExtensionCount(extensions.size())
		.setPpEnabledExtensionNames(extensions.data())
		.setPEnabledFeatures(nullptr);

	device_data_ptr_->device = device_data_ptr_->physical_device.createDeviceUnique(device_create_info);

	device_data_ptr_->frame = 0;

	vk::Format surface_format;
	if (surface)
	{
		vk::SurfaceCapabilitiesKHR surface_capabilities = device_data_ptr_->physical_device.getSurfaceCapabilitiesKHR(surface->surface_gfx_data_ptr_->surface);
		std::vector<vk::PresentModeKHR> surface_present_modes = device_data_ptr_->physical_device.getSurfacePresentModesKHR(surface->surface_gfx_data_ptr_->surface);
		std::vector<vk::SurfaceFormatKHR> formats = device_data_ptr_->physical_device.getSurfaceFormatsKHR(surface->surface_gfx_data_ptr_->surface);
		
		if (formats.size() == 1 && formats[0].format == vk::Format::eUndefined) 
		{
			surface_format = vk::Format::eB8G8R8A8Unorm;
		}
		else 
		{
			surface_format = formats[0].format;
		}
		
		// kinda dirty
		uint8_t idx = 0;
		for (vk::Format fmt : impl_::vulkan_formats)
		{
			if (fmt == surface_format)
			{
				device_data_ptr_->surface_format = format(idx);
			}
		}

		vk::ColorSpaceKHR surface_color_space = formats[0].colorSpace;

		vk::Extent2D surface_extent = vk::Extent2D(surface->width_, surface->height_);
		if (surface_capabilities.currentExtent.width != -1)
		{
			surface_extent = surface_capabilities.currentExtent;
		}

		vk::PresentModeKHR present_mode = vk::PresentModeKHR::eFifo;
		for (auto& pm : surface_present_modes)
		{
			if (pm == vk::PresentModeKHR::eMailbox)
			{
				present_mode = vk::PresentModeKHR::eMailbox;
				break;
			}
		}

		uint32_t desired_num_swapchain_images = 3;
		if (desired_num_swapchain_images < surface_capabilities.minImageCount)
		{
			desired_num_swapchain_images = surface_capabilities.minImageCount;
		}
		if (surface_capabilities.maxImageCount > 0 && desired_num_swapchain_images > surface_capabilities.maxImageCount)
		{
			desired_num_swapchain_images = surface_capabilities.maxImageCount;
		}

		vk::SurfaceTransformFlagBitsKHR pre_transform;
		if (surface_capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
		{
			pre_transform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
		}
		else
		{
			pre_transform = surface_capabilities.currentTransform;
		}

		vk::CompositeAlphaFlagBitsKHR composite_alpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		vk::CompositeAlphaFlagBitsKHR composite_alpha_flags[4] = {
			vk::CompositeAlphaFlagBitsKHR::eOpaque,
			vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
			vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
			vk::CompositeAlphaFlagBitsKHR::eInherit,
		};
		for (uint32_t i = 0; i < sizeof(composite_alpha_flags); i++) {
			if (surface_capabilities.supportedCompositeAlpha & composite_alpha_flags[i]) {
				composite_alpha = composite_alpha_flags[i];
				break;
			}
		}

		std::vector<uint32_t> queue_fams = {device_data_ptr_->gfx_queue_fam_idx};
		if (device_data_ptr_->gfx_queue_fam_idx != device_data_ptr_->present_queue_fam_idx)
		{
			queue_fams.push_back(device_data_ptr_->present_queue_fam_idx);
		}

		vk::SwapchainCreateInfoKHR swapchain_create_info = vk::SwapchainCreateInfoKHR()
			.setSurface(surface->surface_gfx_data_ptr_->surface)
			.setMinImageCount(desired_num_swapchain_images)
			.setImageFormat(surface_format)
			.setImageColorSpace(surface_color_space)
			.setImageExtent(surface_extent)
			.setImageArrayLayers(1)
			.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
			.setImageSharingMode(vk::SharingMode::eExclusive)
			.setQueueFamilyIndexCount(queue_fams.size())
			.setPQueueFamilyIndices(queue_fams.data())
			.setPreTransform(pre_transform)
			.setCompositeAlpha(composite_alpha)
			.setPresentMode(present_mode)
			.setClipped(true);

		device_data_ptr_->swapchain = device_data_ptr_->device->createSwapchainKHRUnique(swapchain_create_info);
		device_data_ptr_->surface_extent = surface_extent;

		const static format depth_format = format::D32_sfloat;
		create_texture_params depth_tex_create_params = {
			depth_format,
			surface->width_,
			surface->height_,
			1,
			1,
			image_type::e2D,
			sample_counts::e1_bit,
			image_usage::depth_stencil_attachment_bit,
			1
		};
		device_data_ptr_->depth_texture = create_texture(depth_tex_create_params);

		std::vector<vk::AttachmentReference> color_references = { vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal) };
		vk::AttachmentReference depth_reference = vk::AttachmentReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
		vk::SubpassDescription subpass = vk::SubpassDescription()
			.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
			.setInputAttachmentCount(0)
			.setPInputAttachments(nullptr)
			.setColorAttachmentCount(color_references.size())
			.setPColorAttachments(color_references.data())
			.setPDepthStencilAttachment(&depth_reference)
			.setPResolveAttachments(nullptr)
			.setPreserveAttachmentCount(0);

		vk::AttachmentDescription color_attach_desc = vk::AttachmentDescription()
			.setFormat(surface_format)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStoreOp(vk::AttachmentStoreOp::eStore)
			.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
			.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

		vk::AttachmentDescription depth_attach_desc = vk::AttachmentDescription()
			.setFormat(impl_::vulkan_formats[uint8_t(depth_format)])
			.setSamples(vk::SampleCountFlagBits::e1)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
			.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

		std::vector<vk::AttachmentDescription> attachments = { color_attach_desc, depth_attach_desc	};
		vk::RenderPassCreateInfo render_pass_create_info = vk::RenderPassCreateInfo()
			.setAttachmentCount(attachments.size())
			.setPAttachments(attachments.data())
			.setSubpassCount(1)
			.setPSubpasses(&subpass)
			.setDependencyCount(0)
			.setPDependencies(nullptr);

		device_data_ptr_->surface_render_pass = device_data_ptr_->device->createRenderPassUnique(render_pass_create_info);
	}


	const size_t frame_count = surface ? device_data_ptr_->device->getSwapchainImagesKHR(device_data_ptr_->swapchain.get()).size() : 1;
	device_data_ptr_->cmd_bufs.resize(frame_count);
	device_data_ptr_->frames.resize(frame_count);
	for (size_t i = 0; i < frame_count; i++)
	{
		vk::CommandPoolCreateInfo cmd_pool_create_info = vk::CommandPoolCreateInfo()
			.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
			.setQueueFamilyIndex(device_data_ptr_->gfx_queue_fam_idx);
		device_data_ptr_->frames[i].cmd_pool = device_data_ptr_->device->createCommandPoolUnique(cmd_pool_create_info);

		vk::CommandBufferAllocateInfo cmd_buf_alloc_info = vk::CommandBufferAllocateInfo()
			.setCommandPool(device_data_ptr_->frames[i].cmd_pool.get())
			.setLevel(vk::CommandBufferLevel::ePrimary)
			.setCommandBufferCount(1);
		device_data_ptr_->cmd_bufs[i] = std::move(device_data_ptr_->device->allocateCommandBuffersUnique(cmd_buf_alloc_info)[0]);

		device_data_ptr_->frames[i].cmd_buf_id = i;
	}

	if (surface)
	{
		device_data_ptr_->surface_frame_data.resize(frame_count);
		std::vector<vk::Image> swapchain_images = device_data_ptr_->device->getSwapchainImagesKHR(device_data_ptr_->swapchain.get());
		for (size_t i = 0; i < frame_count; i++)
		{
			vk::ImageViewCreateInfo image_view_create_info = vk::ImageViewCreateInfo()
				.setImage(swapchain_images[i])
				.setViewType(vk::ImageViewType::e2D)
				.setFormat(surface_format)
				.setComponents(vk::ComponentMapping()) // TODO: no swizzling for now
				.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
			device_data_ptr_->surface_frame_data[i].image_view = device_data_ptr_->device->createImageViewUnique(image_view_create_info);

			std::vector<vk::ImageView> attachments = { device_data_ptr_->surface_frame_data[i].image_view.get(), device_data_ptr_->depth_texture->image_view };

			vk::FramebufferCreateInfo framebuffer_create_info = vk::FramebufferCreateInfo()
				.setRenderPass(device_data_ptr_->surface_render_pass.get())
				.setAttachmentCount(attachments.size())
				.setPAttachments(attachments.data())
				.setWidth(device_data_ptr_->surface_extent.width)
				.setHeight(device_data_ptr_->surface_extent.height)
				.setLayers(1);

			device_data_ptr_->surface_frame_data[i].framebuffer = device_data_ptr_->device->createFramebufferUnique(framebuffer_create_info);

			vk::SemaphoreCreateInfo semaphore_create_info = vk::SemaphoreCreateInfo();
			device_data_ptr_->frames[i].render_finished_semaphore = device_data_ptr_->device->createSemaphoreUnique(semaphore_create_info);
			device_data_ptr_->frames[i].image_available_semaphore = device_data_ptr_->device->createSemaphoreUnique(semaphore_create_info);

			vk::FenceCreateInfo fence_create_info = vk::FenceCreateInfo()
				.setFlags(vk::FenceCreateFlagBits::eSignaled);
			device_data_ptr_->frames[i].fence = device_data_ptr_->device->createFenceUnique(fence_create_info);
		}
	}
}

device::~device() 
{
	free_texture(device_data_ptr_->depth_texture);
}


format device::get_surface_format() const
{
	return device_data_ptr_->surface_format;
}

uint32_t device::get_frame() const
{
	return device_data_ptr_->frame;
}

void device::next_frame()
{
	device_data_ptr_->frame = (device_data_ptr_->frame + 1ui32) % static_cast<uint32_t>(device_data_ptr_->frames.size());
}

void device::swap_buffers()
{
	device_data_ptr_->device->waitForFences({ device_data_ptr_->frames[device_data_ptr_->frame].fence.get() }, true, std::numeric_limits<uint64_t>::max());
	device_data_ptr_->device->resetFences({ device_data_ptr_->frames[device_data_ptr_->frame].fence.get() });

	vk::ResultValue<uint32_t> swapchain_image_idx = device_data_ptr_->device->acquireNextImageKHR(device_data_ptr_->swapchain.get(), std::numeric_limits<uint64_t>::max(),
		device_data_ptr_->frames[device_data_ptr_->frame].image_available_semaphore.get(), nullptr);

	cmd_buf_id cmd_buf_id = device_data_ptr_->frames[device_data_ptr_->frame].cmd_buf_id;
	vk::PipelineStageFlags pipeline_stage_flags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	vk::SubmitInfo submit_info = vk::SubmitInfo()
		.setPWaitDstStageMask(&pipeline_stage_flags)
		.setWaitSemaphoreCount(1)
		.setPWaitSemaphores(&device_data_ptr_->frames[device_data_ptr_->frame].image_available_semaphore.get())
		.setCommandBufferCount(1)
		.setPCommandBuffers(&device_data_ptr_->cmd_bufs[cmd_buf_id].get())
		.setSignalSemaphoreCount(1)
		.setPSignalSemaphores(&device_data_ptr_->frames[device_data_ptr_->frame].render_finished_semaphore.get());
	vk::Queue gfx_submit_queue = device_data_ptr_->device->getQueue(device_data_ptr_->gfx_queue_fam_idx, 0); // TODO: do this in ctor?
	gfx_submit_queue.submit({ submit_info }, device_data_ptr_->frames[device_data_ptr_->frame].fence.get());

	// TODO: rn assuming present queue and gfx queue are same

	vk::PresentInfoKHR present_info = vk::PresentInfoKHR()
		.setWaitSemaphoreCount(1)
		.setPWaitSemaphores(&device_data_ptr_->frames[device_data_ptr_->frame].render_finished_semaphore.get())
		.setSwapchainCount(1)
		.setPSwapchains(&device_data_ptr_->swapchain.get())
		.setPImageIndices(&swapchain_image_idx.value)
		.setPResults(nullptr);

	vk::Queue present_queue = device_data_ptr_->device->getQueue(device_data_ptr_->present_queue_fam_idx, 0);
	present_queue.presentKHR(present_info);
}

shader* device::create_shader(const create_shader_params& params)
{
	vk::ShaderModuleCreateInfo shader_module_create_info = vk::ShaderModuleCreateInfo()
		.setFlags(vk::ShaderModuleCreateFlags())
		.setCodeSize(params.bytecode.size())
		.setPCode(reinterpret_cast<uint32_t*>(const_cast<char*>(params.bytecode.c_str())));
	vk::ShaderModule shader_module = device_data_ptr_->device->createShaderModule(shader_module_create_info);
	
	shader* result = new shader;
	result->pipeline_stage_create_info = vk::PipelineShaderStageCreateInfo()
		.setFlags(vk::PipelineShaderStageCreateFlags())
		.setStage(impl_::shader_stage_bits[uint8_t(params.stage)])
		.setModule(shader_module)
		.setPName("main")
		.setPSpecializationInfo(nullptr);

	return result;
}

void device::free_shader(shader* shader)
{
	device_data_ptr_->device->destroyShaderModule(shader->pipeline_stage_create_info.module);
	delete shader;
}

texture* device::create_texture(const create_texture_params& params, std::vector<std::vector<uint8_t>> data)
{
	vk::ImageFormatListCreateInfo image_format_list_create_info = vk::ImageFormatListCreateInfo();
	
	vk::ImageCreateFlags create_flags = vk::ImageCreateFlags();
	if (params.type == image_type::cube || params.type == image_type::cube_array) {
		create_flags |= vk::ImageCreateFlagBits::eCubeCompatible;
	}

	vk::ImageUsageFlags image_usage = vk::ImageUsageFlags();
	if (uint8_t(params.usage) & uint8_t(image_usage::transfer_src_bit))
	{
		image_usage |= vk::ImageUsageFlagBits::eTransferSrc;
	}
	if (uint8_t(params.usage) & uint8_t(image_usage::transfer_dst_bit))
	{
		image_usage |= vk::ImageUsageFlagBits::eTransferDst;
	}
	if (uint8_t(params.usage) & uint8_t(image_usage::sampled_bit))
	{
		image_usage |= vk::ImageUsageFlagBits::eSampled;
	}
	if (uint8_t(params.usage) & uint8_t(image_usage::storage_bit))
	{
		image_usage |= vk::ImageUsageFlagBits::eStorage;
	}
	if (uint8_t(params.usage) & uint8_t(image_usage::transfer_src_bit))
	{
		image_usage |= vk::ImageUsageFlagBits::eTransferSrc;
	}
	if (uint8_t(params.usage) & uint8_t(image_usage::color_attachment_bit))
	{
		image_usage |= vk::ImageUsageFlagBits::eColorAttachment;
	}
	if (uint8_t(params.usage) & uint8_t(image_usage::depth_stencil_attachment_bit))
	{
		image_usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
	}
	if (uint8_t(params.usage) & uint8_t(image_usage::transient_attachment_bit))
	{
		image_usage |= vk::ImageUsageFlagBits::eTransientAttachment;
	}
	if (uint8_t(params.usage) & uint8_t(image_usage::input_attachment_bit))
	{
		image_usage= vk::ImageUsageFlagBits::eInputAttachment;
	}

	vk::ImageType image_type = impl_::vulkan_image_type[uint8_t(params.type)];
	vk::Extent3D image_extent = vk::Extent3D(
		params.width,
		(image_type == vk::ImageType::e3D || image_type == vk::ImageType::e2D) ? params.height : 1,
		(image_type == vk::ImageType::e3D) ? params.depth : 1
	);

	vk::ImageCreateInfo image_create_info = vk::ImageCreateInfo()
		.setFlags(create_flags)
		.setImageType(image_type)
		.setFormat(impl_::vulkan_formats[uint8_t(params.format)])
		.setExtent(image_extent)
		.setMipLevels(params.mipmaps)
		.setArrayLayers((params.type == image_type::e1D_array || params.type == image_type::e2D_array ||
			params.type == image_type::cube_array || params.type == image_type::cube) ? params.array_layers : 1)
		.setSamples(impl_::rasterization_sample_count[uint8_t(params.samples)])
		.setTiling(vk::ImageTiling::eOptimal)
		.setUsage(image_usage)
		.setSharingMode(vk::SharingMode::eExclusive)
		.setQueueFamilyIndexCount(0)
		.setPQueueFamilyIndices(nullptr)
		.setInitialLayout(vk::ImageLayout::eUndefined);

	texture* result = new texture;

	result->create_params = params;
	result->image = device_data_ptr_->device->createImage(image_create_info);

	vk::MemoryRequirements mem_requirements = device_data_ptr_->device->getImageMemoryRequirements(result->image);

	vk::MemoryAllocateInfo mem_alloc_info = vk::MemoryAllocateInfo()
		.setAllocationSize(mem_requirements.size)
		.setMemoryTypeIndex(impl_::find_memory_type(mem_requirements.memoryTypeBits, vk::MemoryPropertyFlags(), device_data_ptr_->physical_device));
	
	// TODO: use vma
	result->memory = device_data_ptr_->device->allocateMemory(mem_alloc_info);
	device_data_ptr_->device->bindImageMemory(result->image, result->memory, 0);

	vk::ImageSubresourceRange subresource_range = vk::ImageSubresourceRange(
		(uint8_t(params.usage) & uint8_t(image_usage::depth_stencil_attachment_bit)) ? 
		vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor, 0, params.mipmaps, 0, params.array_layers);

	vk::ImageViewCreateInfo image_view_create_info = vk::ImageViewCreateInfo()
		.setFlags(vk::ImageViewCreateFlags())
		.setImage(result->image)
		.setViewType(impl_::view_types[uint8_t(params.type)])
		.setFormat(image_create_info.format)
		.setComponents(vk::ComponentMapping()) // TODO: no swizzling for now
		.setSubresourceRange(subresource_range);

	result->image_view = device_data_ptr_->device->createImageView(image_view_create_info);

	if (data.size() > 0) {
		for (uint32_t i = 0; i < image_create_info.arrayLayers; i++) {
			// TODO: texture_update(result, i, data[i]);
		}
	}

	return result;

	// TODO: error recovery, barrier to set layer
}

void device::free_texture(texture* texture)
{
	device_data_ptr_->device->destroyImageView(texture->image_view);
	device_data_ptr_->device->freeMemory(texture->memory);
	device_data_ptr_->device->destroyImage(texture->image);
	delete texture;
}

device::framebuffer_format_id device::create_framebuffer_format(std::vector<attachment_format> formats)
{
	std::vector<vk::AttachmentDescription> attachments;
	std::vector<vk::AttachmentReference> color_references;
	std::vector<vk::AttachmentReference> depth_stencil_references;
	std::vector<vk::AttachmentReference> resolve_references;

	for (uint32_t i = 0; i < formats.size(); i++)
	{
		bool is_color = uint16_t(formats[i].usage) & uint16_t(image_usage::color_attachment_bit);
		bool is_depth_stencil = uint16_t(formats[i].usage) & uint16_t(image_usage::depth_stencil_attachment_bit);
		bool is_resolve = uint16_t(formats[i].usage) & uint16_t(image_usage::resolve_attachment_bit);
		bool is_sampled = uint16_t(formats[i].usage) & uint16_t(image_usage::sampled_bit);
		bool is_storage = uint16_t(formats[i].usage) & uint16_t(image_usage::storage_bit);
		
		vk::AttachmentLoadOp load_op = vk::AttachmentLoadOp::eClear, stencil_load_op = vk::AttachmentLoadOp::eClear;
		vk::ImageLayout initial_layout = vk::ImageLayout::eUndefined;
		vk::AttachmentStoreOp store_op, stencil_store_op;
		vk::ImageLayout final_layout;
		if (is_color) {
			store_op = vk::AttachmentStoreOp::eDontCare;
			stencil_store_op = vk::AttachmentStoreOp::eDontCare;
			final_layout = is_sampled ? vk::ImageLayout::eShaderReadOnlyOptimal : (is_storage ? vk::ImageLayout::eGeneral : vk::ImageLayout::eColorAttachmentOptimal);
		}
		else if (is_depth_stencil) {
			store_op = vk::AttachmentStoreOp::eDontCare;
			stencil_store_op = vk::AttachmentStoreOp::eDontCare;
			final_layout = is_sampled ? vk::ImageLayout::eShaderReadOnlyOptimal : (is_storage ? vk::ImageLayout::eGeneral : vk::ImageLayout::eDepthStencilAttachmentOptimal);
		}
		else {
			load_op = vk::AttachmentLoadOp::eDontCare;
			stencil_load_op = vk::AttachmentLoadOp::eDontCare;
			initial_layout = vk::ImageLayout::eUndefined;
		}

		vk::AttachmentDescription description = vk::AttachmentDescription()
			.setFormat(impl_::vulkan_formats[uint8_t(formats[i].format)])
			.setSamples(impl_::rasterization_sample_count[uint8_t(formats[i].samples)])
			.setLoadOp(load_op)
			.setStencilLoadOp(stencil_load_op)
			.setInitialLayout(initial_layout)
			.setStoreOp(store_op)
			.setStencilStoreOp(stencil_store_op)
			.setFinalLayout(final_layout);
		attachments.push_back(description);

		if (is_color)
		{
			color_references.push_back(vk::AttachmentReference()
				.setAttachment(i)
				.setLayout(vk::ImageLayout::eColorAttachmentOptimal)
			);
		}
		else if (is_depth_stencil)
		{
			depth_stencil_references.push_back(vk::AttachmentReference()
				.setAttachment(i)
				.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
			);
		}
		else if (is_resolve)
		{
			resolve_references.push_back(vk::AttachmentReference()
				.setAttachment(i)
				.setLayout(vk::ImageLayout::eColorAttachmentOptimal)
			);
		}
	}

	vk::SubpassDescription subpass = vk::SubpassDescription()
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setInputAttachmentCount(0)
		.setPInputAttachments(nullptr)
		.setColorAttachmentCount(color_references.size())
		.setPColorAttachments(color_references.data())
		.setPDepthStencilAttachment(depth_stencil_references.data())
		.setPResolveAttachments(resolve_references.data())
		.setPreserveAttachmentCount(0)
		.setPResolveAttachments(nullptr);

	vk::RenderPassCreateInfo render_pass_create_info = vk::RenderPassCreateInfo()
		.setAttachmentCount(attachments.size())
		.setPAttachments(attachments.data())
		.setSubpassCount(1)
		.setPSubpasses(&subpass)
		.setDependencyCount(0)
		.setPDependencies(nullptr);

	vk::RenderPass render_pass = device_data_ptr_->device->createRenderPass(render_pass_create_info);

	impl_::framebuffer_format new_format = impl_::framebuffer_format{
		static_cast<uint32_t>(color_references.size()),
		render_pass,
		formats[0].samples
	};

	framebuffer_format_id format_id = framebuffer_format_id(device_data_ptr_->framebuffer_formats.size());
	device_data_ptr_->framebuffer_formats.push_back(new_format);
	return format_id;
}

framebuffer* device::create_framebuffer(std::vector<texture*> texture_attachments, framebuffer_format_id* format_check)
{
	std::vector<attachment_format> attachment_formats;
	attachment_formats.reserve(texture_attachments.size());
	for (texture* tex_ptr : texture_attachments) {
		attachment_format temp_attachment_format = attachment_format{
			tex_ptr->create_params.format,
			tex_ptr->create_params.samples,
			tex_ptr->create_params.usage
		};
		attachment_formats.push_back(temp_attachment_format);
	}

	framebuffer_format_id new_format_id = create_framebuffer_format(attachment_formats);

	framebuffer* result = new framebuffer;
	result->format_id = new_format_id;
	result->attachments = texture_attachments;
	result->width = texture_attachments[0]->create_params.width;
	result->height = texture_attachments[0]->create_params.height;
	return result;
}

void device::free_framebuffer(framebuffer* framebuffer)
{
	delete framebuffer;
}

sampler* device::create_sampler(const create_sampler_params& params)
{
	vk::SamplerCreateInfo sampler_create_info = vk::SamplerCreateInfo()
		.setMagFilter(params.mag_filter == filter::linear ? vk::Filter::eLinear : vk::Filter::eNearest)
		.setMinFilter(params.min_filter == filter::linear ? vk::Filter::eLinear : vk::Filter::eNearest)
		.setMipmapMode(params.mipmap_filter == filter::linear ? vk::SamplerMipmapMode::eLinear : vk::SamplerMipmapMode::eNearest)
		.setAddressModeU(impl_::address_modes[uint8_t(params.address_mode_u)])
		.setAddressModeV(impl_::address_modes[uint8_t(params.address_mode_v)])
		.setAddressModeW(impl_::address_modes[uint8_t(params.address_mode_w)])
		.setMipLodBias(params.mip_lod_bias)
		.setAnisotropyEnable(params.enable_ansiotropy)
		.setMaxAnisotropy(params.max_ansiotropy)
		.setCompareEnable(params.enable_compare)
		.setCompareOp(impl_::compare_operators[uint8_t(params.compare_op)])
		.setMinLod(params.min_lod)
		.setMaxLod(params.max_lod)
		.setBorderColor(impl_::sampler_border_colors[uint8_t(params.border_color)])
		.setUnnormalizedCoordinates(params.unnormalized_coords);

	sampler* result = new sampler;
	result->sampler = device_data_ptr_->device->createSampler(sampler_create_info);
	return result;
}

void device::free_sampler(sampler* sampler)
{
	device_data_ptr_->device->destroySampler(sampler->sampler);
	delete sampler;
}

buffer* device::create_buffer(buffer_type type, const size_t size, const void* data)
{
	vk::BufferUsageFlags usage_flags = vk::BufferUsageFlags();
	if(type == buffer_type::vertex)
	{
		usage_flags = vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer;
	}
	else if(type == buffer_type::index)
	{
		usage_flags = vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer;
	}
	else if (type == buffer_type::uniform)
	{
		usage_flags = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer;
	}

	vk::BufferCreateInfo buffer_create_info = vk::BufferCreateInfo()
		.setSize(size)
		.setUsage(usage_flags)
		.setSharingMode(vk::SharingMode::eExclusive);

	buffer* result = new buffer;
	result->size = size;
	result->buffer = device_data_ptr_->device->createBuffer(buffer_create_info);

	vk::MemoryRequirements mem_requirements = device_data_ptr_->device->getBufferMemoryRequirements(result->buffer);

	vk::MemoryAllocateInfo mem_alloc_info = vk::MemoryAllocateInfo()
		.setAllocationSize(mem_requirements.size)
		.setMemoryTypeIndex(impl_::find_memory_type(mem_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible, device_data_ptr_->physical_device));

	// TODO: use vma
	result->memory = device_data_ptr_->device->allocateMemory(mem_alloc_info);
	device_data_ptr_->device->bindBufferMemory(result->buffer, result->memory, 0);

	if (data)
	{
		void* mapped_data = device_data_ptr_->device->mapMemory(result->memory, 0, size);
		memcpy(mapped_data, data, size);
		device_data_ptr_->device->unmapMemory(result->memory);
	}

	return result;
}

void device::update_buffer(buffer* buf, const size_t offset, const size_t size, const void const* data)
{
	// TODO: implement updating buffers
	void* mapped_data = device_data_ptr_->device->mapMemory(buf->memory, 0, size);
	memcpy(mapped_data, data, size);
	device_data_ptr_->device->unmapMemory(buf->memory);
}

void device::free_buffer(buffer* buffer)
{
	device_data_ptr_->device->destroyBuffer(buffer->buffer);
	device_data_ptr_->device->freeMemory(buffer->memory);
	delete buffer;
}

device::vertex_format_id device::create_vertex_format(std::vector<vertex_attribute> attributes)
{
	impl_::vertex_format new_format = impl_::vertex_format{
		std::vector<vk::VertexInputBindingDescription>(attributes.size()),
		std::vector<vk::VertexInputAttributeDescription>(attributes.size()),
		vk::PipelineVertexInputStateCreateInfo()
	};

	vertex_format_id format_id = device_data_ptr_->vertex_formats.size();
	device_data_ptr_->vertex_formats.push_back(std::move(new_format));

	std::vector<vk::VertexInputBindingDescription>& binding_descs = device_data_ptr_->vertex_formats[format_id].bindings;
	std::vector<vk::VertexInputAttributeDescription>& attribute_descs = device_data_ptr_->vertex_formats[format_id].attributes;

	for (size_t i = 0; i < attributes.size(); i++)
	{
		binding_descs[i].binding = i;
		binding_descs[i].stride = attributes[i].stride;
		binding_descs[i].inputRate = (attributes[i].input_rate == vertex_input_rate::instance) ? vk::VertexInputRate::eInstance : vk::VertexInputRate::eVertex;
		attribute_descs[i].binding = i;
		attribute_descs[i].location = attributes[i].location;
		attribute_descs[i].format = impl_::vulkan_formats[uint8_t(attributes[i].format)];
		attribute_descs[i].offset = attributes[i].offset;
	}

	device_data_ptr_->vertex_formats[format_id].input_state_create_info = vk::PipelineVertexInputStateCreateInfo()
		.setFlags(vk::PipelineVertexInputStateCreateFlags())
		.setVertexAttributeDescriptionCount(attribute_descs.size())
		.setPVertexAttributeDescriptions(attribute_descs.size() != 0 ? attribute_descs.data() : nullptr)
		.setVertexBindingDescriptionCount(binding_descs.size())
		.setPVertexBindingDescriptions(binding_descs.size() != 0 ? binding_descs.data() : nullptr);

	return format_id;
}

uniform_set* device::create_uniform_set(std::vector<uniform_info> uniform_infos, shader* shader)
{
	static constexpr uint32_t max_descriptors_per_pool = 64;
	
	
	std::vector<uint32_t> type_counts = std::vector<uint32_t>(size_t(uniform_type::max), 0);
	for (const uniform_info& uniform_info : uniform_infos)
	{
		type_counts[uint8_t(uniform_info.type)]++;
	}

	std::vector<vk::DescriptorPoolSize> desc_pool_sizes;
	for (uint32_t i = 0; i < type_counts.size(); i++)
	{
		if (type_counts[i] > 0)
		{
			vk::DescriptorPoolSize temp_desc_pool_size = vk::DescriptorPoolSize(
				impl_::vulkan_uniform_types[i], type_counts[i] * max_descriptors_per_pool
			);
			desc_pool_sizes.push_back(temp_desc_pool_size);
		}
	}

	vk::DescriptorPoolCreateInfo desc_pool_create_info = vk::DescriptorPoolCreateInfo()
		.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet) // What the fuck?
		.setMaxSets(max_descriptors_per_pool)
		.setPoolSizeCount(desc_pool_sizes.size())
		.setPPoolSizes(desc_pool_sizes.data());

	vk::UniqueDescriptorPool temp_desc_pool = device_data_ptr_->device->createDescriptorPoolUnique(desc_pool_create_info);
	device_data_ptr_->desc_pools.push_back(std::move(temp_desc_pool));

	std::vector<vk::DescriptorSetLayoutBinding> layout_bindings;
	layout_bindings.reserve(uniform_infos.size());
	for (const uniform_info& uniform_info : uniform_infos)
	{
		vk::ShaderStageFlags stage_bits = vk::ShaderStageFlags();
		for (shader_stage stage : uniform_info.stages)
		{
			stage_bits |= impl_::shader_stage_bits[uint8_t(stage)];
		}

		vk::DescriptorSetLayoutBinding temp_layout_binding = vk::DescriptorSetLayoutBinding(
			uniform_info.binding, impl_::vulkan_uniform_types[uint8_t(uniform_info.type)], 1, stage_bits, nullptr
		);

		layout_bindings.push_back(temp_layout_binding);
	}

	uniform_set* result = new uniform_set;
	vk::DescriptorSetLayoutCreateInfo layout_create_info = vk::DescriptorSetLayoutCreateInfo()
		.setBindingCount(layout_bindings.size())
		.setPBindings(layout_bindings.data());
	result->desc_set_layout = device_data_ptr_->device->createDescriptorSetLayout(layout_create_info);

	vk::DescriptorSetAllocateInfo desc_set_alloc_info = vk::DescriptorSetAllocateInfo()
		.setDescriptorPool(device_data_ptr_->desc_pools.back().get())
		.setDescriptorSetCount(1)
		.setPSetLayouts(&result->desc_set_layout);
	std::vector<vk::DescriptorSet> desc_sets = device_data_ptr_->device->allocateDescriptorSets(desc_set_alloc_info);
	result->desc_set = desc_sets[0];
	result->owning_pool = device_data_ptr_->desc_pools.back().get();

	std::vector<vk::WriteDescriptorSet> writes;
	writes.reserve(uniform_infos.size());
	vk::DescriptorBufferInfo buffer_info;
	for (const uniform_info& uniform_info : uniform_infos)
	{
		vk::WriteDescriptorSet write = vk::WriteDescriptorSet()
			.setDstSet(result->desc_set)
			.setDstBinding(uniform_info.binding)
			.setDstArrayElement(0)
			.setDescriptorCount(1)
			.setDescriptorType(impl_::vulkan_uniform_types[uint8_t(uniform_info.type)]);

		if (uniform_info.type == rhi::uniform_type::uniform_buffer)
		{
			buffer* buf = std::get<buffer*>(uniform_info.data[0]);
			buffer_info = vk::DescriptorBufferInfo()
				.setBuffer(buf->buffer)
				.setOffset(0)
				.setRange(buf->size);
			
			write.setPBufferInfo(&buffer_info)
				.setPImageInfo(nullptr)
				.setPTexelBufferView(nullptr);
		}
		// TODO: rest of uniform types

		writes.push_back(write);
	}

	device_data_ptr_->device->updateDescriptorSets(writes, {});

	return result;
}

void device::free_uniform_set(uniform_set* uniform_set)
{
	device_data_ptr_->device->freeDescriptorSets(uniform_set->owning_pool, uniform_set->desc_set);
	device_data_ptr_->device->destroyDescriptorSetLayout(uniform_set->desc_set_layout);
	delete uniform_set;
}

graphics_pipeline* device::create_graphics_pipeline(std::vector<shader*> shaders, std::vector<uniform_set*> uniforms, framebuffer_format_id ffid, vertex_format_id vfid, primitive_topology topology, const pipeline_rasterization_state& rasterization_state,
	const pipeline_multisample_state& multisample_state, const pipeline_depth_stencil_state& depth_stencil_state, const pipeline_color_blend_state& color_blend_state, dynamic_state dynamic_states)
{
	const impl_::vertex_format& v_fmt = device_data_ptr_->vertex_formats[vfid];
	
	vk::PipelineInputAssemblyStateCreateInfo input_assembly_create_info = vk::PipelineInputAssemblyStateCreateInfo()
		.setTopology(impl_::topology_list[uint8_t(topology)])
		.setPrimitiveRestartEnable(topology == primitive_topology::triangle_strips_with_restart_index);

	vk::PipelineViewportStateCreateInfo viewport_state_create_info = vk::PipelineViewportStateCreateInfo()
		.setViewportCount(1) 
		.setPViewports(nullptr)
		.setScissorCount(1)
		.setPScissors(nullptr);

	vk::CullModeFlags cull_flags = 
		((uint8_t(rasterization_state.cull_mode) & uint8_t(cull_mode::back_bit)) != 0x00 ? vk::CullModeFlagBits::eBack : vk::CullModeFlagBits::eNone) |
		((uint8_t(rasterization_state.cull_mode) & uint8_t(cull_mode::front_bit)) != 0x00 ? vk::CullModeFlagBits::eFront : vk::CullModeFlagBits::eNone);
	vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info = vk::PipelineRasterizationStateCreateInfo()
		.setDepthClampEnable(rasterization_state.enable_depth_clamp)
		.setRasterizerDiscardEnable(rasterization_state.enable_rasterizer_discard)
		.setPolygonMode(rasterization_state.wireframe ? vk::PolygonMode::eLine : vk::PolygonMode::eFill)
		.setCullMode(cull_flags)
		.setFrontFace(rasterization_state.front_face == front_face::clockwise ? vk::FrontFace::eClockwise : vk::FrontFace::eCounterClockwise)
		.setDepthBiasEnable(false)
		.setDepthBiasConstantFactor(0.f)
		.setDepthBiasClamp(0.f)
		.setDepthBiasSlopeFactor(0.f)
		.setLineWidth(1.f);

	std::vector<vk::SampleMask> sample_mask;
	for (uint32_t sample : multisample_state.sample_mask)
	{
		sample_mask.push_back(vk::SampleMask(sample));
	}

	vk::PipelineMultisampleStateCreateInfo multisample_state_create_info = vk::PipelineMultisampleStateCreateInfo()
		.setRasterizationSamples(impl_::rasterization_sample_count[uint8_t(multisample_state.sample_count)])
		.setSampleShadingEnable(multisample_state.enable_sample_shading)
		.setMinSampleShading(multisample_state.min_sample_shading)
		.setPSampleMask(sample_mask.size() > 0 ? sample_mask.data() : nullptr)
		.setAlphaToCoverageEnable(multisample_state.enable_alpha_to_coverage)
		.setAlphaToOneEnable(multisample_state.enable_alpha_to_one);

	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = vk::PipelineDepthStencilStateCreateInfo()
		.setDepthTestEnable(depth_stencil_state.enable_depth_test)
		.setDepthWriteEnable(depth_stencil_state.enable_depth_write)
		.setDepthCompareOp(impl_::compare_operators[uint8_t(depth_stencil_state.depth_compare_operator)])
		.setDepthBoundsTestEnable(depth_stencil_state.enable_depth_range)
		.setStencilTestEnable(depth_stencil_state.enable_stencil_test)
		.setFront(vk::StencilOpState())
		.setBack(vk::StencilOpState())
		.setMinDepthBounds(depth_stencil_state.min_depth_range)
		.setMaxDepthBounds(depth_stencil_state.max_depth_range);

	// TODO: blend attachments
	vk::PipelineColorBlendAttachmentState color_blend_attachment = vk::PipelineColorBlendAttachmentState()
		.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
			vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
		.setBlendEnable(false);
		//vk::PipelineColorBlendAttachmentState()
		/*.setBlendEnable(true)
		.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
		.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
		.setColorBlendOp(vk::BlendOp::eAdd)
		.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
		.setDstAlphaBlendFactor(vk::BlendFactor::eZero)
		.setAlphaBlendOp(vk::BlendOp::eAdd);*/

	vk::PipelineColorBlendStateCreateInfo color_blend_state_create_info = vk::PipelineColorBlendStateCreateInfo()
		.setLogicOpEnable(color_blend_state.enable_logic_op)
		.setLogicOp(impl_::logic_operations[uint8_t(color_blend_state.logic_op)])
		.setAttachmentCount(1)
		.setPAttachments(&color_blend_attachment)
		.setBlendConstants({ 0.f, 0.f, 0.f, 0.f });

	std::vector<vk::DynamicState> vk_dynamic_states;
	vk_dynamic_states.push_back(vk::DynamicState::eViewport);
	vk_dynamic_states.push_back(vk::DynamicState::eScissor);

	if (uint8_t(dynamic_states) & uint8_t(dynamic_state::line_width))
	{
		vk_dynamic_states.push_back(vk::DynamicState::eLineWidth);
	}
	if (uint8_t(dynamic_states) & uint8_t(dynamic_state::depth_bias))
	{
		vk_dynamic_states.push_back(vk::DynamicState::eDepthBias);
	}
	if (uint8_t(dynamic_states) & uint8_t(dynamic_state::blend_constants))
	{
		vk_dynamic_states.push_back(vk::DynamicState::eBlendConstants);
	}
	if (uint8_t(dynamic_states) & uint8_t(dynamic_state::depth_bounds))
	{
		vk_dynamic_states.push_back(vk::DynamicState::eDepthBounds);
	}
	if (uint8_t(dynamic_states) & uint8_t(dynamic_state::stencil_compare_mask))
	{
		vk_dynamic_states.push_back(vk::DynamicState::eStencilCompareMask);
	}
	if (uint8_t(dynamic_states) & uint8_t(dynamic_state::stencil_write_mask))
	{
		vk_dynamic_states.push_back(vk::DynamicState::eStencilWriteMask);
	}
	if (uint8_t(dynamic_states) & uint8_t(dynamic_state::stencil_reference))
	{
		vk_dynamic_states.push_back(vk::DynamicState::eStencilReference);
	}

	vk::PipelineDynamicStateCreateInfo dynamic_state_create_info = vk::PipelineDynamicStateCreateInfo()
		.setDynamicStateCount(vk_dynamic_states.size())
		.setPDynamicStates(vk_dynamic_states.data());

	std::vector<vk::PipelineShaderStageCreateInfo>  vk_shader_stages;
	for (shader* shader : shaders)
	{
		vk_shader_stages.push_back(shader->pipeline_stage_create_info);
	}

	std::vector<vk::DescriptorSetLayout> set_layouts;
	for (uniform_set* uniform_set : uniforms)
	{
		set_layouts.push_back(uniform_set->desc_set_layout);
	}

	graphics_pipeline* result = new graphics_pipeline;
	vk::PipelineLayoutCreateInfo layout_create_info = vk::PipelineLayoutCreateInfo()
		.setSetLayoutCount(set_layouts.size())
		.setPSetLayouts(set_layouts.data());
	result->layout = device_data_ptr_->device->createPipelineLayout(layout_create_info);
	
	vk::RenderPass render_pass = ffid == surface_ffid ? device_data_ptr_->surface_render_pass.get() : device_data_ptr_->framebuffer_formats[ffid].render_pass;
	//vk::Extent2D fb_extent = ffid == surface_ffid ? device_data_ptr_->surface_extent : device_data_ptr_->framebuffer_formats[ffid].
	vk::GraphicsPipelineCreateInfo gfx_pipeline_create_info = vk::GraphicsPipelineCreateInfo()
		.setStageCount(vk_shader_stages.size())
		.setPStages(vk_shader_stages.data())
		.setPVertexInputState(&v_fmt.input_state_create_info)
		.setPInputAssemblyState(&input_assembly_create_info)
		.setPTessellationState(nullptr) // TODO
		.setPViewportState(&viewport_state_create_info)
		.setPRasterizationState(&rasterization_state_create_info)
		.setPMultisampleState(&multisample_state_create_info)
		.setPDepthStencilState(&depth_stencil_state_create_info)
		.setPColorBlendState(&color_blend_state_create_info)
		.setPDynamicState(&dynamic_state_create_info)
		.setLayout(result->layout)
		.setRenderPass(render_pass)
		.setSubpass(0)
		.setBasePipelineHandle({}) // VK_NULL_HANDLE
		.setBasePipelineIndex(0);

	result->pipeline = vk::Pipeline(device_data_ptr_->device->createGraphicsPipeline({ /*VK_NULL_HANDLE*/ }, gfx_pipeline_create_info));

	return result;
}

void device::free_graphics_pipeline(graphics_pipeline* gfx_pipeline)
{
	device_data_ptr_->device->destroyPipeline(gfx_pipeline->pipeline);
	device_data_ptr_->device->destroyPipelineLayout(gfx_pipeline->layout);
	delete gfx_pipeline;
}

device::cmd_buf_id device::begin_gfx_cmd_buf_for_surface(const color& clear_color)
{
	cmd_buf_id cmd_id = device_data_ptr_->frames[device_data_ptr_->frame].cmd_buf_id;
	vk::CommandBuffer& cmd_buf = device_data_ptr_->cmd_bufs[cmd_id].get();

	vk::CommandBufferBeginInfo cmd_buf_begin_info = vk::CommandBufferBeginInfo()
		.setPInheritanceInfo(nullptr);
	cmd_buf.begin(cmd_buf_begin_info);

	std::vector<vk::ClearValue> clear_values = { vk::ClearColorValue(clear_color), vk::ClearDepthStencilValue(1.f, 0)};
	vk::RenderPassBeginInfo render_pass_begin_info = vk::RenderPassBeginInfo()
		.setRenderPass(device_data_ptr_->surface_render_pass.get())
		.setFramebuffer(device_data_ptr_->surface_frame_data[device_data_ptr_->frame].framebuffer.get())
		.setRenderArea(vk::Rect2D()
			.setExtent(device_data_ptr_->surface_extent)
			.setOffset(vk::Offset2D())
		)
		.setClearValueCount(clear_values.size())
		.setPClearValues(clear_values.data());

	cmd_buf.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

	vk::Viewport temp_viewport = vk::Viewport()
		.setX(0.f)
		.setY(0.f)
		.setWidth(device_data_ptr_->surface_extent.width)
		.setHeight(device_data_ptr_->surface_extent.height)
		.setMinDepth(0.f)
		.setMaxDepth(1.f);

	cmd_buf.setViewport(0, { temp_viewport });

	vk::Rect2D scissor = vk::Rect2D()
		.setOffset(vk::Offset2D(0, 0))
		.setExtent(device_data_ptr_->surface_extent);

	cmd_buf.setScissor(0, scissor);

	return cmd_id;
}

device::cmd_buf_id device::begin_gfx_cmd_buf(const begin_gfx_cmd_buf_params& params)
{
	const impl_::framebuffer_format& fb_fmt = device_data_ptr_->framebuffer_formats[params.framebuffer->format_id];

	std::vector<vk::ImageView> attachments;
	for (texture* tex : params.framebuffer->attachments)
	{
		attachments.push_back(tex->image_view);
	}

	vk::FramebufferCreateInfo framebuffer_create_info = vk::FramebufferCreateInfo()
		.setRenderPass(fb_fmt.render_pass)
		.setAttachmentCount(attachments.size())
		.setPAttachments(attachments.data())
		.setWidth(params.framebuffer->width)
		.setHeight(params.framebuffer->height)
		.setLayers(1);

	device_data_ptr_->frames[device_data_ptr_->frame].bound_framebuffer = device_data_ptr_->device->createFramebuffer(framebuffer_create_info);

	std::vector<vk::ClearValue> clear_values;
	uint32_t color_idx = 0;
	for (texture* tex : params.framebuffer->attachments)
	{
		vk::ClearValue clear_value;
		if (color_idx < params.clear_color_values.size() && (uint8_t(tex->create_params.usage) & uint8_t(image_usage::color_attachment_bit)))
		{
			const color& color = params.clear_color_values[color_idx];
			clear_value.color = vk::ClearColorValue()
				.setFloat32(color);
		}
		else if (uint8_t(tex->create_params.usage) & uint8_t(image_usage::depth_stencil_attachment_bit))
		{
			clear_value.depthStencil = vk::ClearDepthStencilValue()
				.setDepth(params.clear_depth)
				.setStencil(params.clear_stencil); 
		}
		else
		{
			clear_value.color = vk::ClearColorValue()
				.setFloat32({ 0.f, 0.f, 0.f, 0.f });
		}

		clear_values.push_back(clear_value);
	}

	// TODO: sampling barrier
	cmd_buf_id cmd_id = device_data_ptr_->frames[device_data_ptr_->frame].cmd_buf_id;
	vk::CommandBuffer& cmd_buf = device_data_ptr_->cmd_bufs[cmd_id].get();

	vk::CommandBufferBeginInfo cmd_buf_begin_info = vk::CommandBufferBeginInfo()
		.setPInheritanceInfo(nullptr);
	cmd_buf.begin(cmd_buf_begin_info);

	vk::RenderPassBeginInfo render_pass_begin_info = vk::RenderPassBeginInfo()
		.setRenderPass(fb_fmt.render_pass)
		.setFramebuffer(device_data_ptr_->frames[device_data_ptr_->frame].bound_framebuffer)
		.setRenderArea(vk::Rect2D()
			.setExtent(vk::Extent2D(params.framebuffer->width, params.framebuffer->height))
			.setOffset(vk::Offset2D())
		)
		.setClearValueCount(clear_values.size())
		.setPClearValues(clear_values.data());

	cmd_buf.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

	vk::Viewport temp_viewport = vk::Viewport()
		.setX(0.f)
		.setY(0.f)
		.setWidth(params.framebuffer->width)
		.setHeight(params.framebuffer->height)
		.setMinDepth(0.f)
		.setMaxDepth(1.f);

	cmd_buf.setViewport(0, { temp_viewport });

	vk::Rect2D scissor = vk::Rect2D()
		.setOffset(vk::Offset2D(0, 0))
		.setExtent(vk::Extent2D(params.framebuffer->width, params.framebuffer->height));

	cmd_buf.setScissor(0, scissor);

	return cmd_id;
}

void device::gfx_cmd_buf_bind_pipeline(cmd_buf_id id, graphics_pipeline* pipeline)
{
	vk::CommandBuffer& cmd_buf = device_data_ptr_->cmd_bufs[id].get();
	cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->pipeline);
}

void device::gfx_cmd_buf_bind_uniform_set(cmd_buf_id id, uniform_set* uniform_set, graphics_pipeline* pipeline, uint32_t index)
{
	vk::CommandBuffer& cmd_buf = device_data_ptr_->cmd_bufs[id].get();
	cmd_buf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline->layout, 0, { uniform_set->desc_set }, { });
}

void device::gfx_cmd_buf_bind_vertex_array(cmd_buf_id id, buffer* vertex_array)
{
	vk::CommandBuffer& cmd_buf = device_data_ptr_->cmd_bufs[id].get();
	cmd_buf.bindVertexBuffers(0, { vertex_array->buffer }, { 0 });
}

void device::gfx_cmd_buf_bind_index_array(cmd_buf_id id, buffer* index_array, index_type index_type)
{
	vk::CommandBuffer& cmd_buf = device_data_ptr_->cmd_bufs[id].get();
	cmd_buf.bindIndexBuffer(index_array->buffer, 0, index_type == index_type::uint16 ? vk::IndexType::eUint16 : vk::IndexType::eUint32);
}

void device::gfx_cmd_buf_draw(cmd_buf_id id, bool use_indices, uint32_t element_count, uint32_t instances)
{
	vk::CommandBuffer& cmd_buf = device_data_ptr_->cmd_bufs[id].get();
	if (use_indices)
	{
		cmd_buf.drawIndexed(element_count, instances, 0, 0, 0);
	}
	else
	{
		cmd_buf.draw(element_count, instances, 0, 0);
	}
}

void device::gfx_cmd_buf_end(cmd_buf_id id)
{
	vk::CommandBuffer& cmd_buf = device_data_ptr_->cmd_bufs[id].get();
	cmd_buf.endRenderPass();
	cmd_buf.end();
}

} // namespace rhi
} // namespace xs