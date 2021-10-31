#pragma once

#include <vector>
#include <memory>

#include "../util.hpp"
#include "../rhi/rhi.hpp"

#define AST_TYPE_KIND \
	T(real) \
	T(integer) \
	T(vector) \
	T(matrix) \
	T(none)

#define AST_NON_IMM_NODE_KIND \
    T(call) \
	T(let) \
	T(function)

#define AST_IMM_NODE_KIND \
    T(constant) \
    T(uniform) \
    T(vtx_attrib) \
	T(variable) 

namespace xs
{

// shitty ast stuff
namespace ast
{

struct named_binding
{
	named_binding(const std::string_view str_) : 
		str(str_), num(0)
	{
		static std::uint32_t counter = 0;
		num = counter++;
	}

	const char* c_str() const { return str.c_str(); }

	std::string str;
	std::uint32_t num;
};

struct named_binding_hash
{
	std::size_t operator()(const named_binding& b) const
	{
		return std::hash<std::string>{}(b.str) ^ b.num;
	}
};

struct named_binding_equal
{
	bool operator()(const named_binding& a, const named_binding& b) const
	{
		return a.num == b.num && a.str == b.str;
	}
};

struct unnamed_binding
{
	unnamed_binding(const std::string_view str_) :
		num(0)
	{
		static std::uint32_t counter = 0;
		num = counter++;
	}

	const char* c_str() const { return nullptr; }

	std::uint32_t num;
};

using unnamed_binding_hash = std::hash<std::uint32_t>;
using unnamed_binding_equal = std::equal_to<std::uint32_t>;

using binding = named_binding;
using binding_hash = named_binding_hash;
using binding_equal = named_binding_equal;

enum class type_kind
{
#define T(k) k,
	AST_TYPE_KIND
#undef T
};

struct type
{
	type_kind kind;
	
	type(const type_kind kind_) : 
		kind(kind_) 
	{}

	virtual ~type() = default;

	template<typename T>
	const T* as() const { return static_cast<const T*> (this);  }
};

namespace types
{
	struct integer : public type
	{
		std::uint32_t width;
		bool has_sign;
		integer(const std::uint32_t width_, const bool has_sign_) : 
			type(type_kind::integer), width(width_), has_sign(has_sign_)
		{}
	};

	struct real : public type
	{
		std::uint32_t width;
		real(const std::uint32_t width_) :
			type(type_kind::real), width(width_)
		{}
	};

	struct vector : public type
	{
		const type* comp_type;
		std::uint32_t size;
		vector(const type* comp_type_, const std::uint32_t size_) : 
			type(type_kind::vector), comp_type(comp_type_), size(size_) 
		{}
	};

	struct matrix : public type
	{
		const type* comp_type;
		std::uint32_t cols;
		std::uint32_t rows;
		matrix(const type* comp_type_, const std::uint32_t cols_, const std::uint32_t rows_) :
			type(type_kind::matrix), comp_type(comp_type_), cols(cols_), rows(rows_) 
		{}
	};

	struct none : public type
	{
		none() : type(type_kind::none) {}
	};
}

enum class node_kind
{
#define T(k) k,
	AST_IMM_NODE_KIND
	AST_NON_IMM_NODE_KIND
#undef T
};

struct node
{
	node_kind kind;
	const type* return_type;

	node(const node_kind kind_, const type* return_type_) : 
		kind(kind_), return_type(return_type_)
	{}

	virtual ~node() = default;

	template<typename T>
	const T* as() const { return static_cast<const T*> (this); }
};

namespace nodes
{
	template<typename T>
	struct constant_view : public node
	{
		const T* payload;

		template<typename U>
		U data_cast() const { return *((U*)payload); }

		template<typename U>
		constant_view(const type* type_, const U& payload_) :
			node(node_kind::constant, type_), payload((T*)&payload_)
		{}
	};

	using constant = constant_view<std::uint8_t>;

	struct uniform : public node
	{
		uniform(const type* type_) : node(node_kind::uniform, type_) {}
	};

	struct vtx_attrib : public node
	{
		vtx_attrib(const type* type_) : node(node_kind::vtx_attrib, type_) {}
	};

	struct variable : public node
	{
		binding name;
		variable(const type* type_, const binding& name_) :
			node(node_kind::variable, type_), name(name_)
		{}
	};

	struct call : public node
	{
		named_binding name;
		std::vector<std::unique_ptr<node>> args;

		call(const type* return_type_, const named_binding& name_, std::vector<std::unique_ptr<node>> args_) :
			node(node_kind::call, return_type_), name(name_), args(std::move(args_))
		{}
	};

	struct let : public node
	{
		binding name;
		std::unique_ptr<node> before;
		std::unique_ptr<node> after;

		let(const type* type_, const binding& name_, std::unique_ptr<node> before_, std::unique_ptr<node> after_) :
			node(node_kind::let, type_), name(name_), before(std::move(before_)), after(std::move(after_))
		{}
	};

	struct function : public node
	{
		named_binding name;
		std::unique_ptr<node> body;
		std::vector<variable> params;

		function(const type* return_type_, const binding& name_, std::unique_ptr<node> body_, std::vector<variable> params_) :
			node(node_kind::function, return_type_), name(name_), body(std::move(body_)), params(std::move(params_))
		{}
	};
}

}

class script_context
{
public:
	//script_context() = default;
	script_context(rhi::context& ctx);
	~script_context();

	std::uint64_t add(fvector<std::uint32_t> spirv);
	std::unique_ptr<ast::node> parse(const std::string_view filename);
	fvector<std::uint32_t> compile(std::unique_ptr<ast::node> root);
	std::vector<std::uint32_t> link(const std::vector<std::uint64_t>& module_ids, const bool optimize = true);

	std::function<void(std::string_view)> logger_;

//private:	
	std::unique_ptr<struct tools_impl> tools_;

};

}

