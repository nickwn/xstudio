#pragma once

#include <cstdint>
#include <cmath>
#include <memory>
#include <concepts>

#if !defined(_MSC_VER) || _MSC_VER < 1900  // MSVC 2015
#error ur compiler is actually good??
#endif

#include <malloc.h>
#include <immintrin.h>

// math library for crackheads, WIP
// SIMD instructions not super needed with modern compiler optimizations but they look cool and I want practice with optimizing
// most of these still need to be tested for speed though

#define mth_rpl(a, i) _mm_shuffle_ps(a, a, _MM_SHUFFLE(i, i, i, i))
#define mth_szl(a, i, j, k ,l) _mm_shuffle_ps(a, a, _MM_SHUFFLE(i, j, k, l))

namespace xs
{
namespace mth
{
	namespace vec_impl
	{
		using vreg = __m128;
		struct alignas(16) vec_base
		{
			vreg data;
			constexpr vec_base() : data() {}
			constexpr vec_base(const vec_base& init) : data(init) {}
			constexpr vec_base(const vreg& init) : data(init) {}
			constexpr operator vreg&() { return data; }
			constexpr operator const vreg& () const { return data; }
		};

		static inline vec_base make(const float x)
		{
			return _mm_set1_ps(x);
		}

		static inline vec_base make(const float x, const float y, const float z, const float w)
		{
			return _mm_setr_ps(x, y, z, w);
		}

		static constexpr float& get_comp_ref(vreg& vec, const uint32_t idx)
		{
			return (((float*)&(vec))[idx]);
		}

		static constexpr float get_comp(const vec_base& vec, const uint32_t idx)
		{
			return (((float*)&(vec))[idx]);
		}

		static inline vec_base negate(const vec_base& vec)
		{
			return _mm_sub_ps(_mm_setzero_ps(), vec);
		}

		static inline vec_base add(const vec_base& vec1, const vec_base& vec2)
		{
			return _mm_add_ps(vec1, vec2);
		}

		static inline vec_base sub(const vec_base& vec1, const vec_base& vec2)
		{
			return _mm_sub_ps(vec1, vec2);
		}

		static inline vec_base mul(const vec_base& vec1, const vec_base& vec2)
		{
			return _mm_mul_ps(vec1, vec2);
		}

		static inline vec_base div(const vec_base& vec1, const vec_base& vec2)
		{
			return _mm_div_ps(vec1, vec2);
		}

		static inline vec_base madd(const vec_base& vec1, const vec_base& vec2, const vec_base& vec3)
		{
			return _mm_fmadd_ps(vec1, vec2, vec3);
		}

		static inline vec_base sin(const vec_base& v)
		{
			return _mm_sin_ps(v);
		}

		static inline vec_base cos(const vec_base& v)
		{
			return _mm_cos_ps(v);
		}

		// 0xff default param for xyzvecs?
		static inline vec_base dot(const vec_base& vec1, const vec_base& vec2)
		{
			return _mm_dp_ps(vec1, vec2, 0xff);
		}

		static inline vec_base sqrt(const vec_base& vec)
		{
			return _mm_sqrt_ps(vec);
		}

		static inline vec_base inv_sqrt(const vec_base& vec)
		{
			return _mm_rsqrt_ps(vec);
		}

		static inline vec_base inv(const vec_base& vec)
		{
			return _mm_rcp_ps(vec);
		}

		static inline vec_base inv_len(const vec_base& vec)
		{
			return inv_sqrt(dot(vec, vec));
		}

		static inline vec_base len(const vec_base& vec)
		{
			return sqrt(dot(vec, vec));
		}

		static inline vec_base normalize(const vec_base& vec)
		{
			return mul(vec, inv_len(vec));
		}

		// TODO: is this faster than non-fma version?
		static inline vec_base cross_fma(const vec_base& x, const vec_base& y)
		{
			const vec_base tmp0 = _mm_shuffle_ps(y, y, _MM_SHUFFLE(3, 0, 2, 1));
			const vec_base tmp1 = _mm_shuffle_ps(x, x, _MM_SHUFFLE(3, 0, 2, 1));
			const vec_base tmp2 = _mm_mul_ps(tmp1, y);
			const vec_base tmp3 = _mm_fmsub_ps(tmp0, x, tmp2);
			return _mm_shuffle_ps(tmp3, tmp3, _MM_SHUFFLE(3, 0, 2, 1));
		}

