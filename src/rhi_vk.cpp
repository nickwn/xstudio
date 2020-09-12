#include "rhi.hpp"
#include <vulkan/vulkan.hpp>
#include <vector>

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

	// public interface objects

	struct device_data
	{
		vk::Device device;
		vk::PhysicalDevice physical_device;
		std::vector<framebuffer_format> framebuffer_formats;
		std::vector<vertex_format> vertex_formats;
		std::vector<vk::UniqueDescriptorPool> desc_pools; // TODO: pool reference count
	};

	struct shader
	{
		vk::PipelineShaderStageCreateInfo pipeline_stage_create_info;
	};

	struct graphics_pipeline
	{

	};

	struct texture
	{
		rendering_device::create_texture_params create_params; // should be on class but am lazy
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
		rendering_device::framebuffer_format_id format_id;
		std::vector<texture*> attachments;
		uint32_t width;
		uint32_t height;
	};

	struct buffer
	{
		vk::Buffer buffer;
		vk::DeviceMemory memory;
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
        vk::Format::eR64G64B64A64Sfloat
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

} // namespace impl_

shader* rendering_device::create_shader(const create_shader_params& params)
{
	vk::ShaderModuleCreateInfo shader_module_create_info = vk::ShaderModuleCreateInfo()
		.setFlags(vk::ShaderModuleCreateFlags())
		.setCodeSize(params.bytecode.size())
		.setPCode(reinterpret_cast<uint32_t*>(const_cast<char*>(params.bytecode.c_str())));
	vk::ShaderModule shader_module = device_data_ptr_->device.createShaderModule(shader_module_create_info);
	
	shader* result = new shader;
	result->pipeline_stage_create_info = vk::PipelineShaderStageCreateInfo()
		.setFlags(vk::PipelineShaderStageCreateFlags())
		.setStage(impl_::shader_stage_bits[uint8_t(params.stage)])
		.setModule(shader_module)
		.setPName("main")
		.setPSpecializationInfo(nullptr);

	return result;
}

void rendering_device::free_shader(shader* shader)
{
	device_data_ptr_->device.destroyShaderModule(shader->pipeline_stage_create_info.module);
	delete shader;
}

texture* rendering_device::create_texture(const create_texture_params& params, std::vector<std::vector<uint8_t>> data)
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
		(image_type == vk::ImageType::e3D || image_type == vk::ImageType::e3D) ? params.height : 1,
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
	result->image = device_data_ptr_->device.createImage(image_create_info);

	vk::MemoryRequirements mem_requirements = device_data_ptr_->device.getImageMemoryRequirements(result->image);

	vk::MemoryAllocateInfo mem_alloc_info = vk::MemoryAllocateInfo()
		.setAllocationSize(mem_requirements.size)
		.setMemoryTypeIndex(impl_::find_memory_type(mem_requirements.memoryTypeBits, vk::MemoryPropertyFlags(), device_data_ptr_->physical_device));
	
	// TODO: use vma
	result->memory = device_data_ptr_->device.allocateMemory(mem_alloc_info);
	device_data_ptr_->device.bindImageMemory(result->image, result->memory, 0);

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

	result->image_view = device_data_ptr_->device.createImageView(image_view_create_info);

	if (data.size() > 0) {
		for (uint32_t i = 0; i < image_create_info.arrayLayers; i++) {
			// TODO: texture_update(result, i, data[i]);
		}
	}

	return result;

	// TODO: error recovery, barrier to set layer
}

void rendering_device::free_texture(texture* texture)
{
	device_data_ptr_->device.destroyImageView(texture->image_view);
	device_data_ptr_->device.freeMemory(texture->memory);
	device_data_ptr_->device.destroyImage(texture->image);
	delete texture;
}

rendering_device::framebuffer_format_id rendering_device::create_framebuffer_format(std::vector<attachment_format> formats)
{
	uint32_t num_color_references;
	vk::RenderPass render_pass = device_data_ptr_->create_render_pass(initial_action::clear, final_action::discard,
		initial_action::clear, final_action::discard, &num_color_references);

	impl_::framebuffer_format new_format = impl_::framebuffer_format{
		num_color_references,
		render_pass,
		formats[0].samples
	};

	framebuffer_format_id format_id = framebuffer_format_id(device_data_ptr_->framebuffer_formats.size());
	device_data_ptr_->framebuffer_formats.push_back(new_format);
	return format_id;
}

framebuffer* rendering_device::create_framebuffer(std::vector<texture*> texture_attachments, framebuffer_format_id* format_check)
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

void rendering_device::free_framebuffer(framebuffer* framebuffer)
{
	delete framebuffer;
}

sampler* rendering_device::create_sampler(const create_sampler_params& params)
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
	result->sampler = device_data_ptr_->device.createSampler(sampler_create_info);
	return result;
}

void rendering_device::free_sampler(sampler* sampler)
{
	device_data_ptr_->device.destroySampler(sampler->sampler);
	delete sampler;
}

