#include "particles.hpp"

#include <algorithm>

namespace xs
{
	spatial_hash_table::spatial_hash_table(const std::vector<particle>& elements, float cell_size) :
		inv_cell_size_(mth::vec_impl::div(mth::vec_impl::make(1.f), mth::vec_impl::make(cell_size))),
		sorted_elements_(elements),
		grid_table_(elements.size())
	{
		for (particle& p : sorted_elements_)
		{
			union { __m128i m; uint32_t i[4]; }  ipos;
			ipos.m = to_uint_space(p.pos);
			p.morton = morton_encode(ipos.i[0], ipos.i[1], ipos.i[2]);
		}

		std::sort(std::begin(sorted_elements_), std::end(sorted_elements_), [](const particle& a, const particle& b) { return a.morton < b.morton; });
		grid_table_.reserve(sorted_elements_.size());
		for (size_t i = sorted_elements_.size(); i-- > 0;) // can't set i to a potentially negative number in init, so...
		{
			const uint64_t hash = hash_coord(sorted_elements_[i].pos);
			grid_table_[hash] = i;
		}
	}

	void spatial_hash_table::optimize()
	{
		for (particle& p : sorted_elements_)
		{
			union { __m128i m; uint32_t i[4]; }  ipos;
			ipos.m = to_uint_space(p.pos);
			p.morton = morton_encode(ipos.i[0], ipos.i[1], ipos.i[2]);
		}

		std::sort(std::begin(sorted_elements_), std::end(sorted_elements_), [](const particle& a, const particle& b) { return a.morton < b.morton; });

		for (size_t i = sorted_elements_.size(); i-- > 0;)
		{
			const uint64_t hash = hash_coord(sorted_elements_[i].pos);
			grid_table_[hash] = i;
		}
	}

	std::vector<particle> spatial_hash_table::get_neighbors(const particle& elem) const
	{
		// TODO: is this actually faster?
		const static union { int64_t a[4]; __m256i m; } idx_offsets[] = {
			{-1, -1, -1, 0}, {0, -1, -1, 0}, {1, -1, -1, 0},
			{-1,  0, -1, 0}, {0,  0, -1, 0}, {1,  0, -1, 0},
			{-1,  1, -1, 0}, {0,  1, -1, 0}, {1,  1, -1, 0},
			{-1, -1,  0, 0}, {0, -1,  0, 0}, {1, -1,  0, 0},
			{-1,  0,  0, 0}, /*{0,  0,  0, 0},*/ {1,  0,  0, 0},
			{-1,  1,  0, 0}, {0,  1,  0, 0}, {1,  1,  0, 0},
			{-1, -1,  1, 0}, {0, -1,  1, 0}, {1, -1,  1, 0},
			{-1,  0,  1, 0}, {0,  0,  1, 0}, {1,  0,  1, 0},
			{-1,  1,  1, 0}, {0,  1,  1, 0}, {1,  1,  1, 0}
		};

		std::vector<particle> neighbors;
		neighbors.reserve(32);

		const uint64_t morton = elem.morton;

		for (uint8_t idx_offset = 0; idx_offset < 26; idx_offset++)
		{
			const uint32_t hash = hash_coord(elem.pos, idx_offsets[idx_offset].m);
			const auto cell_itr = grid_table_.find(hash);
			if (cell_itr == std::end(grid_table_))
			{
				continue;
			}

			static constexpr uint64_t max_morton_diff = 1;

			size_t particle_idx = cell_itr->second;
			do
			{
				if (sorted_elements_[particle_idx].morton != elem.morton) // TODO: fix this bug
				{
					neighbors.push_back(sorted_elements_[particle_idx]);
				}
				particle_idx++;
			} while (particle_idx < sorted_elements_.size() && sorted_elements_[particle_idx].morton - morton < max_morton_diff);
		}

		return neighbors;
	}

	particle_system::particle_system(const std::vector<particle>& particles, rhi::device* device, const float h, const float p0, const float k) :
		grid_(particles, h),
		particle_mesh_(),
		kernel_(),
		inv_h_(1.f / h),
		inv_h_d_(pow<3>(1.f / h)),
		inv_p0_(1.f / p0),
		k_(k),
		mass_(pow<3>(h) * p0)
	{
		std::vector<mth::pos> verts;
		verts.reserve(particles.size());
		for (const particle& p : particles)
		{
			verts.push_back(p.pos);
		}

		particle_mesh_ = xs::mesh(device, std::move(verts), {});

		float itr = 0.f;
		for (size_t i = 0; i < kernel_.size(); i++, itr += step)
		{
			kernel_[i] = cubic_kernel(itr);
		}
	}

	void particle_system::evaluate_pressure(eval_context& ctx)
	{
		const size_t particle_idx = ctx.node_registry.get<size_t>(ctx.parent);
		const particle& cur_particle = grid_[particle_idx];
		std::vector<particle> neighbors = grid_.get_neighbors(grid_[particle_idx]);

		// TODO: unroll?
		float density = 0.f;
		for (const particle& neighbor : neighbors)
		{
			const float q = mth::len(neighbor.pos - cur_particle.pos) * inv_h_;
			const float f_q = sample_kernel(q);
			const float Wij = inv_h_d_ * f_q;

			density += mass_ * Wij;
		}

		const float pressure = k_ * (pow<7>(density * inv_p0_) - 1.f);

		ctx.node_registry.emplace_or_replace<float>(ctx.cur, density);
		ctx.node_registry.emplace_or_replace<size_t>(ctx.cur, particle_idx);
		grid_[particle_idx].pressure = pressure;
	}

