#include "script.hpp"

#include <iostream>
#include <fstream>
#include <cassert>
#include <array>
#include <span>

#include "glslang/SPIRV/spirv.hpp"
#include "glslang/SPIRV/Logger.h"
#include "glslang/SPIRV/spvIR.h"

#include <spirv-tools/libspirv.hpp>
#include <spirv-tools/optimizer.hpp>
#include <spirv-tools/linker.hpp>


#define XS_SPIRV_VERSION_WORD(MAJOR, MINOR) \
  ((uint32_t(uint8_t(MAJOR)) << 16) | (uint32_t(uint8_t(MINOR)) << 8))

extern "C"
{
    extern FILE* yyin;
    extern int yyparse();
    typedef struct yy_buffer_state* YY_BUFFER_STATE;
    extern void yy_switch_to_buffer(YY_BUFFER_STATE);
    extern YY_BUFFER_STATE yy_create_buffer(FILE*, int);
}

namespace xs
{

namespace ast
{
    static inline bool is_imm(ast::node_kind kind)
    {
        return kind == node_kind::constant || kind == node_kind::uniform || kind == node_kind::vtx_attrib;
    }
}

struct spirv_function
{
    spv::Id return_type;
    spv::Id function_id;
};

struct spirv_builder
{
    scope_stack stack_pool;
    scope_alloc_mgr_t<std::uint32_t> root_scope;

    using operand_vector = fvector<std::uint32_t, scope_alloc_mgr_t<std::uint32_t>>;
    using instr = fvector<std::uint32_t, scope_alloc_mgr_t<std::uint32_t>>;
    dvector<instr> capabilities;
    dvector<instr> imports;
    dvector<instr> decorations;
    dvector<instr> constants_types_globals;
    dvector<instr> externals;
    dvector<instr> module;

    using type_kind_cache = fvector<spv::Id, scope_alloc_mgr_t<spv::Id>>;

    static constexpr std::size_t num_type_kinds = std::size_t(ast::type_kind::none) + 1;
    using spirv_type_cache = std::array<type_kind_cache, num_type_kinds>;
    spirv_type_cache type_cache;


    struct cached_pointer
    {
        spv::Id type;
        spv::StorageClass storage_class;

        bool operator==(const cached_pointer& other) const
        {
            return type == other.type && storage_class == other.storage_class;
        }
    };

    struct cached_pointer_hash
    {
        std::uint64_t operator()(const cached_pointer& item) const
        {
            return item.type ^ item.storage_class;
        }
    };

    std::unordered_map<cached_pointer, spv::Id, cached_pointer_hash> pointer_cache;

    struct cached_constant
    {
        std::uint64_t val;
        spv::Id type;

        bool operator==(const cached_constant& other) const
        {
            return val == other.val && type == other.type;
        }
    };

    struct cached_constant_hash
    {
        std::uint64_t operator()(const cached_constant& item) const
        {
            return item.val ^ item.type;
        }
    };

    std::unordered_map<cached_constant, spv::Id, cached_constant_hash> constant_cache;

    struct cached_func_type
    {
        spv::Id return_type;
        std::span<spv::Id> params;

        bool operator==(const cached_func_type& other) const
        {
            const bool return_equal = return_type == other.return_type;
            const bool params_equal = std::equal(params.begin(), params.end(), other.params.begin());
            return return_equal && params_equal;
        }
    };

    struct cached_func_type_hash
    {
        std::uint64_t operator()(const cached_func_type& item) const
        {
            std::uint64_t params_hash = 0;
            for (const spv::Id type_id : item.params)
            {
                params_hash ^= type_id;
            }

            return item.return_type ^ params_hash;
        }
    };

    std::unordered_map<cached_func_type, spv::Id, cached_func_type_hash> func_type_cache;

    instr memory_model_instr;

    std::uint32_t spv_version;
    std::uint32_t builder_num;
    std::uint32_t unique_id;

    std::size_t dump_size;

    static constexpr std::uint32_t magic_number = 0x07230203;
    static constexpr std::size_t stack_pool_size = (1 << 20) * sizeof(std::uint32_t); // 4MB
    static constexpr std::size_t num_spirv_types = spv::OpConstantTrue - spv::OpTypeVoid;
    spirv_builder(const std::uint32_t spv_version_, const std::uint32_t builder_num_) :
        stack_pool(stack_pool_size),
        type_cache(),
        spv_version(spv_version_),
        builder_num(builder_num_),
        unique_id(0),
        dump_size(0)
    {
        root_scope = stack_pool.scope<std::uint32_t>();
        set_memory_model(spv::AddressingModelLogical, spv::MemoryModelGLSL450);
    }

