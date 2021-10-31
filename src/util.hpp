#include <utility>
#include <cassert>
#include <cstdlib>
#include <span>
#include <bit>
#include <string>
#include <fstream>

#define assert_not_implemented assert(false);
#define assert_not_reached assert(false);

namespace xs
{

template<typename T>
concept Alloc = requires(T&& a)
{
	a.data();
	a.size();
};

template<typename T, std::size_t N>
concept SizedAllocMgr = requires(T&& m)
{
	m.alloc(N);
};

template<typename T>
concept SingleAllocMgr = requires(T&& m)
{
	m.alloc();
};

template<typename T, typename U>
concept ReAllocMgr = requires(T&& m, U&& a)
{
	m.realloc(a, 5);
};

template<typename T>
concept OwningAllocMgr = T::owning_allocs;

template<typename T>
concept NonOwningAllocMgr = !T::owning_allocs;

template<typename T, std::size_t N, std::size_t alignment = alignof(T)>
class stack_alloc
{
public:
	stack_alloc(const std::size_t size = N) {}

	T* data() { return data_; }
	const T* data() const { return data_; }

	std::size_t size() const { return N; }

private:
	alignas(alignment) T data_[N];
};

template<typename T, std::size_t N>
void swap(stack_alloc<T, N>& a, stack_alloc<T, N>& b)
{
	std::swap(a.data_, b.data_);
}

static_assert(Alloc<stack_alloc<std::uint32_t, 1>>);
static_assert(std::is_swappable_v<stack_alloc<std::uint32_t, 1>>);

template<typename T, std::size_t alignment = alignof(T)>
class stack_alloc_mgr
{
public:
	template<std::size_t N>
	using alloc_type = stack_alloc<T, N>;
	static constexpr bool owning_allocs = true;

	template<std::size_t N>
	constexpr stack_alloc<T, N, alignment> alloc(const std::size_t size = N)
	{
		static_assert(size == N);
		return stack_alloc<T, N, alignment>();
	}
};

template<typename T, bool owning, std::size_t alignment>
class heap_alloc_mgr;

template<typename T, bool owning, std::size_t alignment = alignof(T)>
class heap_alloc
{
public:

	friend heap_alloc_mgr<T, owning, alignment>;

	template<typename T2, bool owning2, std::size_t alignment2>
	friend void swap(heap_alloc<T2, owning2, alignment2>& a, heap_alloc<T2, owning2, alignment2>& b);

	heap_alloc() : 
		data_(nullptr), size_(0) 
	{}

	heap_alloc(const std::size_t size) :
		//data_(reinterpret_cast<T*>(_aligned_malloc(size * sizeof(T), alignment))),
		data_(reinterpret_cast<T*>(std::malloc(size * sizeof(T)))),
		size_(size)
	{}

	~heap_alloc()
	{
		if constexpr (owning)
		{
			//_aligned_free(data_);
			std::free(data_);
		}
	}

	T* data() { return data_; }
	const T* data() const { return data_; }

	std::size_t size() const { return size_; }

private:
	T* data_;
	std::size_t size_;
};


template<typename T, bool owning, std::size_t alignment>
void swap(heap_alloc<T, owning, alignment>& a, heap_alloc<T, owning, alignment>& b)
{
	std::swap(a.data_, b.data_);
	std::swap(a.size_, b.size_);
}

template<typename T, std::size_t alignment = alignof(T)>
using owning_heap_alloc = heap_alloc<T, true, alignment>;

template<typename T, std::size_t alignment = alignof(T)>
using nonowning_heap_alloc = heap_alloc<T, false, alignment>;

static_assert(std::is_swappable_v<owning_heap_alloc<std::uint32_t>>);
static_assert(std::is_swappable_v<nonowning_heap_alloc<std::uint32_t>>);

template<typename T, bool owning, std::size_t alignment = alignof(T)>
class heap_alloc_mgr
{
public:
	using alloc_type = heap_alloc<T, owning>;
	static constexpr bool owning_allocs = owning;

	alloc_type alloc(const std::size_t size = 1) const
	{
		return heap_alloc<T, owning, alignment>(size);
	}