		/*static inline vec_base cross_fma(const vec_base& x, const vec_base& y)
		{
			vec_base tmp0 = _mm_shuffle_ps(y, y, _MM_SHUFFLE(3, 0, 2, 1));
			vec_base tmp1 = _mm_shuffle_ps(x, x, _MM_SHUFFLE(3, 0, 2, 1));
			tmp0 = _mm_mul_ps(tmp0, x);
			tmp1 = _mm_mul_ps(tmp1, y);
			__m128 tmp2 = _mm_sub_ps(tmp0, tmp1);
			return _mm_shuffle_ps(tmp2, tmp2, _MM_SHUFFLE(3, 0, 2, 1));
		}*/

		/*static inline vec_base qmul(const vec_base& a, const vec_base& b)
		{
			const static __m128 qmul_sign_mask0 = _mm_setr_ps(1.f, -1.f, 1.f, -1.f);
			const static __m128 qmul_sign_mask1 = _mm_setr_ps(1.f, 1.f, -1.f, -1.f);
			const static __m128 qmul_sign_mask2 = _mm_setr_ps(-1.f, 1.f, 1.f, -1.f);

			__m128 res = mul(mth_rpl(a, 3), b);
			res = madd(mul(mth_rpl(a, 0), mth_szl(b, 3, 2, 1, 0)), qmul_sign_mask0, res);
			res = madd(mul(mth_rpl(a, 1), mth_szl(b, 2, 3, 0, 1)), qmul_sign_mask1, res);
			res = madd(mul(mth_rpl(a, 2), mth_szl(b, 1, 0, 3, 2)), qmul_sign_mask2, res);
			return res;
		}*/

		// above function doesn't work so i have this for now...
		static inline vec_base qmul(const vec_base& q1, const vec_base& q2) 
		{
			const float q1x = get_comp(q1, 0), q1y = get_comp(q1, 1), q1z = get_comp(q1, 2), q1w = get_comp(q1, 3);
			const float q2x = get_comp(q2, 0), q2y = get_comp(q2, 1), q2z = get_comp(q2, 2), q2w = get_comp(q2, 3);

			float x = q1x * q2w + q1y * q2z - q1z * q2y + q1w * q2x;
			float y = -q1x * q2z + q1y * q2w + q1z * q2x + q1w * q2y;
			float z = q1x * q2y - q1y * q2x + q1z * q2w + q1w * q2z;
			float w = -q1x * q2x - q1y * q2y - q1z * q2z + q1w * q2w;
			return make(x, y, z, w);
		}

		static inline vec_base vqmul(const vec_base q, const vec_base v)
		{
			const vec_base qw = mth_rpl(q, 3);
			vec_base t = cross_fma(q, v);
			t = add(t, t);
			const __m128 vt0 = madd(qw, t, v);
			const __m128 vt1 = cross_fma(q, t);
			const __m128 rotated = add(vt0, vt1);
			return rotated;
		}

		struct xyz_vec_base: public vec_base
		{
			constexpr xyz_vec_base() = default;
			constexpr xyz_vec_base(const vec_base& init) : vec_base(init) {}
			inline xyz_vec_base(const float x) : vec_base(make(x)) {}
			inline xyz_vec_base(const float x, const float y, const float z) : vec_base(make(x, y, z, 1.f)) {}

			constexpr float& x() { return vec_impl::get_comp_ref(data, 0); }
			constexpr float& y() { return vec_impl::get_comp_ref(data, 1); }
			constexpr float& z() { return vec_impl::get_comp_ref(data, 2); }
			constexpr float x() const { return vec_impl::get_comp(data, 0); }
			constexpr float y() const { return vec_impl::get_comp(data, 1); }
			constexpr float z() const { return vec_impl::get_comp(data, 2); }

			const static xyz_vec_base zero;
		};

		struct xyzw_vec_base : public vec_base
		{
			constexpr xyzw_vec_base() = default;
			constexpr xyzw_vec_base(const vec_base& init) : vec_base(init) {}
			inline xyzw_vec_base(const float x) : vec_base(make(x)) {}
			inline xyzw_vec_base(const float x, const float y, const float z, float w) : vec_base(make(x, y, z, w)) {}

