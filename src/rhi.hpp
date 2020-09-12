#include <vector>
#include <string>
#include <variant>
#include <memory>

#define DEFINE_SMART_PTR_METHODS(resource_type) \
template<typename... Ts> \
std::unique_ptr<resource_type> create_ ## resource_type ## _unique(Ts... args) \
{ \
    return std::unique_ptr<resource_type>(create_ ## resource_type(args...), \
        [device = shared_from_this()](resource_type* ptr){device->free_ ## resource_type(ptr);}); \
} \
template<typename... Ts> \
std::shared_ptr<resource_type> create_ ## resource_type ## _shared(Ts... args) \
{ \
    return std::shared_ptr<resource_type>(create_ ## resource_type(args...), \
        [device = shared_from_this()](resource_type* ptr){device->free_ ## resource_type(ptr);}); \
}

namespace rhi
{

namespace impl_
{
    struct device_data;

    struct shader;
    struct graphics_pipeline;
    struct texture;
    struct sampler;
    struct framebuffer;
    struct buffer;
    struct uniform_set;
}

using shader = impl_::shader;
using graphics_pipeline = impl_::graphics_pipeline;
using texture = impl_::texture;
using sampler = impl_::sampler;
using framebuffer = impl_::framebuffer;
using buffer = impl_::buffer;
using uniform_set = impl_::uniform_set;

enum class shader_stage : uint8_t
{
	vertex,
	fragment,
	tesselation_control,
	tesselation_evaluation,
	compute
};

enum class format : uint8_t
{
    R8_unorm,
    R8_snorm,
    R8_uscaled,
    R8_sscaled,
    R8_uint,
    R8_sint,
    R8_sRGB,
    R8G8_unorm,
    R8G8_snorm,
    R8G8_uscaled,
    R8G8_sscaled,
    R8G8_uint,
    R8G8_sint,
    R8G8_sRGB,
    R8G8B8_unorm,
    R8G8B8_snorm,
    R8G8B8_uscaled,
    R8G8B8_sscaled,
    R8G8B8_uint,
    R8G8B8_sint,
    R8G8B8_sRGB,
    B8G8R8_unorm,
    B8G8R8_snorm,
    B8G8R8_uscaled,
    B8G8R8_sscaled,
    B8G8R8_uint,
    B8G8R8_sint,
    B8G8R8_sRGB,
    R8G8B8A8_unorm,
    R8G8B8A8_snorm,
    R8G8B8A8_uscaled,
    R8G8B8A8_sscaled,
    R8G8B8A8_uint,
    R8G8B8A8_sint,
    R8G8B8A8_sRGB,
    B8G8R8A8_unorm,
    B8G8R8A8_snorm,
    B8G8R8A8_uscaled,
    B8G8R8A8_sscaled,
    B8G8R8A8_uint,
    B8G8R8A8_sint,
    B8G8R8A8_sRGB,
    R16_unorm,
    R16_snorm,
    R16_uscaled,
    R16_sscaled,
    R16_uint,
    R16_sint,
    R16_sfloat,
    R16G16_unorm,
    R16G16_snorm,
    R16G16_uscaled,
    R16G16_sscaled,
    R16G16_uint,
    R16G16_sint,
    R16G16_sfloat,
    R16G16B16_unorm,
    R16G16B16_snorm,
    R16G16B16_uscaled,
    R16G16B16_sscaled,
    R16G16B16_uint,
    R16G16B16_sint,
    R16G16B16_sfloat,
    R16G16B16A16_unorm,
    R16G16B16A16_snorm,
    R16G16B16A16_uscaled,
    R16G16B16A16_sscaled,
    R16G16B16A16_uint,
    R16G16B16A16_sint,
    R16G16B16A16_sfloat,
    R32_uint,
    R32_sint,
    R32_sfloat,
    R32G32_uint,
    R32G32_sint,
    R32G32_sfloat,
    R32G32B32_uint,
    R32G32B32_sint,
    R32G32B32_sfloat,
    R32G32B32A32_uint,
    R32G32B32A32_sint,
    R32G32B32A32_sfloat,
    R64_uint,
    R64_sint,
    R64_sfloat,
    R64G64_uint,
    R64G64_sint,
    R64G64_sfloat,
    R64G64B64_uint,
    R64G64B64_sint,
    R64G64B64_sfloat,
    R64G64B64A64_uint,
    R64G64B64A64_sint,
    R64G64B64A64_sfloat
};

enum class image_type : uint8_t
{
    e1D,
    e2D,
    e3D,
    cube,
    e1D_array,
    e2D_array,
    cube_array
};

enum class sample_counts : uint8_t
{
    e1_bit = 0x0001,
    e2_bit = 0x0002,
    e4_bit = 0x0004,
    e8_bit = 0x0008,
    e16_bit = 0x0010,
    e32_bit = 0x0020,
    e64_bit = 0x0040
};

enum class image_usage : uint16_t 
{
    transfer_src_bit = 0x0001,
    transfer_dst_bit = 0x0002,
    sampled_bit = 0x0004,
    storage_bit = 0x0008,
    color_attachment_bit = 0x0010,
    depth_stencil_attachment_bit = 0x0020,
    transient_attachment_bit = 0x0040,
    input_attachment_bit = 0x0080,
};

enum class filter : uint8_t
{
    nearest,
    linear
};

enum class sampler_address_mode : uint8_t
{
    repeat,
    mirrored_repeat,
    clamp_to_edge,
    clamp_to_border
};

enum class buffer_type : uint8_t
{
    vertex,
    index,
    uniform,
};

enum class compare_op : uint8_t
{
    never,
    less,
    equal,
    less_or_equal,
    greater,
    not_equal,
    greater_or_equal,
    always
};

enum class sampler_border_color : uint8_t
{
    float_transparent_black,
    int_transparent_black,
    float_opaque_black,
    int_opaque_black,
    float_opaque_white,
    int_opaque_white
};

enum class vertex_input_rate : uint8_t
{
    vertex,
    instance
};

enum class uniform_type : uint8_t
{
    sampler,
    combined_image_sampler,
    sampled_image,
    storage_image,
    uniform_texel_buffer,
    storage_texel_buffer,
    uniform_buffer,
    storage_buffer,
    uniform_buffer_dynamic,
    storage_buffer_dynamic,
    input_attachment,
    max
};

enum class cull_mode : uint8_t
{
    none = 0x00,
    front_bit = 0x01,
    back_bit = 0x02,
    front_and_back = front_bit | back_bit
};

enum class front_face : uint8_t
{
    counter_clockwise,
    clockwise
};

enum class logic_op : uint8_t
{
    clear,
    op_and,
    and_reverse,
    copy,
    and_inverted,
    no_op,
    op_xor,
    op_or,
    nor,
    equivalent,
    invert,
    or_reverse,
    copy_inverted,
    or_inverted,
    nand,
    set
};

enum class dynamic_state : uint8_t
{
    line_width = 0x01,
    depth_bias = 0x02,
    blend_constants = 0x04,
    depth_bounds = 0x08,
    stencil_compare_mask = 0x10,
    stencil_write_mask = 0x20,
    stencil_reference = 0x40
};

enum class initial_action
{
    clear,
    keep,
    drop,
    ia_continue,
};

enum class final_action
{
    read,
    discard,
    fa_continue
};

class rendering_device : std::enable_shared_from_this<rendering_device>
{
public:

    // Shaders

    struct create_shader_params
    {
        shader_stage stage;
        std::string bytecode;
    };
    shader* create_shader(const create_shader_params& params);
    void free_shader(shader* shader);

    // Textures

    struct create_texture_params
    {
        format format;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint32_t mipmaps;
        image_type type;
        sample_counts samples;
        image_usage usage;
        uint32_t array_layers;
        // TODO: ctor
    };
    texture* create_texture(const create_texture_params& params, 
        std::vector<std::vector<uint8_t>> data = std::vector<std::vector<uint8_t>>());
    void free_texture(texture* texture);

    // Framebuffers
    struct attachment_format
    {
        format format;
        sample_counts samples;
        image_usage usage;
    };

    using framebuffer_format_id = uint64_t;
    framebuffer_format_id create_framebuffer_format(std::vector<attachment_format> formats);
    framebuffer* create_framebuffer(std::vector<texture*> texture_attachments, framebuffer_format_id* format_check = nullptr);
    void free_framebuffer(framebuffer* framebuffer);

    // Samplers, kinda TODO
    struct create_sampler_params
    {
        filter mag_filter;
        filter min_filter;
        filter mipmap_filter;
        sampler_address_mode address_mode_u;
        sampler_address_mode address_mode_v;
        sampler_address_mode address_mode_w;
        float mip_lod_bias;
        bool enable_ansiotropy;
        float max_ansiotropy;
        bool enable_compare;
        compare_op compare_op;
        float min_lod;
        float max_lod;
        sampler_border_color border_color;
        bool unnormalized_coords;

        // TODO: default ctor
    };
    sampler* create_sampler(const create_sampler_params& params);
    void free_sampler(sampler* sampler);

    // Buffers
    // TODO: free, custom allocation
    buffer* create_buffer(buffer_type type, const size_t size, const void* data);
    void update_buffer(const buffer const*, const size_t offset, const size_t size, const void const* data);
    void free_buffer(buffer* buffer);

    // Vertex format
    struct vertex_attribute
    {
        uint32_t location;
        uint32_t offset;
        format format;
        uint32_t stride;
        vertex_input_rate input_rate;
    };

    using vertex_format_id = uint64_t;
    vertex_format_id create_vertex_format(std::vector<vertex_attribute> attributes);
    // TODO: create_vertex_array()
    
    // Uniforms
    struct uniform_info
    {
        std::vector<shader_stage> stages;
        uniform_type type;
        uint32_t binding;
        
        using type_variant = std::variant<sampler*, texture*, buffer*>;
        std::vector<type_variant> data;
    };
    uniform_set* create_uniform_set(std::vector<uniform_info> uniform_infos, shader* shader);
    void free_uniform_set(uniform_set* uniform_set);

    // Pipeline
    // TODO: render_primitive

    struct pipeline_rasterization_state 
    {
        bool enable_depth_clamp;
        bool enable_rasterizer_discard;
        bool wireframe;
        cull_mode cull_mode;
        front_face front_face;
        
        // TODO: ctor
    };

    struct pipeline_multisample_state
    {
        sample_counts sample_count;
        bool enable_sample_shading;
        float min_sample_shading;
        std::vector<uint32_t> sample_mask;
        bool enable_alpha_to_coverage;
        bool enable_alpha_to_one;

        // TODO: ctor
    };

    struct pipeline_depth_stencil_state
    {
        bool enable_stencil_test;
        bool enable_depth_write;
        compare_op depth_compare_operator;
        bool enable_depth_range;
        float min_depth_range;
        float max_depth_range;
        bool enable_stencil;

        // TODO: ctor
        // TODO: stencil operation state
    };

    struct pipeline_color_blend_state
    {
        bool enable_logic_op;
        logic_op logic_op;
        
        // TODO: ctor
        // TODO: blend attachments
    };

    graphics_pipeline* create_graphics_pipeline(std::vector<shader*> shaders, framebuffer_format_id ffid, vertex_format_id vfid, const pipeline_rasterization_state& rasterization_state, 
        const pipeline_multisample_state& multisample_state, const pipeline_depth_stencil_state& depth_stencil_state, const pipeline_color_blend_state& color_blend_state, dynamic_state dynamic_states);

    // TODO: compute pipeline

    // TODO: command buffer

    using cmd_buf_id = uint64_t;
    struct begin_gfx_cmd_buf_params
    {
        framebuffer* frambuffer;
        initial_action initial_color_action;
        final_action final_color_action;
        initial_action initial_depth_action;
        final_action final_depth_action;
        
        using color = std::array<float, 4>;
        std::vector<color> clear_color_values = std::vector<color>();
        float clear_depth = 1.f;
        uint32_t clear_stencil = 0;
    };
   
    cmd_buf_id begin_gfx_cmd_buf(const begin_gfx_cmd_buf_params& params);
    void gfx_cmd_buf_bind_pipeline(cmd_buf_id id, graphics_pipeline* pipeline);
    void gfx_cmd_buf_bind_uniform_set(cmd_buf_id id, uniform_set* uniform_set, uint32_t index);
    void gfx_cmd_buf_bind_vertex_array(cmd_buf_id id, buffer* vertex_array);
    void gfx_cmd_buf_bind_index_array(cmd_buf_id id, buffer* index_array);
    void gfx_cmd_buf_draw(cmd_buf_id id, bool use_indices, uint32_t instances = 1);
    void gfx_cmd_buf_end(cmd_buf_id id);

    // TODO: compute lists

    DEFINE_SMART_PTR_METHODS(texture);
    DEFINE_SMART_PTR_METHODS(framebuffer);
    DEFINE_SMART_PTR_METHODS(sampler);
    DEFINE_SMART_PTR_METHODS(buffer);
    DEFINE_SMART_PTR_METHODS(uniform_set);

private:

    std::unique_ptr<impl_::device_data> device_data_ptr_;
};

}