    std::uint32_t new_id()
    {
        return ++unique_id;
    }

    void set_memory_model(const spv::AddressingModel addressing_model, const spv::MemoryModel memory_model)
    {
        const std::size_t mm_instr_size = instr_size_no_result_or_type(2);
        memory_model_instr = make_instr_no_result_or_type(mm_instr_size, spv::OpMemoryModel);
        memory_model_instr.push_back(addressing_model);
        memory_model_instr.push_back(memory_model);
    }

    void add_import(const std::string_view name)
    {
        const std::size_t instr_size = instr_size_no_type(0, name);
        instr import_instr = make_instr_no_type(instr_size, spv::OpExtInstImport, new_id(), {}, name);
        imports.push_back(std::move(import_instr));
    }

    void add_capability(const spv::Capability capability)
    {
        const std::array<std::uint32_t, 1> cap_u32 = { capability };
        const std::span<const std::uint32_t> cap_view = cap_u32;
        const std::size_t instr_size = instr_size_no_result_or_type(cap_view.size());
        instr cap_instr = make_instr_no_result_or_type(instr_size, spv::OpCapability, cap_view);
        capabilities.push_back(std::move(cap_instr));
    }

    void add_decoration(const spv::Id id, const spv::Decoration decoration,
        const std::string_view str, const std::uint32_t literal)
    {
        static constexpr std::size_t imm_id_operands = 3;
        static constexpr std::size_t ops = 1;
        const std::size_t instr_size = ops + imm_id_operands + str_operand_size(str);
        instr decoration_instr = make_instr_no_result_or_type(instr_size, spv::OpDecorate);
        decoration_instr.push_back(id);
        decoration_instr.push_back(decoration);
        push_str_operand(decoration_instr, str);
        decoration_instr.push_back(literal);

        decorations.push_back(std::move(decoration_instr));
    }

    type_kind_cache& get_type_kind_cache(const ast::type_kind type_kind, const std::size_t kind_cache_size)
    {
        const std::size_t type_kind_idx = std::size_t(type_kind);
        type_kind_cache& kind_cache = type_cache[type_kind_idx];
        if (kind_cache.size() != 0)
        {
            return kind_cache;
        }

        kind_cache.assign(type_kind_cache(kind_cache_size, root_scope));
        util::copy_n(kind_cache, 0, kind_cache_size);

        return kind_cache;
    }

    spv::Id find_type(const ast::type* type)
    {
        switch (type->kind)
        {
#define T(kind) \
        case ast::type_kind::##kind: \
            return find_##kind##_type(*type->as<ast::types::##kind>()); \
            break;

            AST_TYPE_KIND

#undef T
        }
    }

    spv::Id find_real_type(const ast::types::real& type)
    {
        type_kind_cache& kind_cache = get_type_kind_cache(type.kind, 3);

        spv::Id* found = nullptr;
        switch (type.width)
        {
        case 64:
            found = &kind_cache[2];
            add_capability(spv::CapabilityFloat64);
            break;
        case 32:
            found = &kind_cache[1];
            break;
        case 16:
            found = &kind_cache[0];
            break;
        default:
            assert_not_reached
        }

        assert(found);
        if (*found)
        {
            return *found;
        }

        *found = new_id();
        const std::span<const std::uint32_t> width_operand = std::span<const std::uint32_t>(&type.width, 1);
        const std::size_t instr_size = instr_size_no_type(width_operand.size());
        instr real_type_instr = make_instr_no_type(instr_size, spv::OpTypeFloat, *found, width_operand);

        constants_types_globals.push_back(std::move(real_type_instr));

        return *found;
    }

    spv::Id find_integer_type(const ast::types::integer& type)
    {
        type_kind_cache& kind_cache = get_type_kind_cache(type.kind, 4);

        spv::Id *found = nullptr;
        switch (type.width)
        {
        case 64:
            found = &kind_cache[3];
            add_capability(spv::CapabilityInt64);
            break;
        case 32:
            found = &kind_cache[2];
            break;
        case 16:
            found = &kind_cache[1];
            break;
        case 8:
            found = &kind_cache[0];
            break;
        default:
            assert_not_reached
        }

        assert(found);
        if (*found)
        {
            return *found;
        }

        *found = new_id();
        const std::size_t instr_size = instr_size_no_type(2);
        instr int_type_instr = make_instr_no_type(instr_size, spv::OpTypeInt, *found);
        int_type_instr.push_back(type.width);
        int_type_instr.push_back(type.has_sign ? 1 : 0);

        constants_types_globals.push_back(std::move(int_type_instr));

        return *found;
    }