			constexpr float& x() { return vec_impl::get_comp_ref(data, 0); }
			constexpr float& y() { return vec_impl::get_comp_ref(data, 1); }
			constexpr float& z() { return vec_impl::get_comp_ref(data, 2); }
			constexpr float& w() { return vec_impl::get_comp_ref(data, 3); }
			constexpr float x() const { return vec_impl::get_comp(data, 0); }
			constexpr float y() const { return vec_impl::get_comp(data, 1); }
			constexpr float z() const { return vec_impl::get_comp(data, 2); }
			constexpr float w() const { return vec_impl::get_comp(data, 3); }
		};

		struct rgba_vec_base : public vec_base
		{
			constexpr rgba_vec_base() : vec_base() {}
			constexpr rgba_vec_base(const vec_base& init) : vec_base(init) {}
			inline rgba_vec_base(const float r) : vec_base(make(r)) {}
			inline rgba_vec_base(const float r, const float g, const float b, const float a = 1.f) : vec_base(make(r, g, b, a)) {}
			
			constexpr float& r() { return vec_impl::get_comp_ref(data, 0); }
			constexpr float& g() { return vec_impl::get_comp_ref(data, 1); }
			constexpr float& b() { return vec_impl::get_comp_ref(data, 2); }
			constexpr float& a() { return vec_impl::get_comp_ref(data, 3); }
			constexpr float r() const { return vec_impl::get_comp(data, 0); }
			constexpr float g() const { return vec_impl::get_comp(data, 1); }
			constexpr float b() const { return vec_impl::get_comp(data, 2); }
			constexpr float a() const { return vec_impl::get_comp(data, 3); }
		};
	} // namespace vec_impl

	namespace mat_impl
	{
		union alignas(64) mat_base
		{
			float m[4][4];
			__m128 row[4];
		};

		// from https://gist.github.com/rygorous/4172889

		// dual linear combination using AVX instructions on YMM regs
		static inline __m256 twolincomb_AVX_8(__m256 A01, const mat_base& B)
		{
			__m256 result;
			result = _mm256_mul_ps(_mm256_shuffle_ps(A01, A01, 0x00), _mm256_broadcast_ps(&B.row[0]));
			result = _mm256_add_ps(result, _mm256_mul_ps(_mm256_shuffle_ps(A01, A01, 0x55), _mm256_broadcast_ps(&B.row[1])));
			result = _mm256_add_ps(result, _mm256_mul_ps(_mm256_shuffle_ps(A01, A01, 0xaa), _mm256_broadcast_ps(&B.row[2])));
			result = _mm256_add_ps(result, _mm256_mul_ps(_mm256_shuffle_ps(A01, A01, 0xff), _mm256_broadcast_ps(&B.row[3])));
			return result;
		}

		// this should be noticeably faster with actual 256-bit wide vector units (Intel);
		// not sure about double-pumped 128-bit (AMD), would need to check.
		static inline void matmult_AVX_8(mat_base& out, const mat_base& A, const mat_base& B)
		{
			_mm256_zeroupper();
			__m256 A01 = _mm256_loadu_ps(&A.m[0][0]);
			__m256 A23 = _mm256_loadu_ps(&A.m[2][0]);

			__m256 out01x = twolincomb_AVX_8(A01, B);
			__m256 out23x = twolincomb_AVX_8(A23, B);

			_mm256_storeu_ps(&out.m[0][0], out01x);
			_mm256_storeu_ps(&out.m[2][0], out23x);
		}

		void compute_inverse(const float* src, float* dst);
	} // namespace mat_impl

	/*template<typename T> 
	concept is_vec3 = requires(T v)
	{
		{v.x()} -> std::same_as<float&>;
		{v.y()} -> std::same_as<float&>;
		{v.z()} -> std::same_as<float&>;
	};

	template<typename T>
	concept is_vec4 = requires(T v)
	{
		{v.x()} -> std::same_as<float&>;
		{v.y()} -> std::same_as<float&>;
		{v.z()} -> std::same_as<float&>;
		{v.w()} -> std::same_as<float&>;
	};*/

	struct dir;

	// A point in 3D space with no direction
	struct pos : public vec_impl::xyz_vec_base 
	{
		inline pos() = default;
		inline pos(const float x) : vec_impl::xyz_vec_base(x) {}
		inline pos(const float x, const float y, const float z) : vec_impl::xyz_vec_base(x, y, z) {}