	void realloc(alloc_type& old_alloc, const std::size_t size) const
	{
		//T* ptr = reinterpret_cast<T*>(_aligned_realloc(old_alloc.data_, size, alignment));
		T* ptr = reinterpret_cast<T*>(std::realloc((void*)old_alloc.data_, size * sizeof(T)));
		old_alloc.data_ = ptr;
		old_alloc.size_ = size;
	}
};

template<typename T>
using owning_heap_alloc_mgr = heap_alloc_mgr<T, true>;

template<typename T>
using nonowning_heap_alloc_mgr = heap_alloc_mgr<T, false>;

template<typename T, std::size_t alignment>
class heap_alloc_mgr<T, false, alignment>
{
	void free(heap_alloc<T, false, alignment>& alloc)
	{
		//_aligned_free(alloc.data());
		std::free(alloc.data());
	}
};

template<typename T, std::size_t block_size>
class pool_alloc_mgr
{
public:

	using alloc_type = T*;
	static constexpr bool owning_allocs = false;

	alloc_type alloc()
	{
		if (free_list_)
		{
			free_item* cur = free_list_;
			free_list_ = cur->next;
			return (T*) cur;
		}
		
		if (next_ == block_size)
		{
			block* cur = new block;
			next_ = 0;

			cur->next = block_list_;
			block_list_ = cur;
		}

		T* res = &block_list_->items[next_];
		next_++;

		return res;
	}

	void free(alloc_type& data)
	{
		data->T::~T();
		free_item* cur = (free_item*)data;
		cur->next = free_list_;
		free_list_ = cur;
	}

private:
	
	struct free_item
	{
		free_item* next;
	};

	static_assert(sizeof(T) >= sizeof(free_item));

	struct block
	{
		block* next;
		T items[block_size];
	};

	block* block_list_;
	free_item* free_list_;
	std::size_t next_;
};

template<typename T>
class scope_alloc_mgr_t
{
public:
	using alloc_type = std::span<T>;
	static constexpr bool owning_allocs = true;

	friend class scope_stack;

	scope_alloc_mgr_t() :
		base_(nullptr),
		sp_(nullptr),
		next_(nullptr)
	{
	}

	scope_alloc_mgr_t(std::uint8_t* sp) :
		base_(sp),
		sp_(sp),
		next_(nullptr)
	{}

	template<typename U> 
	scope_alloc_mgr_t<U>& as()
	{
		return *reinterpret_cast<scope_alloc_mgr_t<U>*>(this);
	}

	alloc_type alloc(const std::size_t size)
	{
		T* old = (T*)sp_;
		sp_ += size * sizeof(T);
		return std::span<T>(old, size);
	}

private:
	std::uint8_t* base_;
	std::uint8_t* sp_;
	scope_alloc_mgr_t<std::uint8_t>* next_;
};

using scope_alloc_mgr = scope_alloc_mgr_t<std::uint8_t>;

// TODO: handle different alignments
class scope_stack
{
public:
	
	scope_stack(const std::size_t size) : 
		alloc_(heap_alloc_mgr<std::uint8_t, true>().alloc(size)),
		scopes_()
	{
		assert(size > sizeof(scope_alloc_mgr));

		scope_alloc_mgr* init_ptr = reinterpret_cast<scope_alloc_mgr*>(alloc_.data());
		std::uint8_t* root_sp = std::next(alloc_.data(), sizeof(scope_alloc_mgr));
		scopes_ = new (init_ptr) scope_alloc_mgr(root_sp);
	}

	scope_stack(const scope_stack&) = delete;
	scope_stack& operator=(const scope_stack&) = delete;

	void enter()
	{
		scope_alloc_mgr* init_ptr = reinterpret_cast<scope_alloc_mgr*>(scopes_->sp_);
		std::uint8_t* next_sp = std::next(scopes_->sp_, sizeof(scope_alloc_mgr));
		scope_alloc_mgr* cur = new (init_ptr) scope_alloc_mgr(next_sp);
		cur->next_ = scopes_;
		scopes_ = cur;
	}

	void exit()
	{
		scope_alloc_mgr* next = scopes_->next_;
		delete scopes_;
		scopes_ = next;
	}

	template<typename T>
	scope_alloc_mgr_t<T>& scope()
	{
		return scopes_->as<T>();
	}