    spv::Id find_vector_type(const ast::types::vector& type)
    {
        type_kind_cache& kind_cache = get_type_kind_cache(type.kind, 3);

        spv::Id* found = nullptr;
        switch (type.size)
        {
        case 2:
            found = &kind_cache[0];
            break;
        case 3:
            found = &kind_cache[1];
            break;
        case 4:
            found = &kind_cache[2];
            break;
        default:
            assert_not_reached
        }

        assert(found);
        if (*found)
        {
            return *found;
        }

        const spv::Id comp_type_id = find_type(type.comp_type);

        *found = new_id();
        const std::size_t instr_size = instr_size_no_type(2);
        instr int_type_instr = make_instr_no_type(instr_size, spv::OpTypeVector, *found);

        assert(type.comp_type->kind == ast::type_kind::integer || type.comp_type->kind == ast::type_kind::real);

        int_type_instr.push_back(comp_type_id);
        int_type_instr.push_back(type.size);

        constants_types_globals.push_back(std::move(int_type_instr));

        return *found;
    }

    spv::Id find_matrix_type(const ast::types::matrix& type)
    {
        static constexpr std::size_t max_dim = 4;
        type_kind_cache& kind_cache = get_type_kind_cache(type.kind, max_dim * max_dim);

        const std::size_t cache_idx = type.rows + type.cols * max_dim;

        assert(cache_idx < max_dim * max_dim);

        spv::Id* found = &kind_cache[cache_idx];
        if (*found)
        {
            return *found;
        }

        const spv::Id comp_type_id = find_type(type.comp_type);
        const spv::Id col_type_id = find_vector_type(ast::types::vector(type.comp_type, type.rows));

        *found = new_id();
        const std::size_t instr_size = instr_size_no_type(2);
        instr int_type_instr = make_instr_no_type(instr_size, spv::OpTypeMatrix, *found);

        assert(type.comp_type->kind == ast::type_kind::integer || type.comp_type->kind == ast::type_kind::real);

        int_type_instr.push_back(col_type_id);
        int_type_instr.push_back(type.cols);

        constants_types_globals.push_back(std::move(int_type_instr));

        return *found;
    }

    spv::Id find_none_type(const ast::types::none& type)
    {
        type_kind_cache& kind_cache = get_type_kind_cache(type.kind, 1);

        spv::Id* found = &kind_cache[0];
        if (*found)
        {
            return *found;
        }

        *found = new_id();
        const std::size_t instr_size = instr_size_no_type(0);
        instr int_type_instr = make_instr_no_type(instr_size, spv::OpTypeVoid, *found);

        constants_types_globals.push_back(std::move(int_type_instr));

        return *found;
    }

    spv::Id find_pointer_type(const spv::Id type, const spv::StorageClass storage_class)
    {
        const cached_pointer tmp_cp = {
            .type = type,
            .storage_class = storage_class
        };

        auto found = pointer_cache.find(tmp_cp);
        if (found != std::end(pointer_cache))
        {
            return found->second;
        }
        
        const spv::Id result_id = new_id();
        const std::size_t instr_sz = instr_size_no_type(2); // storage_class, type
        instr ptr_type_instr = make_instr_no_type(instr_sz, spv::OpTypePointer, result_id);
        ptr_type_instr.push_back(storage_class);
        ptr_type_instr.push_back(type);

        constants_types_globals.push_back(std::move(ptr_type_instr));
        pointer_cache.insert(std::make_pair(tmp_cp, result_id));

        return result_id;
    }

    spv::Id find_function_type(const spv::Id return_type, const std::span<spv::Id> param_types)
    {
        const cached_func_type tmp_ft = {
            .return_type = return_type,
            .params = param_types
        };

        const auto found = func_type_cache.find(tmp_ft);
        if (found != std::end(func_type_cache))
        {
            return found->second;
        }

        const spv::Id result_id = new_id();
        const std::size_t instr_size = instr_size_no_type(1 + param_types.size());
        instr function_type_instr = make_instr_no_type(instr_size, spv::OpTypeFunction, result_id);
        function_type_instr.push_back(return_type);
        util::push(function_type_instr, param_types);
        constants_types_globals.push_back(std::move(function_type_instr));

        const cached_func_type new_ft = {
            .return_type = return_type,
            .params = std::span<spv::Id>(std::prev(std::end(function_type_instr), param_types.size()), param_types.size())
        };
        func_type_cache.insert(std::make_pair(new_ft, result_id));

        return result_id;
    }

