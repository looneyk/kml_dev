#if !defined(QUETZAL_EXPRESSION_EVALUATOR_HPP)
#define QUETZAL_EXPRESSION_EVALUATOR_HPP
//------------------------------------------------------------------------------
// Evaluator.hpp
//
//------------------------------------------------------------------------------

#include "Node.hpp"
#include "SymbolTable.hpp"
#include "quetzal/common/Exception.hpp"
#include "quetzal/common/string_util.hpp"
#include <algorithm>
#include <cmath>
#include <cassert>
#include <sstream>

namespace quetzal::expression
{

    //--------------------------------------------------------------------------
    // Evaluates a Node tree to a T, with the literal conversion, symbol lookup and operators particular to T
    template<typename T>
    class Evaluator
    {
    public:

        T evaluate(const Node& n, const SymbolTable& st) const
        {
            switch (n.kind)
            {
                case NodeKind::Literal:
                    return to_type<T>(n.text);

                case NodeKind::Identifier:
                    return lookup(n.text, st);

                case NodeKind::BinaryOp:
                    assert(n.children.size() == 2);
                    return apply_binary(n.text, evaluate(*n.children[0], st), evaluate(*n.children[1], st));

                case NodeKind::UnaryOp:
                    assert(n.children.size() == 1);
                    return apply_unary(n.text, evaluate(*n.children[0], st));

                case NodeKind::FunctionCall:
                    return apply_function(n, st);
            }

            throw Exception("Evaluator::evaluate - unrecognized node kind", __FILE__, __LINE__);
        }

    private:

        T lookup(const std::string& id, const SymbolTable& st) const
        {
            auto i = st.find(id);
            if (i != st.end())
            {
                return to_type<T>(i->second);
            }

            std::ostringstream oss;
            oss << "Evaluator::lookup - identifier not found " << id;
            throw Exception(oss.str().c_str(), __FILE__, __LINE__);
        }

        T apply_binary(const std::string& op, T a, T b) const
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

        T apply_unary(const std::string& op, T a) const
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

        T apply_function(const Node& n, const SymbolTable& st) const
        {
            const std::string& name = n.text;

            if (name == "abs")
            {
                assert(n.children.size() == 1);

                if constexpr (requires (T a) { std::abs(a); })
                {
                    return std::abs(evaluate(*n.children[0], st));
                }
                else
                {
                    throw Exception("Evaluator::apply_function - abs not supported for this type", __FILE__, __LINE__);
                }
            }
            else if (name == "min")
            {
                assert(n.children.size() == 2);
                return std::min(evaluate(*n.children[0], st), evaluate(*n.children[1], st));
            }
            else if (name == "max")
            {
                assert(n.children.size() == 2);
                return std::max(evaluate(*n.children[0], st), evaluate(*n.children[1], st));
            }
            else if (name == "mod")
            {
                assert(n.children.size() == 2);

                T a = evaluate(*n.children[0], st);
                T b = evaluate(*n.children[1], st);

                // integral types use %, floating-point types use fmod
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

            std::ostringstream oss;
            oss << "Evaluator::apply_function - function not found " << name;
            throw Exception(oss.str().c_str(), __FILE__, __LINE__);
        }
    };

} // namespace quetzal::expression

#endif // QUETZAL_EXPRESSION_EVALUATOR_HPP