	template<typename FuncType>
	void operator || (FuncType&& fn) {
		enter();
		fn(*scopes_);
		exit();
	}

private:
	heap_alloc<std::uint8_t, true> alloc_;
	scope_alloc_mgr* scopes_;
};

template<typename ChildType>
class vector_base
{
public:
	
	vector_base() {}

	auto at(const std::size_t idx) const 
	{
		assert(idx >= 0 && idx < as_child()->size());
		return *std::next(as_child()->data(), idx); 
	}

	auto at(const std::size_t idx) 
	{ 
		assert(idx >= 0 && idx < as_child()->size());
		return *std::next(as_child()->data(), idx); 
	}

	auto operator[](const std::size_t idx) const { return *std::next(as_child()->data(), idx); }
	auto& operator[](const std::size_t idx) { return *std::next(as_child()->data(), idx); }

	bool operator==(const vector_base& other) const
	{
		return std::equal(cbegin(), cend(), other.cbegin());
	}

	auto cbegin() const { return as_child()->data(); }
	auto begin() { return as_child()->data(); }
	auto begin() const { return as_child()->data(); }

	auto cend() const { return std::next(as_child()->data(), as_child()->size()); }
	auto end() { return std::next(as_child()->data(), as_child()->size()); }
	auto end() const { return std::next(as_child()->data(), as_child()->size()); }

private:
	const ChildType* as_child() const { return static_cast<const ChildType*>(this); }
	ChildType* as_child() { return static_cast<ChildType*>(this); }
};


template<typename T, typename AllocMgr = owning_heap_alloc_mgr<T>>
class fvector;

template<typename T, typename AllocMgr>
void swap(fvector<T, AllocMgr>& a, fvector<T, AllocMgr>& b)
{
	using std::swap;
	swap(a.alloc_, b.alloc_);
	swap(a.size_, b.size_);
}

template<typename T, typename AllocMgr = owning_heap_alloc_mgr<T>>
class fvector : public vector_base<fvector<T, AllocMgr>>
{
public:
	using value_type = T;
	using alloc_type = AllocMgr::alloc_type;

	using parent_type = vector_base<fvector<T, AllocMgr>>;
	using size_type = std::size_t;
	using rvalue_reference = T&&;
	using const_reference = const T&;
	using pointer = T*;
	using const_pointer = const T*;

	template<typename T_, typename AllocMgr_>
	friend void swap(fvector<T_, AllocMgr_>& a, fvector<T_, AllocMgr_>& b);

	fvector() :
		alloc_(),
		size_(0)
	{}

	fvector(const std::size_t capacity, const AllocMgr& alloc_mgr = AllocMgr()) :
		alloc_(alloc_mgr.alloc(capacity)),
		size_(0)
	{}

	fvector(const fvector& other, const AllocMgr& alloc_mgr = AllocMgr()) :
		alloc_(alloc_mgr.alloc(other.capacity())),
		size_(other.size())
	{
		std::copy(other.alloc_.data(), std::next(other.alloc_.data(), size()), alloc_.data());
	}

	fvector(fvector&& other, const AllocMgr& alloc_mgr = AllocMgr()) :
		alloc_(),
		size_(0)
	{
		std::swap(alloc_, other.alloc_);
		std::swap(size_, other.size_);
	}

	fvector(const std::size_t capacity, AllocMgr& alloc_mgr) :
		alloc_(alloc_mgr.alloc(capacity)),
		size_(0)
	{}

	fvector(const fvector& other, AllocMgr& alloc_mgr) :
		alloc_(alloc_mgr.alloc(other.capacity())),
		size_(other.size())
	{
		std::copy(other.alloc_.data(), std::next(other.alloc_.data(), size()), alloc_.data());
	}

	fvector(fvector&& other, AllocMgr& alloc_mgr) :
		alloc_(),
		size_(0)
	{
		std::swap(alloc_, other.alloc_);
		std::swap(size_, other.size_);
	}

	~fvector()
	{
		for (std::size_t i = 0; i < size(); i++)
		{
			data()[i].~T();
		}
	}

	fvector& operator=(const fvector& other)
	{
		assign(other);
		return *this;
	}

	fvector& operator=(fvector&& other)
	{
		assign(std::move(other));
		return *this;
	}