    spv::Id find_cached_constant(const cached_constant& cc, const std::size_t num_val_words)
    {
        auto found = constant_cache.find(cc);
        if (found != std::end(constant_cache))
        {
            return found->second;
        }

        union
        {
            std::uint32_t u32s[2];
            std::uint64_t u64;
        } u;

        u.u64 = cc.val;

        spv::Id constant_id = new_id();
        const std::size_t instr_size = instr_size_no_type(num_val_words);
        instr constant_instr = make_instr_no_type(instr_size, spv::OpConstant, constant_id, std::span<std::uint32_t>{u.u32s, num_val_words});

        constants_types_globals.push_back(std::move(constant_instr));
        constant_cache.insert(std::make_pair(cc, constant_id));

        return constant_id;
    }

    spv::Id make_double_constant(const double x)
    {
        union
        {
            double d;
            std::uint64_t u64;
        } u;

        u.d = x;

        ast::types::real double_type = ast::types::real(64);
        const spv::Id float_type_id = find_real_type(double_type);
        
        cached_constant tmp_cc = {
            .val = u.u64,
            .type = float_type_id
        };

        return find_cached_constant(tmp_cc, 2);
    }

    spv::Id make_float_constant(const float x)
    {
        union
        {
            float f;
            std::uint64_t u64;
        } u;

        u.f = x;

        ast::types::real float_type = ast::types::real(32);
        const spv::Id float_type_id = find_real_type(float_type);

        cached_constant tmp_cc = {
            .val = u.u64,
            .type = float_type_id
        };

        return find_cached_constant(tmp_cc, 1);
    }

    spv::Id make_float16_constant(const float x)
    {
        std::uint64_t val16 = util::float_to_half_fast(x).u;

        ast::types::real float_type = ast::types::real(16);
        const spv::Id float_type_id = find_real_type(float_type);

        cached_constant tmp_cc = {
            .val = val16,
            .type = float_type_id
        };

        return find_cached_constant(tmp_cc, 1);
    }

    template<typename T>
    spv::Id make_integer_constant(const T x)
    {
        const ast::types::integer int_type = ast::types::integer(util::bitsof<T>(), std::is_signed_v<T>);
        const spv::Id int_type_id = find_type(&int_type);

        std::uint64_t x_dword = 0;
        std::memcpy(&x_dword, &x, sizeof(T));

        cached_constant tmp_cc = {
            .val = x_dword,
            .type = int_type_id
        };

        return find_cached_constant(tmp_cc, util::div_round_up(sizeof(T), 4ui64));
    }

    spv::Id make_function_call(const spv::Id return_type, const spv::Id fn_id, const std::span<spv::Id>& args)
    {
        const spv::Id result_id = new_id();
        const std::size_t instr_sz = instr_size(args.size() + 1); // + 1 for fn id
        instr call_instr = make_instr(instr_sz, spv::OpFunctionCall, result_id, return_type);
        call_instr.push_back(fn_id);
        util::push(call_instr, args);

        module.push_back(std::move(call_instr));

        return result_id;
    }

    spv::Id make_variable(const spv::Decoration precision, const spv::StorageClass storage_class, const spv::Id type, 
        const char* name, const spv::Id initializer = spv::NoResult)
    {
        const spv::Id pointer_type = find_pointer_type(type, storage_class);
        const spv::Id result_id = new_id();
        const std::size_t instr_sz = instr_size(initializer == spv::NoResult ? 1 : 2);
        instr var_instr = make_instr(instr_sz, spv::OpVariable, result_id, pointer_type);
        var_instr.push_back(storage_class);
        
        if (initializer != spv::NoResult)
        {
            var_instr.push_back(initializer);
        }

        if (storage_class == spv::StorageClassFunction)
        {
            module.push_back(std::move(var_instr));
        }
        else
        {
            constants_types_globals.push_back(std::move(var_instr));
        }

        // TODO: add name, set precision

        return result_id;
    }

    spv::Id make_label()
    {
        const std::size_t instr_sz = instr_size_no_type(0);
        const spv::Id result_id = new_id();
        instr label_instr = make_instr_no_type(instr_sz, spv::OpLabel, result_id);
        module.push_back(std::move(label_instr));
        return result_id;
    }

    void make_return(const spv::Id ret_val = spv::NoResult)
    {
        if (ret_val != spv::NoResult) 
        {
            std::span<const spv::Id> value_view = std::span<const spv::Id>(&ret_val, 1);
            const std::size_t instr_sz = instr_size_no_result_or_type(value_view.size());
            instr ret_instr = make_instr_no_result_or_type(instr_sz, spv::OpReturnValue, value_view);
            module.push_back(std::move(ret_instr));
        }
        else
        {
            const std::size_t instr_sz = instr_size_no_result_or_type();
            instr ret_instr = make_instr_no_result_or_type(instr_sz, spv::OpReturn);
            module.push_back(std::move(ret_instr));
        }
    }