		constexpr pos(const vec_impl::xyz_vec_base& init) : vec_impl::xyz_vec_base(init) {}

		pos operator+(const dir& d) const;
		pos operator-(const dir& d) const;
		dir operator-(const pos& p) const;

		pos& operator+=(const dir& rhs);
	};

	// A direction in 3D space with no position
	struct dir : public vec_impl::xyz_vec_base 
	{
		inline dir() = default;
		inline dir(const float x) : vec_impl::xyz_vec_base(x) {}
		inline dir(const float x, const float y, const float z) : vec_impl::xyz_vec_base(x, y, z) {}

		constexpr dir(const vec_impl::xyz_vec_base& init) : vec_impl::xyz_vec_base(init) {}

		dir operator*(const dir& d) const;
		dir operator*(const float f) const;
		dir operator/(const float f) const;
		dir operator/(const dir& d) const;
		dir operator+(const dir& d) const;
		dir operator-(const dir& d) const;
		dir operator-() const;

		dir& operator+=(const dir& rhs);
	};

	// A normalized direction in 3D space with no position
	// still not sure how to implement this efficiently
	struct ndir : public vec_impl::xyz_vec_base 
	{
		inline ndir() = default;
		inline ndir(const float x) : vec_impl::xyz_vec_base(x) {}
		inline ndir(const float x, const float y, const float z) : vec_impl::xyz_vec_base(x, y, z) {}

		constexpr ndir(const vec_impl::xyz_vec_base& init) : vec_impl::xyz_vec_base(init) {}

		ndir operator+(const dir& d) const;
		ndir operator-(const dir& d) const;
		ndir operator-() const;
	};

	inline pos pos::operator+(const dir& d) const
	{
		return pos(vec_impl::add(*this, d));
	}

	inline pos pos::operator-(const dir& d) const
	{
		return pos(vec_impl::sub(*this, d));
	}

	inline dir pos::operator-(const pos& p) const
	{
		return dir(vec_impl::sub(*this, p));
	}

	inline pos& pos::operator+=(const dir& rhs)
	{
		data = vec_impl::add(*this, rhs);
		return *this;
	}

	inline dir dir::operator*(const float f) const
	{
		return dir(vec_impl::mul(*this, vec_impl::make(f)));
	}

	inline dir dir::operator*(const dir& d) const
	{
		return dir(vec_impl::mul(*this, d));
	}

	inline dir dir::operator/(const float f) const
	{
		return dir(vec_impl::div(*this, vec_impl::make(f)));
	}

	inline dir dir::operator/(const dir& d) const
	{
		return dir(vec_impl::div(*this, d));
	}

	inline dir dir::operator+(const dir& d) const
	{
		return dir(vec_impl::add(*this, d));
	}

	inline dir dir::operator-(const dir& d) const
	{
		return dir(vec_impl::sub(*this, d));
	}

	inline dir dir::operator-() const
	{
		return dir(vec_impl::negate(*this));
	}

	inline dir& dir::operator+=(const dir& rhs)
	{
		data = vec_impl::add(*this, rhs);
		return *this;
	}

	inline ndir ndir::operator+(const dir& d) const
	{
		return dir(vec_impl::add(*this, d));
	}

	inline ndir ndir::operator-(const dir& d) const
	{
		return dir(vec_impl::sub(*this, d));
	}

	inline ndir ndir::operator-() const
	{
		return dir(vec_impl::negate(*this));
	}

	// non-member functions

	static inline ndir normalize(const dir& d)
	{
		return ndir(vec_impl::normalize(d));
	}

	static inline float dot(const dir& vec1, const dir& vec2)
	{
		return vec_impl::get_comp(vec_impl::dot(vec1, vec2), 0);
	}

	static inline dir cross(const dir& dir1, const dir& dir2)
	{
		return dir(vec_impl::cross_fma(dir1, dir2));
	}

	static inline float len(const dir& vec)
	{
		return vec_impl::get_comp(vec_impl::len(vec), 0);
	}

	static inline float inv_len(const dir& vec)
	{
		return vec_impl::get_comp(vec_impl::inv_len(vec), 0);
	}

	struct mat
	{
		mat() = default;

		inline mat operator*(const mat& m) const
		{
			mat out;
			mat_impl::matmult_AVX_8(out.base_, base_, m.base_);
			return out;
		}