	void assign(const fvector& other, AllocMgr alloc_mgr = AllocMgr())
	{
		fvector tmp = fvector(other, alloc_mgr);
		using std::swap;
		swap(*this, tmp);
	}

	void assign(fvector&& other, AllocMgr alloc_mgr = AllocMgr())
	{
		fvector tmp = fvector(std::move(other), alloc_mgr);
		using std::swap;
		swap(*this, tmp);
	}

	void push_back(rvalue_reference elem)
	{
		size_type back = size_;
		size_++;

		assert(size_ <= capacity());
		new (&data()[back]) T(std::move(elem));
	}

	void push_back(const_reference elem)
	{
		size_type back = size_;
		size_++;

		assert(size_ <= capacity());
		new (&data()[back]) T(elem);
	}

	const_pointer data() const { return alloc_.data(); }
	pointer data() { return alloc_.data(); }

	size_type size() const { return size_; }
	size_type capacity() const { return alloc_.size(); }

	void set_size_unsafe(const std::size_t size)
	{
		assert(size <= capacity());
		size_ = size;
	}

	std::span<value_type> span() const { return std::span<value_type>{ data(), size() }; }
	std::span<value_type> span() { return std::span<value_type>{ data(), size() }; }

private:
	alloc_type alloc_;
	size_type size_;
};

/*
template<typename T, std::size_t N, typename AllocMgr = stack_alloc_mgr<T>>
class sfvector : public vector_base<sfvector<T, N, AllocMgr>>
{
public:
	using value_type = T;
	using alloc_type = AllocMgr::alloc_type;

	using parent_type = vector_base<sfvector<T, N, AllocMgr>>;
	using size_type = std::size_t;
	using rvalue_reference = T&&;
	using const_reference = const T&;
	using pointer = T*;
	using const_pointer = const T*;

	sfvector() :
		alloc_(),
		size_(0)
	{}

	sfvector(const sfvector& other, AllocMgr& alloc_mgr = stack_alloc_mgr<T>()) :
		alloc_(alloc_mgr.alloc<N>(other.capacity())),
		size_(other.size())
	{
		std::copy(other.alloc_.data(), std::next(other.alloc_.data(), size()), alloc_.data());
	}

	~sfvector()
	{
		for (std::size_t i = 0; i < size(); i++)
		{
			data()[i].~T();
		}
	}

	void push_back(rvalue_reference elem)
	{
		size_type back = size_;
		size_++;

		assert(size_ < capacity());
		at(back) = std::move(elem);
	}

	void push_back(const_reference elem)
	{
		size_type back = size_;
		size_++;

		assert(size_ < capacity());
		at(back) = elem;
	}

	const_pointer data() const { return alloc_.data(); }
	pointer data() { return alloc_.data(); }

	size_type size() const { return size_; }
	size_type capacity() const { return alloc_.size(); }

	std::span<value_type> span() const { return std::span<value_type, N>{ data() }; }
	std::span<value_type> span() { return std::span<value_type, N>{ data() }; }

	void set_size_unsafe(const std::size_t size)
	{
		assert(size <= capacity());
		size_ = size;
	}

private:
	alloc_type alloc_;
	size_type size_;
};*/

template<typename T, typename AllocMgr = owning_heap_alloc_mgr<T>>
class dvector : public vector_base<dvector<T, AllocMgr>>
{
public:
	using value_type = T;
	using alloc_type = AllocMgr::alloc_type;

	using parent_type = vector_base<dvector<T, AllocMgr>>;
	using size_type = std::size_t;
	using rvalue_reference = T&&;
	using const_reference = const T&;
	using pointer = T*;
	using const_pointer = const T*;

	template<typename T_, typename AllocMgr_>
	friend void swap(dvector<T_, AllocMgr_>& a, dvector<T_, AllocMgr_>& b);

	dvector() :
		alloc_(),
		size_(0),
		alloc_mgr_()
	{}

	dvector(const size_type capacity, AllocMgr& alloc_mgr = AllocMgr()) :
		alloc_(capacity == 0 ? alloc_type() : alloc_mgr.alloc(capacity)),
		size_(0),
		alloc_mgr_(alloc_mgr)
	{}

