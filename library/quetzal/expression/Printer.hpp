#if !defined(QUETZAL_EXPRESSION_PRINTER_HPP)
#define QUETZAL_EXPRESSION_PRINTER_HPP
//------------------------------------------------------------------------------
// Printer.hpp
//
//------------------------------------------------------------------------------

#include "Node.hpp"
#include <ostream>

namespace quetzal::expression
{

    //--------------------------------------------------------------------------
    // Prints a Node tree from each Node's own text
    class Printer
    {
    public:

        void print(std::ostream& os, const Node& n) const
        {
            if (n.parenthesized)
            {
                os << "(";
            }

            switch (n.kind)
            {
                case NodeKind::Literal:
                case NodeKind::Identifier:
                    os << n.text;
                    break;

                case NodeKind::BinaryOp:
                    print(os, *n.children[0]);
                    os << " " << n.text << " ";
                    print(os, *n.children[1]);
                    break;

                case NodeKind::UnaryOp:
                    os << n.text;
                    print(os, *n.children[0]);
                    break;

                case NodeKind::FunctionCall:
                    os << n.text << "(";
                    for (size_t i = 0; i < n.children.size(); ++i)
                    {
                        if (i > 0)
                        {
                            os << ", ";
                        }

                        print(os, *n.children[i]);
                    }
                    os << ")";
                    break;
            }

            if (n.parenthesized)
            {
                os << ")";
            }
        }
    };

} // namespace quetzal::expression

#endif // QUETZAL_EXPRESSION_PRINTER_HPP