    instr make_function_end_instr()
    {
        const std::size_t end_instr_sz = instr_size_no_result_or_type();
        instr end_instr = make_instr_no_result_or_type(end_instr_sz, spv::OpFunctionEnd);
        return end_instr;
    }

    void make_function_end()
    {
        instr end_instr = make_function_end_instr();
        module.push_back(std::move(end_instr));
    }

    spv::Id create_external_function(const spv::Id return_type, const std::span<spv::Id>& param_types)
    {
        const spv::Id function_type = find_function_type(return_type, param_types);

        const spv::Id result_id = new_id();
        const std::size_t instr_sz = instr_size(2 + param_types.size());
        instr func_instr = make_instr(instr_sz, spv::OpFunction, result_id, return_type);
        func_instr.push_back(spv::FunctionControlMaskNone);
        func_instr.push_back(function_type);

        externals.push_back(std::move(func_instr));

        for (const spv::Id spv_type : param_types)
        {
            const spv::Id param_id = new_id();
            const std::size_t param_instr_sz = instr_size();
            instr param_instr = make_instr(param_instr_sz, spv::OpFunctionParameter, param_id, spv_type);
            externals.push_back(std::move(param_instr));
        }

        instr end_instr = make_function_end_instr();
        externals.push_back(std::move(end_instr));

        return result_id;
    }

    spv::Id create_function_entry(const spv::Decoration precision, const spv::Id return_type, const char* name, 
        const std::span<spv::Id>& param_types, const std::span<std::span<spv::Decoration>>& decorations)
    {
        const spv::Id func_type = find_function_type(return_type, param_types);

        // TODO: handle return and param precision
        // TODO: name handling

        const spv::Id result_id = new_id();
        const std::size_t instr_sz = instr_size(2 + param_types.size());
        instr func_instr = make_instr(instr_sz, spv::OpFunction, result_id, return_type);
        func_instr.push_back(spv::FunctionControlMaskNone);
        func_instr.push_back(func_type);

        module.push_back(std::move(func_instr));

        for (const spv::Id spv_type : param_types)
        {
            const spv::Id param_id = new_id();
            const std::size_t param_instr_sz = instr_size();
            instr param_instr = make_instr(param_instr_sz, spv::OpFunctionParameter, param_id, spv_type);
            module.push_back(std::move(param_instr));
        }

        return result_id;
    }

    static std::size_t str_operand_size(const std::string_view name)
    {
        return name.empty() ? 0 : util::div_round_up(name.size() + 1, 4ui64);
    }

    static std::size_t instr_size(const std::size_t num_operands = 0)
    {
        static constexpr std::size_t min_size = 3;
        return min_size + num_operands;
    }

    static std::size_t instr_size_no_result_or_type(const std::size_t num_operands = 0, const std::string_view str_op = {})
    {
        static constexpr std::size_t min_size = 1;
        return min_size + str_operand_size(str_op) + num_operands;
    }

    static std::size_t instr_size_no_type(const std::size_t num_operands = 0, const std::string_view str_op = {})
    {
        static constexpr std::size_t min_size = 2;
        return min_size + str_operand_size(str_op) + num_operands;
    }

    instr make_instr(const std::size_t word_count, const spv::Op op, const spv::Id result_id, const spv::Id type_id, 
        const std::span<const std::uint32_t> operands = {})
    {
        dump_size += word_count;
        instr result = instr(word_count, root_scope);
        const std::uint32_t first_word = ((word_count) << spv::WordCountShift) | op;
        result.push_back(first_word);
        result.push_back(type_id);
        result.push_back(result_id);
        util::push(result, operands);
        return result;
    }

    instr make_instr_no_type(const std::size_t word_count, const spv::Op op, const spv::Id result_id,
        const std::span<const std::uint32_t> operands = {}, const std::string_view str_op = {})
    {
        dump_size += word_count;
        instr result = instr(word_count, root_scope);
        const std::uint32_t first_word = ((word_count) << spv::WordCountShift) | op;
        result.push_back(first_word);
        result.push_back(result_id);
        util::push(result, operands);
        push_str_operand(result, str_op);
        return result;
    }

    instr make_instr_no_result_or_type(const std::size_t word_count, spv::Op op, 
        const std::span<const std::uint32_t> operands = {}, const std::string_view str_op = {})
    {
        dump_size += word_count;
        instr result = instr(word_count, root_scope);
        const std::uint32_t first_word = ((word_count) << spv::WordCountShift) | op;
        result.push_back(first_word);
        util::push(result, operands);
        return result;
    }

