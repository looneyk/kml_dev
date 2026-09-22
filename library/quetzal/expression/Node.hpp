#if !defined(QUETZAL_EXPRESSION_NODE_HPP)
#define QUETZAL_EXPRESSION_NODE_HPP
//------------------------------------------------------------------------------
// Node.hpp
//
//------------------------------------------------------------------------------

#include <memory>
#include <string>
#include <vector>

namespace quetzal::expression
{

    enum class NodeKind
    {
        BinaryOp,
        UnaryOp,
        Literal,
        Identifier,
        FunctionCall
    };

    //--------------------------------------------------------------------------
    // A parse tree node, independent of any value type
    struct Node
    {
        using pointer_type = std::shared_ptr<Node>;

        NodeKind kind;
        std::string text; // operator symbol, function/identifier name, or literal text
        std::vector<pointer_type> children;
        bool parenthesized = false;
    };

} // namespace quetzal::expression

#endif // QUETZAL_EXPRESSION_NODE_HPP
