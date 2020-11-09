#include "meth.hpp"

namespace xs
{
namespace mth
{
	namespace vec_impl
	{
		const xyz_vec_base xyz_vec_base::zero = vec_base(_mm_setzero_ps());
	}

	mat perspective(const float fov, const float width, const float height, const float z_near, const float z_far)
	{
		const float half_fov = fov * 0.5f;
		const float h = std::cos(half_fov) / std::sin(half_fov);
		const float w = h * height / width;

		mat out;
		out[0][0] = w;
		out[0][1] = 0.f;
		out[0][2] = 0.f;
		out[0][3] = 0.f;
		out[1][0] = 0.f;
		out[1][1] = -h;
		out[1][2] = 0.f;
		out[1][3] = 0.f;
		out[2][0] = 0.f;
		out[2][1] = 0.f;
		out[2][2] = z_far / (z_near - z_far);
		out[2][3] = -1.f;
		out[3][0] = 0.f;
		out[3][1] = 0.f;
		out[3][2] = -(z_far * z_near) / (z_far - z_near);
		out[3][3] = 0.f;
		return out;
	}

	mat look_at(const pos& eye, const pos& center, const ndir& up)
	{
		const ndir f = normalize(center - eye);
		const ndir s = normalize(cross(f, up));
		const ndir u = cross(s, f);
		const ndir nf = ndir(vec_impl::negate(f));

		// TODO: can this use simd?
		mat out;
		out[0][0] = s.x();
		out[1][0] = s.y();
		out[2][0] = s.z();
		out[0][1] = u.x();
		out[1][1] = u.y();
		out[2][1] = u.z();
		out[0][2] = nf.x();
		out[1][2] = nf.y();
		out[2][2] = nf.z();
		out[3][0] = -mth::dot(s, eye);
		out[3][1] = -mth::dot(u, eye);
		out[3][2] = mth::dot(f, eye);
		out[3][3] = 1.f;
		out[0][3] = 0.f;
		out[1][3] = 0.f;
		out[2][3] = 0.f;
		return out;
	}

	mat to_mat(const transform& transform)
	{
		const float q0 = transform.rotation.x(), q1 = transform.rotation.y(), q2 = transform.rotation.z(), q3 = transform.rotation.w();
		const quat q_2 = quat(vec_impl::mul(transform.rotation, transform.rotation));

		mat out;
		out[0][0] = 1.f - 2.f * (q_2.z() + q_2.w());
		out[1][0] = 2.f * (q1 * q2 - q0 * q3);
		out[2][0] = 2.f * (q0 * q2 + q1 * q3);
		out[0][1] = 2.f * (q1 * q2 + q0 * q3);
		out[1][1] = 1.f - 2.f * (q_2.y() + q_2.w());
		out[2][1] = 2.f * (q2 * q3 - q0 * q1);
		out[0][2] = 2.f * (q1 * q3 - q0 * q2);
		out[1][2] = 2.f * (q0 * q1 + q2 * q3);
		out[2][2] = 1.f - 2.f * (q_2.y() + q_2.z());
		out[3][0] = transform.translation.x();
		out[3][1] = transform.translation.y();
		out[3][2] = transform.translation.z();
		out[3][3] = 1.f;
		out[0][3] = 0.f;
		out[1][3] = 0.f;
		out[2][3] = 0.f;
		return out;
	}
}
}