    static void push_str_operand(instr& result, const std::string_view str)
    {
        std::size_t word_idx = 0;
        std::uint32_t cur_word = 0;
        const std::size_t str_words = str_operand_size(str);
        char* cur_word_ptr = (char*)std::next(result.begin(), result.size());
        result.set_size_unsafe(result.size() + str_operand_size(str));
        for (const char c : str)
        {
            cur_word_ptr[word_idx] = c;
            word_idx++;
        }

        for (; word_idx < str_words * 4; word_idx++)
        {
            cur_word_ptr[word_idx] = 0;
        }
    }

    fvector<std::uint32_t> dump() const
    {
        static constexpr std::size_t num_header_instrs = 5;
        const std::size_t num_instrs = num_header_instrs + capabilities.size() + 
            imports.size() + decorations.size() + externals.size() + module.size();

        fvector<std::uint32_t> result = fvector<std::uint32_t>(dump_size + num_header_instrs);
        result.push_back(magic_number);
        result.push_back(spv_version);
        result.push_back(builder_num);
        result.push_back(unique_id + 1);
        result.push_back(0);

        auto push_instr_vector = [&result](const dvector<instr>& instr_vec) {
            for (const instr& i : instr_vec)
            {
                util::push(result, i);
            }
        };

        push_instr_vector(capabilities);
        push_instr_vector(imports);
        util::push(result, memory_model_instr);
        push_instr_vector(decorations);
        push_instr_vector(constants_types_globals);
        push_instr_vector(externals);
        push_instr_vector(module);

        return result;
    }
};


struct spirv_codegen
{
    spirv_builder builder;

    std::unordered_map<ast::named_binding, spirv_function, 
        ast::named_binding_hash, ast::named_binding_equal> extern_cache;
    std::unordered_map<ast::named_binding, spirv_function, 
        ast::named_binding_hash, ast::named_binding_equal> function_cache;
    std::unordered_map<ast::binding, spv::Id, ast::binding_hash, ast::binding_equal> binding_cache;

    spirv_codegen(const bool linkage, spv::SpvBuildLogger* logger_ = nullptr) :
        builder(XS_SPIRV_VERSION_WORD(1, 5), 'P' << 24 | 'O' << 16 | 'O' << 8 | 'P')
    {
        //builder.setSource(spv::SourceLanguageUnknown, 0);
        builder.add_import("GLSL.std.450");
        builder.set_memory_model(spv::AddressingModelLogical, spv::MemoryModelGLSL450);

        //spv::Function* shaderEntry = builder.makeEntryPoint(glslangIntermediate->getEntryPointName().c_str());
        //entryPoint = builder.addEntryPoint(executionModel, shaderEntry, glslangIntermediate->getEntryPointName().c_str());

        builder.add_capability(spv::CapabilityShader);
        
        if (linkage)
        {
            builder.add_capability(spv::CapabilityLinkage);
        }
        //builder.addExecutionMode(shaderEntry, spv::ExecutionModeLocalSize, glslangIntermediate->getLocalSize(0),
        //    glslangIntermediate->getLocalSize(1),
        //    glslangIntermediate->getLocalSize(2));
    }

    spv::Id immediate_id(const ast::node* node)
    {
       assert(ast::is_imm(node->kind));

       switch (node->kind)
       {
#define T(type) \
        case ast::node_kind::##type: \
            return type##_id(node->as<ast::nodes::##type>()); \
            break;

           AST_IMM_NODE_KIND

#undef T

       default:
           assert(false && "bad type");
       }
    }

    spv::Id constant_id(const ast::nodes::constant* node)
    {
        switch (node->return_type->kind)
        {
        case ast::type_kind::real:
            return constant_real_id(node);
        case ast::type_kind::integer:
            return constant_integer_id(node);
        default:
            assert_not_reached
        }
    }

    spv::Id constant_real_id(const ast::nodes::constant* node)
    {
        static constexpr std::uint32_t double_width = 64;
        static constexpr std::uint32_t float_width = 32;
        static constexpr std::uint32_t half_width = 16;

        const ast::types::real* type = node->return_type->as<ast::types::real>();
        switch (type->width)
        {
        case double_width:
            return builder.make_double_constant(node->data_cast<double>());
        case float_width:
            return builder.make_float_constant(node->data_cast<float>());
        case half_width:
            return builder.make_float16_constant(node->data_cast<float>());
        default:
            assert_not_reached
        }
    }