	dvector(const dvector& other, AllocMgr& alloc_mgr = AllocMgr()) :
		alloc_(alloc_mgr.alloc(other.capacity())),
		size_(other.size()),
		alloc_mgr_(alloc_mgr)
	{
		std::copy(other.alloc_.data(), std::next(other.alloc_.data(), size()), alloc_.data());
	}

	dvector(dvector&& other) :
		alloc_(),
		size_(0),
		alloc_mgr_(other.alloc_mgr_)
	{
		std::swap(alloc_, other.alloc_);
		std::swap(size_, other.size_);
	}

	~dvector()
	{
		for (std::size_t i = 0; i < size(); i++)
		{
			data()[i].~T();
		}
	}

	dvector& operator=(const dvector& other) = delete;
	dvector& operator=(dvector&& other) = delete;

	void push_back(rvalue_reference elem)
	{
		size_type back = size_;

		if (size_ + 1 >= capacity())
		{
			reserve(size_ * 2 + 1);
		}

		size_++;

		assert(size_ <= capacity());
		new (alloc_.data() + back) T(std::move(elem));
	}

	void push_back(const_reference elem)
	{
		size_type back = size_;

		if (size_ + 1 >= capacity())
		{
			reserve(size_ * 2 + 1);
		}

		size_++;

		assert(size_ <= capacity());
		new (alloc_.data() + back) T(elem);
	}

	void reserve(const size_type capacity)
	{
		alloc_mgr_.realloc(alloc_, capacity);
	}

	const_pointer data() const { return alloc_.data(); }
	pointer data() { return alloc_.data(); }

	size_type size() const { return size_; }
	size_type capacity() const { return alloc_.size(); }

	std::span<value_type> span() const { return std::span<value_type>{ data(), size() }; }
	std::span<value_type> span() { return std::span<value_type>{ data(), size() }; }

private:
	alloc_type alloc_;
	size_type size_;
	AllocMgr alloc_mgr_;
};

template<typename T, typename AllocMgr>
void swap(dvector<T, AllocMgr>& a, dvector<T, AllocMgr>& b)
{
	using std::swap;
	swap(a.alloc_, b.alloc_);
	swap(a.size_, b.size_);
}

namespace util
{
	template<typename Container1, typename Container2>
	void push(Container1& pushee, const Container2& pusher)
	{
		for (const auto& elem : pusher)
		{
			pushee.push_back(elem);
		}
	}

	template<typename Container, typename T>
	void copy_n(Container& pushee, const T& v, const std::size_t n)
	{
		for (std::size_t i = 0; i < n; i++)
		{
			pushee.push_back(v);
		}
	}

	template<typename T> requires std::is_integral_v<T>
	constexpr static T div_round_up(const T x, const T y)
	{
		return x == 0 ? 0 : 1 + ((std::make_signed_t<T>(x) - 1) / std::make_signed_t<T>(y));
	}

	template<typename T>
	constexpr std::size_t bitsof()
	{
		static constexpr std::size_t bits_in_byte = 8;
		return sizeof(T) * bits_in_byte;
	}

	// TODO: move to util.cpp
	static std::string read_file(const std::string_view filename)
	{
		std::ifstream file(filename.data(), std::ios::ate | std::ios::binary);

		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open file!");
		}

		std::size_t file_size = (std::size_t)file.tellg();
		std::string buffer = std::string(file_size, ' ');
		file.seekg(0);
		file.read(buffer.data(), file_size);
		file.close();

		return buffer;
	}

