#pragma once

#include <cstdint>
#include <cmath>
#include <memory>

#if !defined(_MSC_VER) || _MSC_VER < 1900  // MSVC 2015
#error ur compiler does not pass the vibe check
#endif

#include <malloc.h>
#include <immintrin.h>

// WIP math library
// SIMD instructions not needed with modern compiler optimizations but they look cool and I want practice with optimizing
// most of these still need to be tested for speed though

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

		static constexpr float& get_comp(const vec_base& vec, const uint32_t idx)
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

		struct xyz_vec_base: public vec_base
		{
			constexpr xyz_vec_base() = default;
			constexpr xyz_vec_base(const vec_base& init) : vec_base(init) {}
			inline xyz_vec_base(const float x) : vec_base(make(x)) {}
			inline xyz_vec_base(const float x, const float y, const float z) : vec_base(make(x, y, z, 0.f)) {}

			constexpr float& x() { return vec_impl::get_comp(data, 0); }
			constexpr float& y() { return vec_impl::get_comp(data, 1); }
			constexpr float& z() { return vec_impl::get_comp(data, 2); }
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

			constexpr float& x() { return vec_impl::get_comp(data, 0); }
			constexpr float& y() { return vec_impl::get_comp(data, 1); }
			constexpr float& z() { return vec_impl::get_comp(data, 2); }
			constexpr float& w() { return vec_impl::get_comp(data, 3); }
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
			
			constexpr float& r() { return vec_impl::get_comp(data, 0); }
			constexpr float& g() { return vec_impl::get_comp(data, 1); }
			constexpr float& b() { return vec_impl::get_comp(data, 2); }
			constexpr float& a() { return vec_impl::get_comp(data, 3); }
			constexpr float r() const { return vec_impl::get_comp(data, 0); }
			constexpr float g() const { return vec_impl::get_comp(data, 1); }
			constexpr float b() const { return vec_impl::get_comp(data, 2); }
			constexpr float a() const { return vec_impl::get_comp(data, 3); }
		};
	} // namespace vec_impl

	namespace mat_impl
	{
		union mat_base
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
	} // namespace mat_impl

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
		dir operator+(const dir& d) const;
		dir operator-(const dir& d) const;
		dir operator-() const;
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

	private:
		mat_impl::mat_base base_;
	};

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
	};

	// TODO: not efficient at all
	static inline quat to_quat(const float angle, const ndir& axis)
	{
		const float half_alpha = angle * .5f;
		const vec_impl::vec_base beta = axis;
		vec_impl::get_comp(beta, 3) = half_alpha;
		_mm_shuffle_ps(beta, beta, _MM_SHUFFLE(3, 0, 1, 2));
		const vec_impl::vec_base sin_half_alpha = vec_impl::make(std::sin(half_alpha));
		vec_impl::get_comp(sin_half_alpha, 0) = 1.f;
		const vec_impl::vec_base cos_beta = vec_impl::cos(beta);
		return quat(vec_impl::mul(sin_half_alpha, cos_beta));
	}

	// TODO: add scale
	struct transform
	{
		inline transform(const pos& translation_, const quat& rotation_) : translation(translation_), rotation(rotation_) {}

		pos translation;
		quat rotation;
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