    spv::Id constant_integer_id(const ast::nodes::constant* node)
    {
        static constexpr std::uint32_t int64_width = 64;
        static constexpr std::uint32_t int32_width = 32;
        static constexpr std::uint32_t int16_width = 16;
        static constexpr std::uint32_t int8_width = 8;

        const ast::types::integer* type = node->return_type->as<ast::types::integer>();
        if (type->has_sign)
        {
            switch (type->width)
            {
            case int64_width:
                return builder.make_integer_constant(node->data_cast<std::int64_t>());
            case int32_width:
                return builder.make_integer_constant(node->data_cast<std::int32_t>());
            case int16_width:
                return builder.make_integer_constant(node->data_cast<std::int16_t>());
            case int8_width:
                return builder.make_integer_constant(node->data_cast<std::int8_t>());
            default:
                assert_not_reached
            }
        }
        else
        {
            switch (type->width)
            {
            case int64_width:
                return builder.make_integer_constant(node->data_cast<std::uint64_t>());
            case int32_width:
                return builder.make_integer_constant(node->data_cast<std::uint32_t>());
            case int16_width:
                return builder.make_integer_constant(node->data_cast<std::uint16_t>());
            case int8_width:
                return builder.make_integer_constant(node->data_cast<std::uint8_t>());
            default:
                assert_not_reached
            }
        }
    }

    spv::Id uniform_id(const ast::nodes::uniform* node)
    {
        assert_not_implemented
        return 0;
    }

    spv::Id vtx_attrib_id(const ast::nodes::vtx_attrib* node)
    {
        assert_not_implemented
        return 0;
    }

    spv::Id variable_id(const ast::nodes::variable* node)
    {
        const auto found_binding = binding_cache.find(node->name);
        if (found_binding != std::end(binding_cache))
        {
            return found_binding->second;
        }

        assert_not_reached
    }

    spv::Id translate_node(const ast::node* node)
    {
        spv::Id res;
        switch (node->kind)
        {
#define T(type) \
        case ast::node_kind::##type: \
            res = translate_##type(static_cast<const ast::nodes::##type*>(node)); \
            break;

            AST_NON_IMM_NODE_KIND

#undef T
        default:
            assert(false && "bad type");
        }

        return res;
    }

    spv::Id translate_call(const ast::nodes::call* call)
    {
        fvector<spv::Id> arg_ids = fvector<spv::Id>(call->args.size());
        for (const std::unique_ptr<ast::node>& arg : call->args)
        {
            arg_ids.push_back(immediate_id(arg.get()));
        }

        spirv_function spv_func = find_function(call);

        return builder.make_function_call(spv_func.return_type, spv_func.function_id, arg_ids.span());
    }

    spv::Id translate_let(const ast::nodes::let* node)
    {
        const spv::Id before_eval = translate_node(node->before.get());

        binding_cache.insert(std::make_pair(node->name, before_eval));
        const spv::Id after_eval = translate_node(node->after.get());
        binding_cache.erase(node->name);
        return after_eval;
    }

    spv::Id translate_function(const ast::nodes::function* node)
    {
        spv::Block* entry_bb;
        //spv::Block* old_bb = builder.getBuildPoint();
        const spv::Id ret_type_id = type_id(node->return_type);
        const spirv_function& func = make_function(node, entry_bb);

        //builder.setBuildPoint(entry_bb);

        for (const ast::nodes::variable& var : node->params)
        {
            const spv::Id param_id = builder.make_variable(spv::NoPrecision, spv::StorageClassFunction, 
                type_id(var.return_type), var.name.c_str());
            binding_cache.insert(std::make_pair(var.name, param_id));
        }

        builder.make_label();

        translate_node(node->body.get());

        builder.make_return();
        builder.make_function_end();

        for (const ast::nodes::variable& var : node->params)
        {
            binding_cache.erase(var.name);
        }

        //builder.setBuildPoint(old_bb);

        return 0;
    }

    spv::Id type_id(const ast::type* type)
    {
        return builder.find_type(type);
    }

    spirv_function find_function(const ast::nodes::call* call)
    {        
        const auto cached_declared_func = function_cache.find(call->name);
        if (cached_declared_func != std::end(function_cache))
        {
            const spirv_function& found_func = cached_declared_func->second;
            return found_func;
        }

        const auto cached_extern_func = extern_cache.find(call->name);
        if (cached_extern_func != std::end(extern_cache))
        {
            return cached_extern_func->second;
        }

        std::vector<spv::Id> arg_types;
        arg_types.reserve(call->args.size());
        for (const std::unique_ptr<ast::node>& arg : call->args)
        {
            arg_types.push_back(type_id(arg->return_type));
        }

        const spv::Id ret_type_id = type_id(call->return_type);

        std::vector<std::vector<spv::Decoration>> arg_precisions;
        const spv::Id function_id = builder.create_external_function(ret_type_id, arg_types);
        
        builder.add_decoration(function_id, spv::DecorationLinkageAttributes, call->name.c_str(), spv::LinkageTypeImport);

        const spirv_function extern_fn = {
            .return_type = ret_type_id,
            .function_id = function_id
        };

        extern_cache[call->name] = extern_fn;

        return extern_fn;
    }