	// float->half variants.
	// by Fabian "ryg" Giesen.
	//
	// I hereby place this code in the public domain, as per the terms of the
	// CC0 license:
	//
	//   https://creativecommons.org/publicdomain/zero/1.0/
	//
	// float_to_half_full: This is basically the ISPC stdlib code, except
	// I preserve the sign of NaNs (any good reason not to?)
	//
	// float_to_half_fast: Almost the same, with some unnecessary cases cut.
	//
	// float_to_half_fast2: This is where it gets a bit weird. See lengthy
	// commentary inside the function code. I'm a bit on the fence about two
	// things:
	// 1. This *will* behave differently based on whether flush-to-zero is
	//    enabled or not. Is this acceptable for ISPC?
	// 2. I'm a bit on the fence about NaNs. For half->float, I opted to extend
	//    the mantissa (preserving both qNaN and sNaN contents) instead of always
	//    returning a qNaN like the original ISPC stdlib code did. For float->half
	//    the "natural" thing would be just taking the top mantissa bits, except
	//    that doesn't work; if they're all zero, we might turn a sNaN into an
	//    Infinity (seriously bad!). I could test for this case and do a sticky
	//    bit-like mechanism, but that's pretty ugly. Instead I go with ISPC
	//    std lib behavior in this case and just return a qNaN - not quite symmetric
	//    but at least it's always safe. Any opinions?
	//
	// I'll just go ahead and give "fast2" the same treatment as my half->float code,
	// but if there's concerns with the way it works I might revise it later, so watch
	// this spot.
	//
	// float_to_half_fast3: Bitfields removed. Ready for SSE2-ification :)
	//
	// approx_float_to_half: Simpler (but less accurate) version that matches the Fox
	// toolkit float->half conversions: http://blog.fox-toolkit.org/?p=40 - note that this
	// also (incorrectly) translates some sNaNs into infinity, so be careful!
	//
	// float_to_half_rtne_full: Unoptimized round-to-nearest-break-ties-to-even reference
	// implementation.
	// 
	// float_to_half_fast3_rtne: Variant of float_to_half_fast3 that performs round-to-
	// nearest-even.
	
	typedef unsigned int uint;

	union FP32
	{
		uint u;
		float f;
		struct
		{
			uint Mantissa : 23;
			uint Exponent : 8;
			uint Sign : 1;
		};
	};

	union FP16
	{
		unsigned short u;
		struct
		{
			uint Mantissa : 10;
			uint Exponent : 5;
			uint Sign : 1;
		};
	};

	// Original ISPC reference version; this always rounds ties up.
	static FP16 float_to_half_full(const float ff)
	{
		FP32 f;
		f.f = ff;

		FP16 o = { 0 };

		// Based on ISPC reference code (with minor modifications)
		if (f.Exponent == 0) // Signed zero/denormal (which will underflow)
			o.Exponent = 0;
		else if (f.Exponent == 255) // Inf or NaN (all exponent bits set)
		{
			o.Exponent = 31;
			o.Mantissa = f.Mantissa ? 0x200 : 0; // NaN->qNaN and Inf->Inf
		}
		else // Normalized number
		{
			// Exponent unbias the single, then bias the halfp
			int newexp = f.Exponent - 127 + 15;
			if (newexp >= 31) // Overflow, return signed infinity
				o.Exponent = 31;
			else if (newexp <= 0) // Underflow
			{
				if ((14 - newexp) <= 24) // Mantissa might be non-zero
				{
					uint mant = f.Mantissa | 0x800000; // Hidden 1 bit
					o.Mantissa = mant >> (14 - newexp);
					if ((mant >> (13 - newexp)) & 1) // Check for rounding
						o.u++; // Round, might overflow into exp bit, but this is OK
				}
			}
			else
			{
				o.Exponent = newexp;
				o.Mantissa = f.Mantissa >> 13;
				if (f.Mantissa & 0x1000) // Check for rounding
					o.u++; // Round, might overflow to inf, this is OK
			}
		}

		o.Sign = f.Sign;
		return o;
	}

