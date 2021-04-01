#pragma once

#include <vector>
#include <array>
#include <string>
#include <variant>
#include <memory>
#include <functional>
#include <utility>

// TODO: fix unique ptr strat sometime, each unique ptr has 3 (!) ptrs, one to data, and two for shared_ptr to device
#define DEFINE_SMART_PTR_DELETER(resource_type) \
template<> \
struct deleter<resource_type> \
{ \
    deleter() = default; \
    deleter(std::shared_ptr<device> device_ptr) : device_ptr_(device_ptr) {} \
    void operator()(resource_type* ptr) { device_ptr_->free_ ## resource_type(ptr); } \
    std::shared_ptr<device> device_ptr_; \
};

#define DEFINE_SMART_PTR_METHODS(resource_type) \
template<typename... Ts> \
std::unique_ptr<resource_type, deleter<resource_type>> create_ ## resource_type ## _unique(Ts&&... args) \
{ \
    return std::unique_ptr<resource_type, deleter<resource_type>>( \
        create_ ## resource_type(std::forward<Ts>(args)...), deleter<resource_type>(shared_from_this())); \
} \
template<typename... Ts> \
std::shared_ptr<resource_type> create_ ## resource_type ## _shared(Ts... args) \
{ \
    return std::shared_ptr<resource_type>(create_ ## resource_type(std::forward<Ts>(args)...),  \
        [device = shared_from_this()](resource_type* ptr){device->free_ ## resource_type(ptr); }); \
}

#define DEFINE_SMART_PTR(resource_type) \
DEFINE_SMART_PTR_DELETER(resource_type) \
DEFINE_SMART_PTR_METHODS(resource_type)

// Based off godot's rhi: https://github.com/godotengine/godot/blob/master/servers/rendering/rendering_device.h

namespace xs
{
namespace rhi
{
namespace impl_
{
    struct context_gfx_data;
    struct context_plat_data;

    struct surface_gfx_data;
    struct surface_plat_data;

    struct device_data;

    struct shader;
    struct pipeline;
    struct texture;
    struct sampler;
    struct framebuffer;
    struct buffer;
    struct uniform_set;
}

using shader = impl_::shader;
using pipeline = impl_::pipeline;
using texture = impl_::texture;
using sampler = impl_::sampler;
using framebuffer = impl_::framebuffer;
using buffer = impl_::buffer;
using uniform_set = impl_::uniform_set;

using graphics_pipeline = pipeline;
using compute_pipeline = pipeline;

enum class shader_stage : std::uint8_t
{
	vertex,
	fragment,
    geometry,
	tesselation_control,
	tesselation_evaluation,
	compute
};

enum class format : std::uint8_t
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
    R64G64B64A64_sfloat,
    D32_sfloat,
};

enum class image_type : std::uint8_t
{
    e1D,
    e2D,
    e3D,
    cube,
    e1D_array,
    e2D_array,
    cube_array
};

enum class sample_counts : std::uint8_t
{
    e1_bit,
    e2_bit,
    e4_bit,
    e8_bit,
    e16_bit,
    e32_bit,
    e64_bit
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
    resolve_attachment_bit = 0x0100
};

enum class filter : std::uint8_t
{
    nearest,
    linear
};

enum class sampler_address_mode : std::uint8_t
{
    repeat,
    mirrored_repeat,
    clamp_to_edge,
    clamp_to_border
};

enum class buffer_type : std::uint8_t
{
    vertex,
    index,
    uniform,
    storage
};

enum class compare_op : std::uint8_t
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

enum class blend_factor : std::uint8_t
{
    zero,
    one,
    src_color,
    one_minus_src_color,
    dst_color,
    one_minus_dst_color,
    src_alpha,
    one_minus_src_alpha,
    dst_alpha,
    one_minus_dst_alpha,
    constant_color,
    one_minus_constant_color,
    constant_alpha,
    one_minus_constant_alpha,
    src_alpha_saturate,
    src1_color,
    one_minus_src1_color,
    src1_alpha,
    one_minus_src1_alpha,
};

enum class blend_op : std::uint8_t
{
    add,
    subtract,
    reverse_subtract,
    min,
    max,
};

enum class sampler_border_color : std::uint8_t
{
    float_transparent_black,
    int_transparent_black,
    float_opaque_black,
    int_opaque_black,
    float_opaque_white,
    int_opaque_white
};

enum class vertex_input_rate : std::uint8_t
{
    vertex,
    instance
};

enum class uniform_type : std::uint8_t
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

enum class cull_mode : std::uint8_t
{
    none = 0x00,
    front_bit = 0x01,
    back_bit = 0x02,
    front_and_back = front_bit | back_bit
};

enum class front_face : std::uint8_t
{
    counter_clockwise,
    clockwise
};

enum class logic_op : std::uint8_t
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

enum class dynamic_state : std::uint8_t
{
    none = 0x00,
    line_width = 0x01,
    depth_bias = 0x02,
    blend_constants = 0x04,
    depth_bounds = 0x08,
    stencil_compare_mask = 0x10,
    stencil_write_mask = 0x20,
    stencil_reference = 0x40,
};

enum class initial_action : std::uint8_t
{
    clear,
    keep,
    drop,
    ia_continue,
};

enum class final_action : std::uint8_t
{
    read,
    discard,
    fa_continue
};

enum class primitive_topology : std::uint8_t
{
    points,
    lines,
    lines_with_adjacency,
    line_strips,
    line_strips_with_adjacency,
    triangles,
    triangles_with_adjacency,
    triangle_strips,
    triangle_strips_with_adjacency,
    triangle_strips_with_restart_index,
    tesselation_patch,
};

enum class index_type : std::uint8_t
{
    uint16,
    uint32
};

class context
{
public:
    context(void* params);
    ~context();

    void log(std::string_view str);

    friend class surface;
    friend class device;
private:
    std::unique_ptr<impl_::context_gfx_data> context_gfx_data_ptr_;
    std::unique_ptr<impl_::context_plat_data> context_plat_data_ptr_;
};

class surface
{
public:
    surface(const context& ctx, std::wstring name, std::uint32_t width, std::uint32_t height);
    ~surface();

    inline std::uint32_t get_width() const { return width_; }
    inline std::uint32_t get_height() const { return height_; }

    void add_scroll_callback(std::function<void(std::int32_t)> cb) { scroll_callbacks_.push_back(std::move(cb)); }
    void add_key_callback(std::function<void(std::uint32_t)> cb) { key_callbacks_.push_back(std::move(cb)); }

    std::vector<std::function<void(std::int32_t)>> scroll_callbacks_;
    std::vector<std::function<void(std::uint32_t)>> key_callbacks_;

    friend class device;
private:
    std::uint32_t width_;
    std::uint32_t height_;

    std::unique_ptr<impl_::surface_gfx_data> surface_gfx_data_ptr_; // Graphics API specific data
    std::unique_ptr<impl_::surface_plat_data> surface_plat_data_ptr_; // platform specific data
};

class device : public std::enable_shared_from_this<device>
{
public:

    device(const context& ctx, surface* surface = nullptr);
    ~device();

    format get_surface_format() const;
    std::uint32_t get_frame() const;
    void next_frame();
    void swap_buffers();

    // Shaders

    struct create_shader_params
    {
        shader_stage stage;
        std::string bytecode;
    };
    [[nodiscard]]
    shader* create_shader(const create_shader_params& params);
    void free_shader(shader* shader);

    // Textures

    struct create_texture_params
    {
        format format;
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t depth;
        std::uint32_t mipmaps;
        image_type type;
        sample_counts samples;
        image_usage usage;
        std::uint32_t array_layers;
        // TODO: ctor
    };
    [[nodiscard]]
    texture* create_texture(const create_texture_params& params, 
        std::vector<std::vector<std::uint8_t>> data = std::vector<std::vector<std::uint8_t>>());
    void free_texture(texture* texture);

    // Framebuffers THESE DO NOT WORK rn
    struct attachment_format
    {
        format format;
        sample_counts samples;
        image_usage usage;
    };

    using framebuffer_format_id = uint64_t;
    static constexpr framebuffer_format_id surface_ffid = 0;
    framebuffer_format_id create_framebuffer_format(std::vector<attachment_format> formats);
    [[nodiscard]]
    framebuffer* create_framebuffer(std::vector<texture*> texture_attachments);
    void free_framebuffer(framebuffer* framebuffer);

    framebuffer* get_surface_framebuffer() const;
    framebuffer_format_id get_framebuffer_format(framebuffer* framebuffer) const;

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
    [[nodiscard]]
    sampler* create_sampler(const create_sampler_params& params);
    void free_sampler(sampler* sampler);

    // Buffers
    // TODO: free, custom allocation
    [[nodiscard]]
    buffer* create_buffer(buffer_type type, const std::size_t size, const void* data);
    void free_buffer(buffer* buffer);

    //void update_buffer(buffer*, const std::size_t offset, const std::size_t size, const void const* data);
    
    template<typename T>
    class scoped_mmap
    {
    public:
        template<typename Ty>
        friend class scoped_mmap;

        using element_type = T;
        using value_type = std::remove_cv_t<element_type>;
        using difference_type = ptrdiff_t;
        using pointer = element_type*;
        using const_pointer = const element_type*;
        using reference = element_type&;
        using const_reference = const element_type&;
        using iterator = element_type*;
        using reverse_iterator = std::reverse_iterator<iterator>;

        scoped_mmap(T* data, const std::size_t size, const device* owner, const buffer* buf) : data_(data), size_(size), owner_(owner), buf_(buf) {}
        ~scoped_mmap() { if (buf_ && owner_) { owner_->unmap_buffer_mem(buf_); } }
        scoped_mmap() noexcept = default;
        scoped_mmap(const scoped_mmap&) = delete;
        template<typename Ty>
        scoped_mmap(scoped_mmap<Ty>&& other) noexcept : scoped_mmap<T>(other.data<T>(), other.size(), other.owner_, other.buf_) { other.buf_ = nullptr; }
        
        scoped_mmap& operator=(const scoped_mmap&) = delete;
        template<typename Ty>
        inline scoped_mmap& operator=(scoped_mmap<Ty>&& v) noexcept 
        { // TODO: copy&swap? ugly
            data_ = v.data<T>();
            size_ = v.size_;
            owner_ = v.owner_;
            buf_ = v.buf_;
            v.buf_ = nullptr;
            return *this;
        };

        inline size_t size() const noexcept { return size_; }
        inline bool empty() const noexcept { return !data_ || size_ == 0; }
        template<typename Ty = T> 
        inline Ty* data() const noexcept { return reinterpret_cast<Ty*>(data_); }

        inline iterator begin() const noexcept { return iterator(data()); }
        inline iterator end() const noexcept { return iterator(data() + size_); }
        inline reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }
        inline reverse_iterator rend() const noexcept { return reverse_iterator(begin()); }

        inline reference operator[](const size_t i) const { return data()[i]; }
        template<typename Ty = T> 
        inline reference at(const size_t i) const { return data<Ty>()[i]; }
        constexpr reference front() const { return data()[0]; }
        constexpr reference back() const { return std::next(data(), size_ - 1); }
    private:
        T* data_;
        std::size_t size_;
        const device* owner_;
        const buffer* buf_;
    };
    
    template<typename T>
    scoped_mmap<T> map_buffer(buffer* buffer, const std::size_t offset, const std::size_t size) const
    {
        scoped_mmap<std::byte> byte_mmap = map_buffer<std::byte>(buffer, offset * sizeof(T), size * sizeof(T));
        return scoped_mmap<T>(std::move(byte_mmap));
    }

    template<>
    scoped_mmap<std::byte> map_buffer(buffer* buffer, const std::size_t offset, const std::size_t size) const;

    // Vertex format
    struct vertex_attribute
    {
        std::uint32_t location;
        std::uint32_t offset;
        format format;
        std::uint32_t stride;
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
        std::uint32_t binding;
    };
    
    using uniform_set_format_id = std::uint64_t;
    uniform_set_format_id create_uniform_set_format(const std::vector<uniform_info>& uniform_infos);

    [[nodiscard]]
    uniform_set* create_uniform_set(const uniform_set_format_id format_id);
    void free_uniform_set(uniform_set* uniform_set);

    using type_variant = std::variant<sampler*, texture*, buffer*>;
    struct uniform_set_write
    {
        std::uint32_t binding;
        std::vector<type_variant> data;
    };
    void update_uniform_set(uniform_set* set, const std::vector<uniform_set_write>& writes);

    // Pipeline
    // TODO: render_primitive

    struct pipeline_rasterization_state 
    {
        bool enable_depth_clamp = false;
        bool enable_rasterizer_discard = false;
        bool wireframe = false;
        cull_mode cull_mode = cull_mode::none;
        front_face front_face = front_face::clockwise;
    };

    struct pipeline_multisample_state
    {
        sample_counts sample_count = sample_counts::e1_bit;
        bool enable_sample_shading = false;
        float min_sample_shading = 0.f;
        std::vector<std::uint32_t> sample_mask = {};
        bool enable_alpha_to_coverage = false;
        bool enable_alpha_to_one = false;
    };

    struct pipeline_depth_stencil_state
    {
        bool enable_depth_test = true;
        bool enable_stencil_test = false;
        bool enable_depth_write = true;
        compare_op depth_compare_operator = compare_op::less;
        bool enable_depth_range = false;
        float min_depth_range = 0.f;
        float max_depth_range = 1.f;
    };

    struct color_blend_attachment_state
    {
        bool blend_enable = false;

        struct blend_eqn
        {
            blend_factor src;
            blend_op op;
            blend_factor dst;
        };

        blend_eqn color_eqn = { blend_factor::src_alpha, blend_op::add, blend_factor::one_minus_src_alpha };
        blend_eqn alpha_eqn = { blend_factor::one, blend_op::add, blend_factor::zero };
    };

    struct pipeline_color_blend_state
    {
        bool enable_logic_op = false;
        logic_op logic_op = logic_op::copy;
        std::vector<color_blend_attachment_state> blend_attachments = { color_blend_attachment_state() };
    };

    [[nodiscard]]
    pipeline* create_graphics_pipeline(std::vector<shader*> shaders, const std::vector<uniform_set_format_id>& ufids, framebuffer_format_id ffid, vertex_format_id vfid, primitive_topology topology, const pipeline_rasterization_state& rasterization_state,
        const pipeline_multisample_state& multisample_state, const pipeline_depth_stencil_state& depth_stencil_state, const pipeline_color_blend_state& color_blend_state, dynamic_state dynamic_states);
    void free_graphics_pipeline(pipeline* pipeline);

    pipeline* create_compute_pipeline(shader* shader, const std::vector<uniform_set_format_id>& ufids);
    void free_compute_pipeline(pipeline* pipeline);

    using cmd_buf_id = uint64_t;
    using color = std::array<float, 4>;
    cmd_buf_id begin_cmd_buf();
    void cmd_buf_bind_uniform_set(cmd_buf_id id, uniform_set* uniform_set, pipeline* pipeline, std::uint32_t index);
    void cmd_buf_end(cmd_buf_id id);

    struct bind_framebuffer_params
    {
        std::vector<color> clear_color_values = std::vector<color>();
        bool clear_depth_stencil = true;
        float clear_depth_value = 1.f;
        std::uint32_t clear_stencil_value = 0;
    };
    void cmd_buf_bind_framebuffer(cmd_buf_id id, framebuffer* fb, const bind_framebuffer_params& params = bind_framebuffer_params());
    void cmd_buf_bind_gfx_pipeline(cmd_buf_id id, pipeline* pipeline);
    void cmd_buf_bind_vertex_array(cmd_buf_id id, const std::vector<buffer*>& vertex_array);
    void cmd_buf_bind_index_array(cmd_buf_id id, buffer* index_array, index_type index_type);
    void cmd_buf_draw(cmd_buf_id id, bool use_indices, std::uint32_t element_count, std::uint32_t instances = 1);

    void cmd_buf_bind_cpu_pipeline(cmd_buf_id id, pipeline* pipeline);
    void cmd_buf_dispatch(cmd_buf_id id, std::uint32_t x_groups, std::uint32_t y_groups, std::uint32_t z_groups);

    template<typename T> 
    struct deleter {};
    DEFINE_SMART_PTR(shader);
    DEFINE_SMART_PTR(texture);
    DEFINE_SMART_PTR(framebuffer);
    DEFINE_SMART_PTR(sampler);
    DEFINE_SMART_PTR(buffer);
    DEFINE_SMART_PTR(uniform_set);
    DEFINE_SMART_PTR(graphics_pipeline);
    DEFINE_SMART_PTR_METHODS(compute_pipeline);

    template<typename T>
    using ptr = std::unique_ptr<T, deleter<T>>;

private:

    void unmap_buffer_mem(const buffer* buffer) const;

    std::unique_ptr<impl_::device_data> device_data_ptr_;
};

} // namespace rhi
} // namespace xs