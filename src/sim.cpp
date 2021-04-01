#include "sim.hpp"

#include <cassert>
#include <algorithm>
#include <execution>

namespace xs
{

cloth::cloth(const sim::size2_t& resolution, const Eigen::Vector3f& low, const Eigen::Vector3f& high, const std::vector<sim::size2_t>& fixed_verts) :
    forces_(resolution[0] * resolution[1], Eigen::Vector3f(0.f, 0.f, 0.f)),
    spring_dampers_(),
    velocities_(resolution[0] * resolution[1], Eigen::Vector3f(0.f, 0.f, 0.f)),
    verts_(resolution[0] * resolution[1]),
    tri_norms_(),
    norms_(resolution[0] * resolution[1], Eigen::Vector3f(0.f, 0.f, 1.f)),
    inds_(),
    fixed_(),
    d_vert_pos_view_(),
    d_vert_norm_view_(),
    d_vert_pos_buf_(),
    d_vert_norm_buf_(),
    d_inds_buf_(),
    v_wind_(Eigen::Vector3f(0.f, 0.f, 0.f)),
    k_spring_(1000.f),
    k_damping_(1.f)
{
	assert(resolution[0] == resolution[1] && "non-squares not supported yet");
    assert(resolution[0] >= 2 && resolution[1] >= 2); // actually they must be powers of 2

    const std::size_t n = resolution[0];
    const Eigen::Vector3f diff = (high - low) / float(n - 1);
    for (std::size_t y = 0; y < resolution[1]; y++)
    {
        for (std::size_t x = 0; x < resolution[0]; x++)
        {
            const std::size_t i = y * n + x;
            const Eigen::Vector3f v = low + diff.cwiseProduct(Eigen::Vector3f(float(x), float(y), 0.f));
            verts_[i] = v;
        }
    }

    const std::size_t squares = resolution[0] * resolution[1];
    static constexpr std::size_t inds_per_square = 2;
    static constexpr std::size_t constraints_per_square = 4;
    inds_.reserve(squares * inds_per_square);
    tri_norms_.resize(squares * inds_per_square, Eigen::Vector4f(0.f, 0.f, 1.f, 0.f));
    spring_dampers_.reserve(squares * constraints_per_square);
    for (std::size_t y = 0; y < resolution[1] - 1; y++)
    {
        for (std::size_t x = 0; x < resolution[0] - 1; x++)
        {
            const std::uint32_t i00 = y * n + x, i01 = y * n + x + 1, i10 = (y + 1) * n + x, i11 = (y + 1) * n + x + 1;
            
            inds_.push_back({ std::uint32_t(i00), std::uint32_t(i10), std::uint32_t(i11) });
            inds_.push_back({ std::uint32_t(i00), std::uint32_t(i11), std::uint32_t(i01) });

            const float l0 = (verts_[i00] - verts_[i10]).norm(), l1 = (verts_[i00] - verts_[i11]).norm(),
                l2 = (verts_[i10] - verts_[i01]).norm(), l3 = (verts_[i00] - verts_[i01]).norm();
            spring_dampers_.push_back({ .i0 = i00, .i1 = i10, .dist = l0 });
            spring_dampers_.push_back({ .i0 = i00, .i1 = i11, .dist = l1 });
            spring_dampers_.push_back({ .i0 = i10, .i1 = i01, .dist = l2 });
            spring_dampers_.push_back({ .i0 = i00, .i1 = i01, .dist = l3 });
        }
    }

    for (std::size_t y = 0; y < resolution[1] - 1; y++)
    {
        const sim::size2_t i0 = { resolution[0] - 1, y }, i1 = { resolution[0] - 1, y + 1 };
        const std::size_t d0 = i0[1] * n + i0[0], d1 = i1[1] * n + i1[0];
        const float l = (verts_[d0] - verts_[d1]).norm();
        spring_dampers_.push_back({ .i0 = d0, .i1 = d1, .dist = l });
    }

    for (std::size_t x = 0; x < resolution[0] - 1; x++)
    {
        const sim::size2_t i0 = { resolution[0] - x - 2, resolution[1] - 1 }, i1 = { resolution[0] - x - 1, resolution[1] - 1 };
        const std::size_t d0 = i0[1] * n + i0[0], d1 = i1[1] * n + i1[0];
        const float l = (verts_[d0] - verts_[d1]).norm();
        spring_dampers_.push_back({ .i0 = d0, .i1 = d1, .dist = l });
    }    

    velocities_ = std::vector<Eigen::Vector3f>(verts_.size(), mth::vec3f_zeros());

    std::sort(std::begin(inds_), std::end(inds_), [](const sim::size3_t& t0, const sim::size3_t& t1) { return t0[0] < t1[0]; });
    std::sort(std::begin(spring_dampers_), std::end(spring_dampers_), 
        [](const spring_damper& sd0, const spring_damper& sd1) { return sd0.i0 < sd1.i0; }
    );

    for (const sim::size2_t& i : fixed_verts)
    {
        fixed_.push_back(i[1] * n + i[0]);
    }
}

void cloth::update(float dt)
{
    static constexpr float mass = .02f;
    static constexpr float inv_mass = 1.f / mass;
    for (Eigen::Vector3f& f : forces_)
    {
        f += Eigen::Vector3f(0.f, 9.81f, 0.f) * mass;
    }

    for (std::size_t i = 0; i < spring_dampers_.size(); i++)
    {
        const std::size_t i0 = spring_dampers_[i].i0, i1 = spring_dampers_[i].i1;
        const Eigen::Vector3f diff = verts_[i1] - verts_[i0];
        const float l = diff.norm();
        const Eigen::Vector3f e = diff / l;
        const float v_close = (velocities_[i0] - velocities_[i1]).dot(e);
        const float f = -(spring_dampers_[i].dist - l) * k_spring_ - v_close * k_damping_;
        const Eigen::Vector3f f_0 = e * f;
        forces_[i0] += f_0;
        forces_[i1] += -f_0;
    }

    for (std::size_t i = 0; i < inds_.size(); i += 3)
    {
        const std::size_t i0 = inds_[i][0], i1 = inds_[i][1], i2 = inds_[i][2];
        const Eigen::Vector3f r0 = verts_[i0], r1 = verts_[i1], r2 = verts_[i2];
        const Eigen::Vector3f v0 = velocities_[i0], v1 = velocities_[i1], v2 = velocities_[i2];
        
        static constexpr float one_third = 1.f / 3.f;
        static constexpr float rho = 1.225f; // fluid density of air
        static constexpr float c_drag = 1.28f; // drag coeff
        const Eigen::Vector3f v_surface = (v0 + v1 + v2) * one_third;
        const Eigen::Vector3f v = v_surface - v_wind_;
        const float v_mag = v.norm();
        const Eigen::Vector3f v_norm = v / v_mag;

        const Eigen::Vector3f n = Eigen::Vector3f(tri_norms_[i].x(), tri_norms_[i].y(), tri_norms_[i].z());
        const float l = tri_norms_[i].w();
        const float a0 = l * .5f;
        const float a = a0 * v_norm.dot(n);
        const Eigen::Vector3f f_aero = n * -.5f * rho * v_mag * v_mag * c_drag * a;
        forces_[i0] += f_aero;
        forces_[i1] += f_aero;
        forces_[i2] += f_aero;
    }

    for (std::size_t i = 0; i < forces_.size(); i++)
    {
        if (std::find(std::begin(fixed_), std::end(fixed_), i) == std::end(fixed_)) [[likely]]
        {
            velocities_[i] += forces_[i] * dt * inv_mass;
            verts_[i] += velocities_[i] * dt;
        }

        if (verts_[i].y() > 20.1f) // ground plane collision
        {
            static constexpr float ep = .05f;
            static constexpr float mu_static = .75f;
            const float v_norm_mag = velocities_[i].y(); // v dot up
            const Eigen::Vector3f v_tan = -velocities_[i] + Eigen::Vector3f(0.f, v_norm_mag, 0.f);
            const float jy = -(1.f + ep) * v_norm_mag * mass;
            const Eigen::Vector3f f_static = v_tan * mu_static;
            const Eigen::Vector3f f_collision = Eigen::Vector3f(0.f, jy / dt - 9.81f, 0.f) + f_static;
            forces_[i] = f_collision;
        }
        else
        {
            forces_[i] = Eigen::Vector3f(0.f, 0.f, 0.f);
        }
    }

    for (std::size_t i = 0; i < norms_.size(); i++)
    {
        norms_[i] = mth::vec3f_zeros();
    }

    for (std::size_t i = 0; i < inds_.size(); i++)
    {
        const std::size_t i0 = inds_[i][0], i1 = inds_[i][1], i2 = inds_[i][2];
        const Eigen::Vector3f r0 = verts_[i0], r1 = verts_[i1], r2 = verts_[i2];
        const Eigen::Vector3f cross = (r1 - r0).cross(r2 - r0);
        const float l = cross.norm();
        const Eigen::Vector3f n = cross / l;
        tri_norms_[i] = Eigen::Vector4f(n.x(), n.y(), n.z(), l);
        norms_[i0] += n;
        norms_[i1] += n;
        norms_[i2] += n;
    }
}

draw_item cloth::draw_item(rhi::device* device, rhi::buffer* d_mvp_buf)
{
    const render_pass& simple_pass = render_pass_registry::get().pass("simple");

    d_inds_buf_ = device->create_buffer_unique(rhi::buffer_type::index, inds_.size() * sizeof(sim::size3_t), inds_.data());
    d_vert_pos_buf_ = device->create_buffer_unique(rhi::buffer_type::vertex, verts_.size() * sizeof(Eigen::Vector3f), verts_.data());
    d_vert_norm_buf_ = device->create_buffer_unique(rhi::buffer_type::vertex, norms_.size() * sizeof(Eigen::Vector3f), norms_.data());
   
    d_vert_pos_view_ = std::move(device->map_buffer<Eigen::Vector3f>(d_vert_pos_buf_.get(), 0, verts_.size()));
    d_vert_norm_view_ = std::move(device->map_buffer<Eigen::Vector3f>(d_vert_norm_buf_.get(), 0, norms_.size()));

    d_mvp_uniforms_ = simple_pass.uniform_set_builder(device, 0)
        .uniform("ubo", { d_mvp_buf })
        .produce();

    return simple_pass.draw_item_builder()
        .elem_count(inds_.size() * 3)
        .vertex_buffers({ d_vert_pos_buf_.get(), d_vert_norm_buf_.get() })
        .index_buffer(d_inds_buf_.get())
        .uniform_sets({ {0, d_mvp_uniforms_.get()} })
        .update([this](rhi::device*) { 
                std::copy(std::begin(verts_), std::end(verts_), std::begin(d_vert_pos_view_));
                std::copy(std::begin(norms_), std::end(norms_), std::begin(d_vert_norm_view_)); 
        })
        .produce_draw();
}

mpm_sim::mpm_sim(const sim::range3_t& domain, const sim::size3_t& domain_resolution, const sim::range3_t& block, const std::size_t num_particles)
{
    grid_resolution_ = domain_resolution;
    const Eigen::Vector3f block_dim = block[1] - block[0];
    points_.reserve(num_particles);
    particles_.reserve(num_particles);
    for (std::size_t i = 0; i < num_particles; i++)
    {
        const Eigen::Vector3f rand3 = Eigen::Vector3f(rand_.nextFloat(), rand_.nextFloat(), rand_.nextFloat());
        const Eigen::Vector3f point = block[0] + block_dim.cwiseProduct(rand3);
        points_.push_back(point);
        
        particles_.emplace_back(Eigen::Vector3f(0.f, 0.f, 0.f), Eigen::Matrix4f::Identity());
    }

    const std::size_t grid_size = domain_resolution[0] * domain_resolution[1] * domain_resolution[2];
    grid_ = std::vector<node>(grid_size, node{ .v = Eigen::Vector3f::Zero(), .m = 0.f });
}

void mpm_sim::register_passes(renderer* renderer)
{
    rhi::device::create_texture_params grid_tex_params = {
        .format = rhi::format::R16G16B16A16_sfloat,
        .width = grid_resolution_[0],
        .height = grid_resolution_[1],
        .depth = 1,
        .mipmaps = 1,
        .type = rhi::image_type::e2D_array,
        .samples = rhi::sample_counts::e1_bit,
        .usage = rhi::image_usage::color_attachment_bit,
        .array_layers = 100
    };

    std::uint8_t* grid_begin = reinterpret_cast<std::uint8_t*>(grid_.data());
    std::uint8_t* grid_end = std::next(grid_begin, grid_.size() * sizeof(node));
    std::vector<std::uint8_t> grid_data = std::vector<std::uint8_t>(grid_begin, grid_end);
    d_grid_tex_ = renderer->device()->create_texture_unique(grid_tex_params, std::vector<std::vector<std::uint8_t>>{ grid_data });
    d_grid_fb_ = renderer->device()->create_framebuffer_unique(std::vector<rhi::texture*>{d_grid_tex_.get()});
    const rhi::device::framebuffer_format_id grid_ffid = renderer->device()->get_framebuffer_format(d_grid_fb_.get());

    renderer::stage_params rasterize_vs = {
        .stage = xs::rhi::shader_stage::vertex,
        .filename = "../shaders/spirv/vert_particle.spv"
    };

    renderer::stage_params rasterize_gs = {
        .stage = xs::rhi::shader_stage::geometry,
        .filename = "../shaders/spirv/geom_particle.spv"
    };

    renderer::stage_params rasterize_fs = {
        .stage = xs::rhi::shader_stage::fragment,
        .filename = "../shaders/spirv/frag_particle.spv"
    };

    rhi::device::pipeline_depth_stencil_state depth_stencil_state = {
        .enable_depth_test = false,
        .enable_stencil_test = false,
        .enable_depth_write = false,
        .enable_depth_range = false
    };

    rhi::device::color_blend_attachment_state grid_blend_state = {
        .blend_enable = true,
        .color_eqn = {rhi::blend_factor::one, rhi::blend_op::add, rhi::blend_factor::one},
        .alpha_eqn = {rhi::blend_factor::one, rhi::blend_op::add, rhi::blend_factor::one}
    };

    rhi::device::pipeline_color_blend_state color_blend_state = {
        .enable_logic_op = false,
        .blend_attachments = { grid_blend_state }
    };

    render_pass rasterize_particles_pass = renderer->create_graphics_pass({ rasterize_vs, rasterize_gs, rasterize_fs }, rhi::primitive_topology::points, d_grid_fb_.get(), //std::optional<rhi::framebuffer*>(),
        rhi::device::pipeline_rasterization_state(), rhi::device::pipeline_multisample_state(), depth_stencil_state,
        color_blend_state);

    render_pass_registry::get().add("rasterize_particles", std::move(rasterize_particles_pass));
}

std::vector<draw_item> mpm_sim::draw_items(rhi::device* device)
{
    const render_pass& rasterize_particles_pass = render_pass_registry::get().pass("rasterize_particles");

    d_verts_buf_ = device->create_buffer_unique(rhi::buffer_type::vertex, points_.size() * sizeof(Eigen::Vector3f), points_.data());
    Eigen::Matrix4f temp_id_mat = Eigen::Matrix4f::Identity();
    d_m_buf_ = device->create_buffer_unique(rhi::buffer_type::uniform, sizeof(Eigen::Matrix4f), &temp_id_mat);

    std::int32_t z_scale = grid_resolution_[2];
    d_grid_metrics_buf_ = device->create_buffer_unique(rhi::buffer_type::uniform, sizeof(std::int32_t), &z_scale);

    draw_item rasterize_particles_item = rasterize_particles_pass.draw_item_builder()
        .elem_count(points_.size())
        .vertex_buffers({ d_verts_buf_.get() })
        .produce_draw();

    return { std::move(rasterize_particles_item) };
}

namespace sim
{
    spatial_hash_table::spatial_hash_table(const std::vector<particle>& elements, float cell_size) :
        inv_cell_size_(mth::vec3f_replicate(1.f/cell_size)),
        sorted_elements_(elements),
        grid_table_(elements.size())
    {
        for (particle& p : sorted_elements_)
        {
            const size3_t ipos = to_uint_space(p.pos);
            p.morton = morton_encode(ipos[0], ipos[1], ipos[2]);
        }

        std::sort(std::begin(sorted_elements_), std::end(sorted_elements_), [](const particle& a, const particle& b) { return a.morton < b.morton; });
        grid_table_.reserve(sorted_elements_.size());
        for (size_t i = sorted_elements_.size(); i-- > 0;) // can't set i to a potentially negative number in init, so...
        {
            const uint64_t hash = hash_coord(sorted_elements_[i].pos);
            grid_table_[hash] = i;
        }
    }