	// Same as above, but with full round-to-nearest-even.
	static FP16 float_to_half_full_rtne(const float ff)
	{
		FP32 f;
		f.f = ff;

		FP16 o = { 0 };

		// Based on ISPC reference code (with minor modifications)
		if (f.Exponent == 0) // Signed zero/denormal (which will underflow)
			o.Exponent = 0;
		else if (f.Exponent == 255) // Inf or NaN (all exponent bits set)
		{
			o.Exponent = 31;
			o.Mantissa = f.Mantissa ? 0x200 : 0; // NaN->qNaN and Inf->Inf
		}
		else // Normalized number
		{
			// Exponent unbias the single, then bias the halfp
			int newexp = f.Exponent - 127 + 15;
			if (newexp >= 31) // Overflow, return signed infinity
				o.Exponent = 31;
			else if (newexp <= 0) // Underflow
			{
				if ((14 - newexp) <= 24) // Mantissa might be non-zero
				{
					uint mant = f.Mantissa | 0x800000; // Hidden 1 bit
					uint shift = 14 - newexp;
					o.Mantissa = mant >> shift;

					uint lowmant = mant & ((1 << shift) - 1);
					uint halfway = 1 << (shift - 1);

					if (lowmant >= halfway) // Check for rounding
					{
						if (lowmant > halfway || (o.Mantissa & 1)) // if above halfway point or unrounded result is odd
							o.u++; // Round, might overflow into exp bit, but this is OK
					}
				}
			}
			else
			{
				o.Exponent = newexp;
				o.Mantissa = f.Mantissa >> 13;
				if (f.Mantissa & 0x1000) // Check for rounding
				{
					if (((f.Mantissa & 0x1fff) > 0x1000) || (o.Mantissa & 1)) // if above halfway point or unrounded result is odd
						o.u++; // Round, might overflow to inf, this is OK
				}
			}
		}

		o.Sign = f.Sign;
		return o;
	}

	static FP16 float_to_half_fast(const float ff)
	{
		FP32 f;
		f.f = ff;

		FP16 o = { 0 };

		// Based on ISPC reference code (with minor modifications)
		if (f.Exponent == 255) // Inf or NaN (all exponent bits set)
		{
			o.Exponent = 31;
			o.Mantissa = f.Mantissa ? 0x200 : 0; // NaN->qNaN and Inf->Inf
		}
		else // Normalized number
		{
			// Exponent unbias the single, then bias the halfp
			int newexp = f.Exponent - 127 + 15;
			if (newexp >= 31) // Overflow, return signed infinity
				o.Exponent = 31;
			else if (newexp <= 0) // Underflow
			{
				if ((14 - newexp) <= 24) // Mantissa might be non-zero
				{
					uint mant = f.Mantissa | 0x800000; // Hidden 1 bit
					o.Mantissa = mant >> (14 - newexp);
					if ((mant >> (13 - newexp)) & 1) // Check for rounding
						o.u++; // Round, might overflow into exp bit, but this is OK
				}
			}
			else
			{
				o.Exponent = newexp;
				o.Mantissa = f.Mantissa >> 13;
				if (f.Mantissa & 0x1000) // Check for rounding
					o.u++; // Round, might overflow to inf, this is OK
			}
		}

		o.Sign = f.Sign;
		return o;
	}

	static FP16 float_to_half_fast2(const float ff)
	{
		FP32 infty = { 31 << 23 };
		FP32 magic = { 15 << 23 };
		FP16 o = { 0 };

		FP32 f;
		f.f = ff;

		uint sign = f.Sign;
		f.Sign = 0;

		// Based on ISPC reference code (with minor modifications)
		if (f.Exponent == 255) // Inf or NaN (all exponent bits set)
		{
			o.Exponent = 31;
			o.Mantissa = f.Mantissa ? 0x200 : 0; // NaN->qNaN and Inf->Inf
		}
		else // (De)normalized number or zero
		{
			f.u &= ~0xfff; // Make sure we don't get sticky bits
			// Not necessarily the best move in terms of accuracy, but matches behavior
			// of other versions.

			// Shift exponent down, denormalize if necessary.
			// NOTE This represents half-float denormals using single precision denormals.
			// The main reason to do this is that there's no shift with per-lane variable
			// shifts in SSE*, which we'd otherwise need. It has some funky side effects
			// though:
			// - This conversion will actually respect the FTZ (Flush To Zero) flag in
			//   MXCSR - if it's set, no half-float denormals will be generated. I'm
			//   honestly not sure whether this is good or bad. It's definitely interesting.
			// - If the underlying HW doesn't support denormals (not an issue with Intel
			//   CPUs, but might be a problem on GPUs or PS3 SPUs), you will always get
			//   flush-to-zero behavior. This is bad, unless you're on a CPU where you don't
			//   care.
			// - Denormals tend to be slow. FP32 denormals are rare in practice outside of things
			//   like recursive filters in DSP - not a typical half-float application. Whether
			//   FP16 denormals are rare in practice, I don't know. Whatever slow path your HW
			//   may or may not have for denormals, this may well hit it.
			f.f *= magic.f;

			f.u += 0x1000; // Rounding bias
			if (f.u > infty.u) f.u = infty.u; // Clamp to signed infinity if overflowed

			o.u = f.u >> 13; // Take the bits!
		}

		o.Sign = sign;
		return o;
	}

