#if !defined(QUETZAL_EXPRESSION_EXPRESSION_HPP)
#define QUETZAL_EXPRESSION_EXPRESSION_HPP
//------------------------------------------------------------------------------
// Expression.hpp
//
//------------------------------------------------------------------------------

#include "Node.hpp"
#include "Evaluator.hpp"
#include "Printer.hpp"
#include "SymbolTable.hpp"
#include <ostream>
#include <utility>

namespace quetzal::expression
{

    //--------------------------------------------------------------------------
    // A copyable parsed Node tree; evaluate<T> and operator<< forward to Evaluator<T> and Printer
    class Expression
    {
    public:

        explicit Expression(Node::pointer_type node) :
            m_node(std::move(node))
        {
        }

        const Node& node() const
        {
            return *m_node;
        }

        template<typename T>
        T evaluate(const SymbolTable& st) const
        {
            return Evaluator<T>().evaluate(*m_node, st);
        }

        friend std::ostream& operator<<(std::ostream& os, const Expression& e)
        {
            Printer().print(os, *e.m_node);
            return os;
        }

    private:

        Node::pointer_type m_node;
    };

} // namespace quetzal::expression

#endif // QUETZAL_EXPRESSION_EXPRESSION_HPP