		inline float* operator[](const size_t i)
		{
			return base_.m[i];
		}

		inline const float* operator[](const size_t i) const
		{
			return base_.m[i];
		}

		const static mat identity;

	private:
		mat_impl::mat_base base_;
	};

	static inline mat inverse(const mat& m)
	{
		mat out;
		mat_impl::compute_inverse(&m[0][0], &out[0][0]);
		return out;
	}

	mat perspective(const float fov, const float width, const float height, const float z_near, const float z_far);

	mat look_at(const pos& eye, const pos& center, const ndir& up = ndir(0.f, 1.f, 0.f));

	constexpr float pi = 3.14159265f;
	inline float to_rad(const float deg)
	{
		static constexpr float c = pi / 180.f;
		return deg * c;
	}

	// 4-component quaternion
	struct quat : vec_impl::xyzw_vec_base
	{
		constexpr quat() = default;
		inline quat(const float x, const float y, const float z, const float w) : vec_impl::xyzw_vec_base(x, y, z, w) {}
		constexpr quat(const vec_impl::xyzw_vec_base& init) : vec_impl::xyzw_vec_base(init) {}

		const static quat identity;
	};

	static inline quat to_quat(const float angle, const ndir& axis)
	{
		const float half_alpha = angle * .5f;
		const float sin_half_alpha = std::sin(half_alpha);
		const float cos_half_alpha = std::cos(half_alpha);
		const vec_impl::vec_base qv = vec_impl::mul(axis, vec_impl::make(sin_half_alpha));
		return quat(vec_impl::get_comp(qv, 0), vec_impl::get_comp(qv, 1), vec_impl::get_comp(qv, 2), cos_half_alpha);
	}

	static inline quat to_quat(const float x, const float y, const float z)
	{
		const float cr = std::cos(x * .5f), sr = std::sin(x * .5f);
		const float cp = std::cos(y * .5f), sp = std::sin(y * .5f);
		const float cy = std::cos(z * .5f), sy = std::sin(z * .5f);

		quat q;
		q.x() = sr * cp * cy - cr * sp * sy;
		q.y() = cr * sp * cy + sr * cp * sy;
		q.z() = cr * cp * sy - sr * sp * cy;
		q.w() = cr * cp * cy + sr * sp * sy;
		return q;
	}

	// TODO: add scale
	struct transform
	{
		inline transform() = default;
		inline transform(const pos& translation_, const quat& rotation_) : translation(translation_), rotation(rotation_) {}

		transform operator*(const transform& other) const
		{
			transform res;
			//const vec_impl::vec_base c = vec_impl::vqmul(other.rotation, translation);
			//res.translation = pos(vec_impl::add(c, other.translation));
			const vec_impl::vec_base c = vec_impl::vqmul(rotation, other.translation);
			res.translation = pos(vec_impl::add(c, translation));
			res.rotation = quat(vec_impl::qmul(other.rotation, rotation));
			return res;
		}

		pos operator*(const pos& other) const
		{
			const vec_impl::vec_base rotated = vec_impl::vqmul(rotation, other);
			return pos(vec_impl::add(rotated, translation));
		}

		pos translation;
		quat rotation;

		const static transform identity;
	};

	// TODO: not optimized
	mat to_mat(const transform& transform);

	// 4-component color TODO: make 16 bit?
	struct color : vec_impl::rgba_vec_base {};

	// 16 bit alignment allocator
	template<typename T>
	class simd_allocator : public std::allocator<T> 
	{
	public:
		using size_type = size_t;
		using pointer = T*;
		using const_pointer = const T*;
		
		constexpr simd_allocator() noexcept : std::allocator<T>() {}
		constexpr simd_allocator(const simd_allocator& a) noexcept : std::allocator<T>(a) {}
		template<class U>
		constexpr simd_allocator(const simd_allocator<U>& a) noexcept : std::allocator<T>(a) {}

		template < typename _Tp1>
		struct rebind 
		{
			using other = simd_allocator<_Tp1>;
		};
		
		static constexpr size_t simd_alignment = 16;

		[[nodiscard]]
		pointer allocate(size_type n) 
		{
			return reinterpret_cast<pointer>(_aligned_malloc(n * sizeof(size_type), simd_alignment));
		}
		
		void deallocate(pointer p, size_type)
		{ 
			_aligned_free(p);
		}
	};
}
}