    void spatial_hash_table::update()
    {
        grid_table_.clear();

#pragma omp parallel for
        for (std::int64_t i = 0; i < sorted_elements_.size(); i++)
        {
            particle& p = sorted_elements_[i];
            const size3_t ipos = to_uint_space(p.pos);
            p.morton = morton_encode(ipos[0], ipos[1], ipos[2]);
        }

        std::sort(std::execution::par, std::begin(sorted_elements_), std::end(sorted_elements_), [](const particle& a, const particle& b) { return a.morton < b.morton; });

        for (std::int64_t i = 0; i < sorted_elements_.size(); i++)
        {
            const std::size_t reverse_i = sorted_elements_.size() - i - 1;
            const uint64_t hash = hash_coord(sorted_elements_[reverse_i].pos);
            grid_table_[hash] = reverse_i;
        }
    }

    std::vector<particle> spatial_hash_table::get_neighbors(const particle& elem) const
    {
        // TODO: is this actually faster?
        const static size3_t idx_offsets[] = {
            {-1, -1, -1}, {0, -1, -1}, {1, -1, -1},
            {-1,  0, -1}, {0,  0, -1}, {1,  0, -1},
            {-1,  1, -1}, {0,  1, -1}, {1,  1, -1},
            {-1, -1,  0}, {0, -1,  0}, {1, -1,  0},
            {-1,  0,  0}, {0,  0,  0}, {1,  0,  0},
            {-1,  1,  0}, {0,  1,  0}, {1,  1,  0},
            {-1, -1,  1}, {0, -1,  1}, {1, -1,  1},
            {-1,  0,  1}, {0,  0,  1}, {1,  0,  1},
            {-1,  1,  1}, {0,  1,  1}, {1,  1,  1}
        };

        std::vector<particle> neighbors;
        neighbors.reserve(32);

        for (uint8_t idx_offset = 0; idx_offset < 27; idx_offset++)
        {
            const uint32_t hash = hash_coord(elem.pos, idx_offsets[idx_offset]);
            const auto cell_itr = grid_table_.find(hash);
            if (cell_itr == std::end(grid_table_))
            {
                continue;
            }

            static constexpr uint64_t max_morton_diff = 1;

            size_t particle_idx = cell_itr->second;

            const uint64_t morton = sorted_elements_[particle_idx].morton;
            do
            {
                if ((sorted_elements_[particle_idx].pos - elem.pos).norm() > 1e-6f)
                {
                    neighbors.push_back(sorted_elements_[particle_idx]);
                }
                particle_idx++;
            } while (particle_idx < sorted_elements_.size() && sorted_elements_[particle_idx].morton == morton);
        }

        return neighbors;
    }
}

sph_sim::sph_sim(const sim::range3_t& block, const std::size_t num_particles, const float h, const float p0, const float k) :
    grid_(),
    kernel_(),
    inv_h_(1.f / h),
    inv_h_d_(sim::pow<3>(1.f / h)),
    inv_h_d1_(sim::pow<4>(1.f / h)),
    h2_(h * h),
    inv_p0_(1.f / p0),
    k_(k),
    mass_(sim::pow<3>(h) * p0),
    block_(block)
{
    mth::pcg32 rand;
    const Eigen::Vector3f block_dim = block[1] - block[0];
    std::vector<sim::particle> particles;
    particles.reserve(num_particles);
    for (std::size_t i = 0; i < num_particles; i++)
    {
        const Eigen::Vector3f rand3 = Eigen::Vector3f(rand.nextFloat(), rand.nextFloat(), rand.nextFloat());
        const Eigen::Vector3f point = block[0] + block_dim.cwiseProduct(rand3);
        particles.emplace_back(point, Eigen::Vector3f::Zero(), 0, 0.f, 0.f);
    }

    grid_ = sim::spatial_hash_table(std::move(particles), h * 2.f);

    float itr = 0.f;
    for (size_t i = 0; i < kernel_.size(); i++, itr += step)
    {
        kernel_[i] = sim::cubic_kernel(itr);
        dkernel_[i] = sim::dcubic_kernel(itr);
    }
}

void sph_sim::update(float dt)
{
    float v_max = 0.f;

#pragma omp parallel for
    for (std::int64_t i = 0; i < grid_.size(); i++)
    {
        sim::particle& cur_particle = grid_[i];
        std::vector<sim::particle> neighbors = grid_.get_neighbors(cur_particle);

        float density = 0.f;
        for (const sim::particle& neighbor : neighbors)
        {
            const float f_q = sample_kernel_3d((cur_particle.pos - neighbor.pos) * inv_h_);
            const float Wij = inv_h_d_ * f_q;

            density += mass_ * Wij;
        }

        density += mass_ * (2.f / 3.f);

        cur_particle.density = density;
        const float pressure = k_ * (sim::pow<7>(density * inv_p0_) - 1.f);
        cur_particle.pressure = pressure;

        const float v_mag = cur_particle.vel.norm();
#pragma omp critical
        v_max = std::max(v_max, v_mag);
    }
    const float v_cap = 1.f / (dt * inv_h_);

#pragma omp parallel for
    for (std::int64_t i = 0; i < grid_.size(); i++)
    {
        sim::particle& cur_particle = grid_[i];
        std::vector<sim::particle> neighbors = grid_.get_neighbors(cur_particle);

        const float density = cur_particle.density;
        const float inv_density2_i = 1.f / (density * density);

        Eigen::Vector3f del_pressure = mth::vec3f_zeros();
        Eigen::Vector3f del2_velocity = mth::vec3f_zeros();
        for (const sim::particle& neighbor : neighbors)
        {
            const float inv_density_j = 1.f / neighbor.density;
            const float inv_density2_j = inv_density_j * inv_density_j;
            const Eigen::Vector3f dpos = neighbor.pos - cur_particle.pos;

            const Eigen::Vector3f dWij = sample_dkernel_3d(dpos * inv_h_) * inv_h_d1_;
            const Eigen::Vector3f dpressure = dWij * (cur_particle.pressure * inv_density2_i + neighbor.pressure * inv_density2_j);
            del_pressure = del_pressure + dpressure;

            const Eigen::Vector3f dvel = (-(cur_particle.vel - neighbor.vel).cwiseProduct(dpos.cwiseProduct(dWij)) * inv_density_j) / (dpos.cwiseProduct(dpos) + mth::vec3f_replicate(.01f * h2_));
            del2_velocity = del2_velocity + dvel;

            if (std::isnan(del_pressure.x()) || std::isnan(del_pressure.y()) || std::isnan(del_pressure.z())) __debugbreak();
        }

        del_pressure = del_pressure * mass_ * cur_particle.density;

        const Eigen::Vector3f pressure_force = std::abs(density) < 0.00001f ? 
            mth::vec3f_replicate(0.f) : del_pressure * (-mass_ / density);

        static constexpr float viscosity = .001f;
        const Eigen::Vector3f friction_force = del2_velocity * 2.f * mass_ * mass_ * viscosity;

        // conventional standard value
        static constexpr float gravity_acc_scalar = 9.80665f;
        static const Eigen::Vector3f gravity_acc = Eigen::Vector3f(0.f, gravity_acc_scalar, 0.f);
        const Eigen::Vector3f gravity_force = gravity_acc * mass_;

        static constexpr float cube_dim = 3.f;
        static constexpr float k_collision = 1000000.f;
        //if (cur_particle.pos.y() > ground_height) __debugbreak();
        const float collision_force_ground = std::max(0.f, cur_particle.pos.y() - cube_dim) * k_collision;
        const float collision_force_x_wall = (std::max(0.f, cur_particle.pos.x() - cube_dim) + std::min(0.f, cur_particle.pos.x() + cube_dim))* k_collision;
        const float collision_force_z_wall = (std::max(0.f, cur_particle.pos.z() - cube_dim) + std::min(0.f, cur_particle.pos.z() + cube_dim)) * k_collision;
        const Eigen::Vector3f collision_force = Eigen::Vector3f(-collision_force_x_wall, -collision_force_ground, -collision_force_z_wall);

        //dt = .4f * 1.f / (inv_h_ * v_max);
        const Eigen::Vector3f force = pressure_force + gravity_force + friction_force + collision_force; // pressure_force + friction_force + gravity_force + collision_force;
        cur_particle.vel = cur_particle.vel + force * dt / mass_;
        cur_particle.pos = cur_particle.pos + cur_particle.vel * dt;

        if (std::isnan(cur_particle.pos.x()) || std::isnan(cur_particle.pos.y()) || std::isnan(cur_particle.pos.z())) __debugbreak();
    }

    grid_.update();
}

draw_item sph_sim::draw_item(rhi::device* device, rhi::buffer* d_mvp_buf)
{
    const render_pass& skinned_pass = render_pass_registry::get().pass("simple_points");

    d_verts_buf_ = device->create_buffer_unique(rhi::buffer_type::vertex, grid_.size() * sizeof(Eigen::Vector3f), nullptr);
    
    d_verts_view_ = device->map_buffer<Eigen::Vector3f>(d_verts_buf_.get(), 0, grid_.size());

    d_mvp_uniforms_ = skinned_pass.uniform_set_builder(device, 0)
        .uniform("mvp", { d_mvp_buf })
        .produce();

    return skinned_pass.draw_item_builder()
        .elem_count(grid_.size())
        .vertex_buffers({ d_verts_buf_.get() })
        .uniform_sets({ {0, d_mvp_uniforms_.get()} })
        .update([this](rhi::device*) {
            std::transform(std::begin(grid_.sorted_elements_), std::end(grid_.sorted_elements_), std::begin(d_verts_view_), [](const sim::particle& p) { return p.pos; });
        })
        .produce_draw();
}

void sph_sim::reset()
{
    mth::pcg32 rand;
    const Eigen::Vector3f block_dim = block_[1] - block_[0];
    std::vector<sim::particle> particles;
    particles.reserve(grid_.size());
    for (std::size_t i = 0; i < grid_.size(); i++)
    {
        const Eigen::Vector3f rand3 = Eigen::Vector3f(rand.nextFloat(), rand.nextFloat(), rand.nextFloat());
        const Eigen::Vector3f point = block_[0] + block_dim.cwiseProduct(rand3);
        particles.emplace_back(point, Eigen::Vector3f::Zero(), 0, 0.f, 0.f);
    }

    grid_ = sim::spatial_hash_table(std::move(particles), 2.f / inv_h_);
}

}