	static FP16 float_to_half_fast3(const float ff)
	{
		FP32 f32infty = { 255 << 23 };
		FP32 f16infty = { 31 << 23 };
		FP32 magic = { 15 << 23 };
		uint sign_mask = 0x80000000u;
		uint round_mask = ~0xfffu;
		FP16 o = { 0 };

		FP32 f;
		f.f = ff;

		uint sign = f.u & sign_mask;
		f.u ^= sign;

		// NOTE all the integer compares in this function can be safely
		// compiled into signed compares since all operands are below
		// 0x80000000. Important if you want fast straight SSE2 code
		// (since there's no unsigned PCMPGTD).

		if (f.u >= f32infty.u) // Inf or NaN (all exponent bits set)
			o.u = (f.u > f32infty.u) ? 0x7e00 : 0x7c00; // NaN->qNaN and Inf->Inf
		else // (De)normalized number or zero
		{
			f.u &= round_mask;
			f.f *= magic.f;
			f.u -= round_mask;
			if (f.u > f16infty.u) f.u = f16infty.u; // Clamp to signed infinity if overflowed

			o.u = f.u >> 13; // Take the bits!
		}

		o.u |= sign >> 16;
		return o;
	}

	// Same, but rounding ties to nearest even instead of towards +inf
	static FP16 float_to_half_fast3_rtne(const float ff)
	{
		FP32 f32infty = { 255 << 23 };
		FP32 f16max = { (127 + 16) << 23 };
		FP32 denorm_magic = { ((127 - 15) + (23 - 10) + 1) << 23 };
		uint sign_mask = 0x80000000u;
		FP16 o = { 0 };

		FP32 f;
		f.f = ff;

		uint sign = f.u & sign_mask;
		f.u ^= sign;

		// NOTE all the integer compares in this function can be safely
		// compiled into signed compares since all operands are below
		// 0x80000000. Important if you want fast straight SSE2 code
		// (since there's no unsigned PCMPGTD).

		if (f.u >= f16max.u) // result is Inf or NaN (all exponent bits set)
			o.u = (f.u > f32infty.u) ? 0x7e00 : 0x7c00; // NaN->qNaN and Inf->Inf
		else // (De)normalized number or zero
		{
			if (f.u < (113 << 23)) // resulting FP16 is subnormal or zero
			{
				// use a magic value to align our 10 mantissa bits at the bottom of
				// the float. as long as FP addition is round-to-nearest-even this
				// just works.
				f.f += denorm_magic.f;

				// and one integer subtract of the bias later, we have our final float!
				o.u = f.u - denorm_magic.u;
			}
			else
			{
				uint mant_odd = (f.u >> 13) & 1; // resulting mantissa is odd

				// update exponent, rounding bias part 1
				f.u += ((15 - 127) << 23) + 0xfff;
				// rounding bias part 2
				f.u += mant_odd;
				// take the bits!
				o.u = f.u >> 13;
			}
		}

		o.u |= sign >> 16;
		return o;
	}

	// Approximate solution. This is faster but converts some sNaNs to
	// infinity and doesn't round correctly. Handle with care.
	static FP16 approx_float_to_half(const float ff)
	{
		FP32 f32infty = { 255 << 23 };
		FP32 f16max = { (127 + 16) << 23 };
		FP32 magic = { 15 << 23 };
		FP32 expinf = { (255 ^ 31) << 23 };
		uint sign_mask = 0x80000000u;
		FP16 o = { 0 };

		FP32 f;
		f.f = ff;

		uint sign = f.u & sign_mask;
		f.u ^= sign;

		if (!(f.f < f32infty.u)) // Inf or NaN
			o.u = f.u ^ expinf.u;
		else
		{
			if (f.f > f16max.f) f.f = f16max.f;
			f.f *= magic.f;
		}

		o.u = f.u >> 13; // Take the mantissa bits
		o.u |= sign >> 16;
		return o;
	}

} // namespace util

} // namespace xs