    spirv_function& make_function(const ast::nodes::function* func, spv::Block*& fn_bb)
    {
        assert(function_cache.find(func->name) == std::end(function_cache));

        std::vector<spv::Id> arg_types;
        arg_types.reserve(func->params.size());
        for (const ast::nodes::variable& param : func->params)
        {
            arg_types.push_back(type_id(param.return_type));
        }

        const spv::Id ret_type_id = type_id(func->return_type);

        std::span<std::span<spv::Decoration>> arg_precisions;
        const spv::Id func_id = builder.create_function_entry(spv::NoPrecision, ret_type_id,
            func->name.str.c_str(), arg_types, arg_precisions);
        
        function_cache[func->name] = spirv_function{
            .return_type = ret_type_id,
            .function_id = func_id
        };

        return function_cache.at(func->name);
    }
};


struct tools_impl
{
    tools_impl() : 
        ctx(SPV_ENV_UNIVERSAL_1_5), 
        core(SPV_ENV_UNIVERSAL_1_5), 
        opt(SPV_ENV_UNIVERSAL_1_5) 
    {}

    spvtools::Context ctx;
    spvtools::SpirvTools core;
    spvtools::Optimizer opt;

    dvector<fvector<std::uint32_t>> modules;
};

script_context::script_context(rhi::context& ctx) :
	logger_(),
	tools_(std::make_unique<tools_impl>())
{
	logger_ = [&ctx](std::string_view s) { ctx.log(s); };
    
	auto spv_tools_log = [this](spv_message_level_t, const char*, const spv_position_t&, const char* m) {
			logger_(m);
	};
    tools_->ctx.SetMessageConsumer(spv_tools_log);
	tools_->core.SetMessageConsumer(spv_tools_log);
	tools_->opt.SetMessageConsumer(spv_tools_log);
    tools_->opt.RegisterPerformancePasses();
    
}

script_context::~script_context() {}

std::uint64_t script_context::add(fvector<std::uint32_t> spirv)
{
    const std::uint64_t id = tools_->modules.size();
    tools_->modules.push_back(std::move(spirv));
    return id;
}

std::unique_ptr<ast::node> script_context::parse(const std::string_view filename)
{
    FILE* f = fopen(filename.data(), "r");
    if (f == NULL) {
        perror(filename.data());
        return nullptr;
    }
    yyin = f;
    yy_switch_to_buffer(yy_create_buffer(yyin, 4096));
    yyparse();
    fclose(f);
    return nullptr;
}

fvector<std::uint32_t> script_context::compile(std::unique_ptr<ast::node> root)
{
    fvector<std::uint32_t> test;

    spv::SpvBuildLogger logger;
    spirv_codegen codegen = spirv_codegen(true, &logger);
    codegen.translate_node(root.get());
    //codegen.builder.postProcess();

    fvector<std::uint32_t> result = codegen.builder.dump();

    std::string text;
    const bool ok = tools_->core.Validate(result.data(), result.size());
    //const bool ok = tools_->core.Disassemble(result.data(), result.size(), &text);
        //SPV_BINARY_TO_TEXT_OPTION_PRINT | SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES | SPV_BINARY_TO_TEXT_OPTION_COMMENT);
    assert(ok);

    //spv::Dissassemble()

    return result;
}

std::vector<std::uint32_t> script_context::link(const std::vector<std::uint64_t>& module_ids, const bool optimize)
{
    fvector<std::size_t> module_sizes(module_ids.size());
    fvector<const std::uint32_t*> module_ptrs(module_ids.size());
    for (const std::uint64_t id : module_ids)
    {
        module_sizes.push_back(tools_->modules[id].size());
        module_ptrs.push_back(tools_->modules[id].data());
    }

    std::vector<std::uint32_t> linked;
    spvtools::Link(tools_->ctx, module_ptrs.data(), module_sizes.data(), module_ids.size(), &linked);

    if (!optimize)
    {
        return linked;
    }

    std::vector<std::uint32_t> optimized;
    tools_->opt.Run(linked.data(), linked.size(), &optimized);

    return optimized;
}


}
