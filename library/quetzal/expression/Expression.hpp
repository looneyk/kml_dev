#if !defined(QUETZAL_EXPRESSION_EXPRESSION_HPP)
#define QUETZAL_EXPRESSION_EXPRESSION_HPP
//------------------------------------------------------------------------------
// expression
// Expression.hpp
//------------------------------------------------------------------------------

#include "Node.hpp"
#include "Evaluator.hpp"
#include "SymbolTable.hpp"
#include "quetzal/common/Exception.hpp"
#include <format>
#include <ostream>
#include <utility>

namespace quetzal::expression
{

    //--------------------------------------------------------------------------
    class Expression
    {
    public:

        explicit Expression(Node::pointer_type node);

        const Node& node() const;

        template<typename T>
        T evaluate(const SymbolTable& symbol_table) const;

    private:

        Node::pointer_type m_node;
    };

    std::ostream& operator<<(std::ostream& os, const Expression& expression);

} // namespace quetzal::expression

//------------------------------------------------------------------------------
template<>
struct std::formatter<quetzal::expression::Expression> : std::formatter<quetzal::expression::Node>
{
    std::format_context::iterator format(const quetzal::expression::Expression& expression, std::format_context& ctx) const;
};

//------------------------------------------------------------------------------
inline quetzal::expression::Expression::Expression(Node::pointer_type node) :
    m_node(std::move(node))
{
}

//------------------------------------------------------------------------------
inline const quetzal::expression::Node& quetzal::expression::Expression::node() const
{
    return *m_node;
}

//------------------------------------------------------------------------------
template<typename T>
T quetzal::expression::Expression::evaluate(const SymbolTable& symbol_table) const
{
    if (m_node->type != Evaluator<T>::data_type)
    {
        throw Exception("Expression::evaluate - the expression's type differs from the requested type", __FILE__, __LINE__);
    }

    return Evaluator<T>().evaluate(*m_node, symbol_table);
}

//------------------------------------------------------------------------------
inline std::ostream& quetzal::expression::operator<<(std::ostream& os, const Expression& expression)
{
    os << expression.node();
    return os;
}

//------------------------------------------------------------------------------
inline std::format_context::iterator std::formatter<quetzal::expression::Expression>::format(const quetzal::expression::Expression& expression,
    std::format_context& ctx) const
{
    return std::formatter<quetzal::expression::Node>::format(expression.node(), ctx);
}

#endif // QUETZAL_EXPRESSION_EXPRESSION_HPP
