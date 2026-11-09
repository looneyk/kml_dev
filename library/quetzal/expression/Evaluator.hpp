#if !defined(QUETZAL_EXPRESSION_EVALUATOR_HPP)
#define QUETZAL_EXPRESSION_EVALUATOR_HPP
//------------------------------------------------------------------------------
// expression
// Evaluator.hpp
//------------------------------------------------------------------------------

#include "Node.hpp"
#include "SymbolTable.hpp"
#include "quetzal/common/Exception.hpp"
#include "quetzal/common/string_util.hpp"
#include <algorithm>
#include <cmath>
#include <concepts>
#include <cassert>
#include <format>
#include <sstream>
#include <string>

namespace quetzal::expression
{

    //--------------------------------------------------------------------------
    template<typename T>
    class Evaluator
    {
    public:

        static constexpr DataType data_type = std::same_as<T, std::string> ? DataType::String : DataType::Number;

        static T evaluate(const Node& n, const SymbolTable& symbol_table);

    private:

        static T lookup(const Node& n, const SymbolTable& symbol_table);
        static T apply_binary(const std::string& op, T a, T b);
        static T apply_unary(const std::string& op, T a);
        static T apply_function(const Node& n, const SymbolTable& symbol_table);
    };

} // namespace quetzal::expression

//------------------------------------------------------------------------------
template<typename T>
T quetzal::expression::Evaluator<T>::evaluate(const Node& n, const SymbolTable& symbol_table)
{
    switch (n.kind)
    {
        case NodeKind::Literal:
            assert(n.type == data_type);
            if constexpr (std::same_as<T, std::string>)
            {
                return n.text;
            }
            else
            {
                return to_type<T>(n.text);
            }

        case NodeKind::Identifier:
            return lookup(n, symbol_table);

        case NodeKind::BinaryOp:
            assert(n.children.size() == 2);
            return apply_binary(n.text, evaluate(*n.children[0], symbol_table), evaluate(*n.children[1], symbol_table));

        case NodeKind::UnaryOp:
            assert(n.children.size() == 1);
            return apply_unary(n.text, evaluate(*n.children[0], symbol_table));

        case NodeKind::FunctionCall:
            return apply_function(n, symbol_table);
    }

    throw Exception("Evaluator::evaluate - unrecognized node kind", __FILE__, __LINE__);
}

//------------------------------------------------------------------------------
template<typename T>
T quetzal::expression::Evaluator<T>::lookup(const Node& n, const SymbolTable& symbol_table)
{
    assert(n.type == data_type);

    auto i = symbol_table.find(n.text);
    if (i != symbol_table.end())
    {
        if (i->second.type != n.type)
        {
            throw Exception(("Evaluator::lookup - symbol type differs from its type when parsed " + n.text).c_str(), __FILE__, __LINE__);
        }

        return to_type<T>(i->second.value);
    }

    std::ostringstream oss;
    oss << "Evaluator::lookup - identifier not found " << n.text;
    throw Exception(oss.str().c_str(), __FILE__, __LINE__);
}

//------------------------------------------------------------------------------
template<typename T>
T quetzal::expression::Evaluator<T>::apply_binary(const std::string& op, T a, T b)
{
    if (op == "+")
    {
        if constexpr (requires (T x, T y) { x + y; })
        {
            return a + b;
        }
        else
        {
            throw Exception("Evaluator::apply_binary - operator + not supported for this type", __FILE__, __LINE__);
        }
    }
    else if (op == "-")
    {
        if constexpr (requires (T x, T y) { x - y; })
        {
            return a - b;
        }
        else
        {
            throw Exception("Evaluator::apply_binary - operator - not supported for this type", __FILE__, __LINE__);
        }
    }
    else if (op == "*")
    {
        if constexpr (requires (T x, T y) { x * y; })
        {
            return a * b;
        }
        else
        {
            throw Exception("Evaluator::apply_binary - operator * not supported for this type", __FILE__, __LINE__);
        }
    }
    else if (op == "/")
    {
        if constexpr (requires (T x, T y) { x / y; })
        {
            return a / b;
        }
        else
        {
            throw Exception("Evaluator::apply_binary - operator / not supported for this type", __FILE__, __LINE__);
        }
    }

    throw Exception("Evaluator::apply_binary - unrecognized operator " + op, __FILE__, __LINE__);
}

//------------------------------------------------------------------------------
template<typename T>
T quetzal::expression::Evaluator<T>::apply_unary(const std::string& op, T a)
{
    if (op == "-")
    {
        if constexpr (requires (T x) { -x; })
        {
            return -a;
        }
        else
        {
            throw Exception("Evaluator::apply_unary - unary - not supported for this type", __FILE__, __LINE__);
        }
    }

    throw Exception("Evaluator::apply_unary - unrecognized operator " + op, __FILE__, __LINE__);
}

//------------------------------------------------------------------------------
template<typename T>
T quetzal::expression::Evaluator<T>::apply_function(const Node& n, const SymbolTable& symbol_table)
{
    const std::string& name = n.text;

    if (name == "abs")
    {
        assert(n.children.size() == 1);
        if constexpr (requires (T a) { std::abs(a); })
        {
            return std::abs(evaluate(*n.children[0], symbol_table));
        }
        else
        {
            throw Exception("Evaluator::apply_function - abs not supported for this type", __FILE__, __LINE__);
        }
    }
    else if (name == "min")
    {
        assert(n.children.size() == 2);
        return std::min(evaluate(*n.children[0], symbol_table), evaluate(*n.children[1], symbol_table));
    }
    else if (name == "max")
    {
        assert(n.children.size() == 2);
        return std::max(evaluate(*n.children[0], symbol_table), evaluate(*n.children[1], symbol_table));
    }
    else if (name == "mod")
    {
        assert(n.children.size() == 2);
        T a = evaluate(*n.children[0], symbol_table);
        T b = evaluate(*n.children[1], symbol_table);

        if constexpr (requires (T x, T y) { x % y; })
        {
            return a % b;
        }
        else if constexpr (requires (T x, T y) { std::fmod(x, y); })
        {
            return std::fmod(a, b);
        }
        else
        {
            throw Exception("Evaluator::apply_function - mod not supported for this type", __FILE__, __LINE__);
        }
    }
    else if (name == "str")
    {
        assert(n.children.size() == 1);
        if constexpr (std::same_as<T, std::string>)
        {
            return std::format("{}", Evaluator<double>::evaluate(*n.children[0], symbol_table));
        }
        else
        {
            throw Exception("Evaluator::apply_function - str not supported for this type", __FILE__, __LINE__);
        }
    }

    std::ostringstream oss;
    oss << "Evaluator::apply_function - function not found " << name;
    throw Exception(oss.str().c_str(), __FILE__, __LINE__);
}

#endif // QUETZAL_EXPRESSION_EVALUATOR_HPP