	void particle_system::evaluate_pressure_force(eval_context& ctx) const
	{
		const size_t particle_idx = ctx.node_registry.get<size_t>(ctx.parent);
		const particle& cur_particle = grid_[particle_idx];
		std::vector<particle> neighbors = grid_.get_neighbors(grid_[particle_idx]);

		const float density = ctx.node_registry.get<float>(ctx.parent);

		// TODO: unroll?
		mth::dir delta_pressure = mth::dir(0.f);
		for (const particle& neighbor : neighbors)
		{
			const mth::dir dpos = neighbor.pos - cur_particle.pos;
			const float dpos_inv_mag = mth::inv_len(dpos);

			const float dpressure = neighbor.pressure - cur_particle.pressure;
			delta_pressure = delta_pressure + dpos * dpressure * dpos_inv_mag;
		}

		const mth::dir force = std::abs(density) < 0.00001f ? 0.f : delta_pressure * (-mass_ / density);

		const mth::dir* maybe_prev_force = ctx.node_registry.try_get<mth::dir>(ctx.parent);
		ctx.node_registry.emplace_or_replace<mth::dir>(ctx.cur, maybe_prev_force ? *maybe_prev_force + force : force);
		ctx.node_registry.emplace_or_replace<size_t>(ctx.cur,  particle_idx);
	}

	void particle_system::evaluate_friction_force(eval_context& ctx) const
	{
		const size_t particle_idx = ctx.node_registry.get<size_t>(ctx.parent);
		const particle& cur_particle = grid_[particle_idx];
		std::vector<particle> neighbors = grid_.get_neighbors(grid_[particle_idx]);

		// TODO: unroll?
		mth::dir delta_velocity = mth::dir(0.f);
		for (const particle& neighbor : neighbors)
		{
			const mth::dir dvel = neighbor.vel - cur_particle.vel;
			delta_velocity = delta_velocity + dvel;
		}
		delta_velocity = neighbors.empty() ? mth::dir::zero : delta_velocity / float(neighbors.size());

		const mth::dir force = (delta_velocity * delta_velocity) * mass_ * cur_particle.vel;

		const mth::dir* maybe_prev_force = ctx.node_registry.try_get<mth::dir>(ctx.parent);
		ctx.node_registry.emplace_or_replace<mth::dir>(ctx.cur, maybe_prev_force ? *maybe_prev_force + force : force);
		ctx.node_registry.emplace_or_replace<size_t>(ctx.cur, particle_idx);
	}

	void particle_system::evaluate_gravity_force(eval_context& ctx) const
	{
		const size_t particle_idx = ctx.node_registry.get<size_t>(ctx.parent);

		// conventional standard value
		static constexpr float gravity_acc_scalar = 9.80665f;
		static const mth::dir gravity_acc = mth::dir(0.f, 0.f, -gravity_acc_scalar);
		const mth::dir gravity_force = gravity_acc * mass_;

		const mth::dir* maybe_prev_force = ctx.node_registry.try_get<mth::dir>(ctx.parent);
		ctx.node_registry.emplace_or_replace<mth::dir>(ctx.cur, maybe_prev_force ? *maybe_prev_force + gravity_force : gravity_force);
		ctx.node_registry.emplace_or_replace<size_t>(ctx.cur, particle_idx);
	}

	void particle_system::evaluate_collision_force(eval_context& ctx)
	{
		const size_t particle_idx = ctx.node_registry.get<size_t>(ctx.parent);
		particle& cur_particle = grid_[particle_idx];

		const mth::dir* maybe_prev_force = ctx.node_registry.try_get<mth::dir>(ctx.parent);

		static constexpr float ground_height = 0.f;

		mth::dir new_force = maybe_prev_force ? *maybe_prev_force : mth::dir::zero;
		if (cur_particle.pos.z() < ground_height)
		{
			cur_particle.vel.z() = 0.f;
			new_force.z() = 0.f;
		}

		ctx.node_registry.emplace_or_replace<mth::dir>(ctx.cur, new_force);
		ctx.node_registry.emplace_or_replace<size_t>(ctx.cur, particle_idx);
	}

	void particle_system::apply_particle_forces(scene* scene, entt::entity cur, float dt)
	{
		const size_t particle_idx = scene->get_node_registry().get<size_t>(cur);
		particle& cur_particle = grid_[particle_idx];
		const mth::dir& force = scene->get_node_registry().get<mth::dir>(cur);

		cur_particle.vel = cur_particle.vel + force * dt / mass_;
		cur_particle.pos = cur_particle.pos + cur_particle.vel * dt;
		
		// This is kind of sketch for triangle meshes since the particles could be re-ordered every iteration
		// but one problem at a time
		particle_mesh_.vertex_buffer[particle_idx] = cur_particle.pos;
	}
}