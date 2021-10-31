/*#include "meth.hpp"

namespace xs
{
namespace mth
{
	namespace vec_impl
	{
		const xyz_vec_base xyz_vec_base::zero = vec_base(_mm_setzero_ps());
	}

	namespace mat_impl
	{
		// The original code as provided by Intel in
		// "Streaming SIMD Extensions - Inverse of 4x4 Matrix"
		// (ftp://download.intel.com/design/pentiumiii/sml/24504301.pdf)
		void compute_inverse(const float* src, float* dst)
		{
			__m128 minor0, minor1, minor2, minor3;
			__m128 row0, row1 = _mm_setzero_ps(), row2, row3 = _mm_setzero_ps();
			__m128 det, tmp1 = _mm_setzero_ps();

			tmp1 = _mm_loadh_pi(_mm_loadl_pi(tmp1, (__m64*)(src)), (__m64*)(src + 4));
			row1 = _mm_loadh_pi(_mm_loadl_pi(row1, (__m64*)(src + 8)), (__m64*)(src + 12));

			row0 = _mm_shuffle_ps(tmp1, row1, 0x88);
			row1 = _mm_shuffle_ps(row1, tmp1, 0xDD);

			tmp1 = _mm_loadh_pi(_mm_loadl_pi(tmp1, (__m64*)(src + 2)), (__m64*)(src + 6));
			row3 = _mm_loadh_pi(_mm_loadl_pi(row3, (__m64*)(src + 10)), (__m64*)(src + 14));

			row2 = _mm_shuffle_ps(tmp1, row3, 0x88);
			row3 = _mm_shuffle_ps(row3, tmp1, 0xDD);

			tmp1 = _mm_mul_ps(row2, row3);
			tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);

			minor0 = _mm_mul_ps(row1, tmp1);
			minor1 = _mm_mul_ps(row0, tmp1);

			tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);

			minor0 = _mm_sub_ps(_mm_mul_ps(row1, tmp1), minor0);
			minor1 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor1);
			minor1 = _mm_shuffle_ps(minor1, minor1, 0x4E);

			tmp1 = _mm_mul_ps(row1, row2);
			tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);

			minor0 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor0);
			minor3 = _mm_mul_ps(row0, tmp1);

			tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);

			minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row3, tmp1));
			minor3 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor3);
			minor3 = _mm_shuffle_ps(minor3, minor3, 0x4E);

			tmp1 = _mm_mul_ps(_mm_shuffle_ps(row1, row1, 0x4E), row3);
			tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
			row2 = _mm_shuffle_ps(row2, row2, 0x4E);

			minor0 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor0);
			minor2 = _mm_mul_ps(row0, tmp1);

			tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);

			minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row2, tmp1));
			minor2 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor2);
			minor2 = _mm_shuffle_ps(minor2, minor2, 0x4E);

			tmp1 = _mm_mul_ps(row0, row1);
			tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);

			minor2 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor2);
			minor3 = _mm_sub_ps(_mm_mul_ps(row2, tmp1), minor3);

			tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);

			minor2 = _mm_sub_ps(_mm_mul_ps(row3, tmp1), minor2);
			minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row2, tmp1));

			tmp1 = _mm_mul_ps(row0, row3);
			tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);

			minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row2, tmp1));
			minor2 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor2);

			tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);

			minor1 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor1);
			minor2 = _mm_sub_ps(minor2, _mm_mul_ps(row1, tmp1));

			tmp1 = _mm_mul_ps(row0, row2);
			tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);

			minor1 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor1);
			minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row1, tmp1));

			tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);

			minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row3, tmp1));
			minor3 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor3);

			det = _mm_mul_ps(row0, minor0);
			det = _mm_add_ps(_mm_shuffle_ps(det, det, 0x4E), det);
			det = _mm_add_ss(_mm_shuffle_ps(det, det, 0xB1), det);

			tmp1 = _mm_rcp_ss(det);

			det = _mm_sub_ss(_mm_add_ss(tmp1, tmp1), _mm_mul_ss(det, _mm_mul_ss(tmp1, tmp1)));
			det = _mm_shuffle_ps(det, det, 0x00);

			minor0 = _mm_mul_ps(det, minor0);
			_mm_storel_pi((__m64*)(dst), minor0);
			_mm_storeh_pi((__m64*)(dst + 2), minor0);

			minor1 = _mm_mul_ps(det, minor1);
			_mm_storel_pi((__m64*)(dst + 4), minor1);
			_mm_storeh_pi((__m64*)(dst + 6), minor1);

			minor2 = _mm_mul_ps(det, minor2);
			_mm_storel_pi((__m64*)(dst + 8), minor2);
			_mm_storeh_pi((__m64*)(dst + 10), minor2);

			minor3 = _mm_mul_ps(det, minor3);
			_mm_storel_pi((__m64*)(dst + 12), minor3);
			_mm_storeh_pi((__m64*)(dst + 14), minor3);
		}
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
		out[1][1] = h;
		out[1][2] = 0.f;
		out[1][3] = 0.f;
		out[2][0] = 0.f;
		out[2][1] = 0.f;
		out[2][2] = z_far / (z_far - z_near);
		out[2][3] = 1.f;
		out[3][0] = 0.f;
		out[3][1] = 0.f;
		out[3][2] = -(z_far * z_near) / (z_far - z_near);
		out[3][3] = 0.f;
		return out;
	}

	mat look_at(const pos& eye, const pos& center, const ndir& up)
	{
		const ndir f = normalize(center - eye);
		const ndir s = normalize(cross(up, f));
		const ndir u = cross(f, s);

		// TODO: can this use simd?
		mat out;
		out[0][0] = s.x();
		out[1][0] = s.y();
		out[2][0] = s.z();
		out[0][1] = u.x();
		out[1][1] = u.y();
		out[2][1] = u.z();
		out[0][2] = f.x();
		out[1][2] = f.y();
		out[2][2] = f.z();
		out[3][0] = -mth::dot(s, eye);
		out[3][1] = -mth::dot(u, eye);
		out[3][2] = -mth::dot(f, eye);
		out[3][3] = 1.f;
		out[0][3] = 0.f;
		out[1][3] = 0.f;
		out[2][3] = 0.f;
		return out;
	}

	mat to_mat(const transform& transform)
	{
		//const float q0 = transform.rotation.x(), q1 = transform.rotation.y(), q2 = transform.rotation.z(), q3 = transform.rotation.w();
		const float qx = transform.rotation.x(), qy = transform.rotation.y(), qz = transform.rotation.z(), qw = transform.rotation.w();
		const quat q_2 = quat(vec_impl::mul(transform.rotation, transform.rotation));

		mat out;
		out[0][0] = 1.f - 2.f * (q_2.y() + q_2.z());
		out[0][1] = 2.f * (qx * qy + qz * qw);
		out[0][2] = 2.f * (qx * qz - qy * qw);
		out[0][3] = 0.f;
		out[1][0] = 2.f * (qx * qy - qz * qw);
		out[1][1] = 1.f - 2.f * (q_2.x() + q_2.z());
		out[1][2] = 2.f * (qy * qz + qx * qw);
		out[1][3] = 0.f;
		out[2][0] = 2.f * (qx * qz + qy * qw);
		out[2][1] = 2.f * (qy * qz - qx * qw);
		out[2][2] = 1.f - 2.f * (q_2.x() + q_2.y());		
		out[2][3] = 0.f;
		out[3][0] = transform.translation.x();
		out[3][1] = transform.translation.y();
		out[3][2] = transform.translation.z();
		out[3][3] = 1.f;
		return out;
	}

	const mat mat::identity = to_mat(transform::identity);

	const quat quat::identity = quat(0.f, 0.f, 0.f, 1.f);

	const transform transform::identity = transform(pos(0.f), quat::identity);
}
}*/