buffer* rendering_device::create_buffer(buffer_type type, const size_t size, const void* data)
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
	result->buffer = device_data_ptr_->device.createBuffer(buffer_create_info);

	vk::MemoryRequirements mem_requirements = device_data_ptr_->device.getBufferMemoryRequirements(result->buffer);

	vk::MemoryAllocateInfo mem_alloc_info = vk::MemoryAllocateInfo()
		.setAllocationSize(mem_requirements.size)
		.setMemoryTypeIndex(impl_::find_memory_type(mem_requirements.memoryTypeBits, vk::MemoryPropertyFlags(), device_data_ptr_->physical_device));

	// TODO: use vma
	result->memory = device_data_ptr_->device.allocateMemory(mem_alloc_info);
	device_data_ptr_->device.bindBufferMemory(result->buffer, result->memory, 0);

	return result;
}

void rendering_device::update_buffer(const buffer const*, const size_t offset, const size_t size, const void const* data)
{
	// TODO: implement updating buffers
}

void rendering_device::free_buffer(buffer* buffer)
{
	device_data_ptr_->device.destroyBuffer(buffer->buffer);
	device_data_ptr_->device.freeMemory(buffer->memory);
	delete buffer;
}

rendering_device::vertex_format_id rendering_device::create_vertex_format(std::vector<vertex_attribute> attributes)
{
	std::vector<vk::VertexInputBindingDescription> binding_descs = std::vector<vk::VertexInputBindingDescription>(attributes.size());
	std::vector<vk::VertexInputAttributeDescription> attribute_descs = std::vector<vk::VertexInputAttributeDescription>(attributes.size());

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

	vk::PipelineVertexInputStateCreateInfo input_state_create_info = vk::PipelineVertexInputStateCreateInfo()
		.setFlags(vk::PipelineVertexInputStateCreateFlags())
		.setVertexAttributeDescriptionCount(attribute_descs.size())
		.setPVertexAttributeDescriptions(attribute_descs.data())
		.setVertexBindingDescriptionCount(binding_descs.size())
		.setPVertexBindingDescriptions(binding_descs.data());

	impl_::vertex_format new_format = impl_::vertex_format{
		std::move(binding_descs),
		std::move(attribute_descs),
		input_state_create_info
	};

	vertex_format_id format_id = device_data_ptr_->vertex_formats.size();
	device_data_ptr_->vertex_formats.push_back(std::move(new_format));
	return format_id;
}

uniform_set* rendering_device::create_uniform_set(std::vector<uniform_info> uniform_infos, shader* shader)
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

	vk::UniqueDescriptorPool temp_desc_pool = device_data_ptr_->device.createDescriptorPoolUnique(desc_pool_create_info);
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
	result->desc_set_layout = device_data_ptr_->device.createDescriptorSetLayout(layout_create_info);

	vk::DescriptorSetAllocateInfo desc_set_alloc_info = vk::DescriptorSetAllocateInfo()
		.setDescriptorPool(device_data_ptr_->desc_pools.back().get())
		.setDescriptorSetCount(1)
		.setPSetLayouts(&result->desc_set_layout);
	std::vector<vk::DescriptorSet> desc_sets = device_data_ptr_->device.allocateDescriptorSets(desc_set_alloc_info);
	result->desc_set = desc_sets[0];
	result->owning_pool = device_data_ptr_->desc_pools.back().get();

	return result;
	// TODO: udpate uniform sets
}

void rendering_device::free_uniform_set(uniform_set* uniform_set)
{
	device_data_ptr_->device.freeDescriptorSets(uniform_set->owning_pool, uniform_set->desc_set);
	device_data_ptr_->device.destroyDescriptorSetLayout(uniform_set->desc_set_layout);
	delete uniform_set;
}

graphics_pipeline* rendering_device::create_graphics_pipeline(std::vector<shader*> shaders, framebuffer_format_id ffid, vertex_format_id vfid, const pipeline_rasterization_state& rasterization_state,
	const pipeline_multisample_state& multisample_state, const pipeline_depth_stencil_state& depth_stencil_state, const pipeline_color_blend_state& color_blend_state, dynamic_state dynamic_states)
{

}

rendering_device::cmd_buf_id rendering_device::begin_gfx_cmd_buf(const begin_gfx_cmd_buf_params& params)
{

}

void rendering_device::gfx_cmd_buf_bind_pipeline(cmd_buf_id id, graphics_pipeline* pipeline)
{

}

void rendering_device::gfx_cmd_buf_bind_uniform_set(cmd_buf_id id, uniform_set* uniform_set, uint32_t index)
{

}

void rendering_device::gfx_cmd_buf_bind_vertex_array(cmd_buf_id id, buffer* vertex_array)
{

}

void rendering_device::gfx_cmd_buf_bind_index_array(cmd_buf_id id, buffer* index_array)
{

}

void rendering_device::gfx_cmd_buf_draw(cmd_buf_id id, bool use_indices, uint32_t instances = 1)
{

}

void rendering_device::gfx_cmd_buf_end(cmd_buf_id id)
{

}

} // namespace rhi