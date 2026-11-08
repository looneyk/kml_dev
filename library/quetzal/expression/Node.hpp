#if !defined(QUETZAL_EXPRESSION_NODE_HPP)
#define QUETZAL_EXPRESSION_NODE_HPP
//------------------------------------------------------------------------------
// expression
// Node.hpp
//------------------------------------------------------------------------------

#include "DataType.hpp"
#include <format>
#include <memory>
#include <ostream>
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
    struct Node
    {
        using pointer_type = std::shared_ptr<Node>;

        NodeKind kind;
        DataType type;
        std::string text; // operator symbol, function/identifier name, or literal text
        std::vector<pointer_type> children;
        bool parenthesized = false;
        char quote = '"';
    };

    std::ostream& operator<<(std::ostream& os, const Node& node);

} // namespace quetzal::expression

//------------------------------------------------------------------------------
template<>
struct std::formatter<quetzal::expression::Node>
{
    constexpr std::format_parse_context::iterator parse(std::format_parse_context& ctx);
    std::format_context::iterator format(const quetzal::expression::Node& node, std::format_context& ctx) const;
};

//------------------------------------------------------------------------------
constexpr std::format_parse_context::iterator std::formatter<quetzal::expression::Node>::parse(std::format_parse_context& ctx)
{
    return ctx.begin();
}

//------------------------------------------------------------------------------
inline std::format_context::iterator std::formatter<quetzal::expression::Node>::format(const quetzal::expression::Node& node,
    std::format_context& ctx) const
{
    using quetzal::expression::NodeKind;
    using quetzal::expression::DataType;

    auto out = ctx.out();

    if (node.parenthesized)
    {
        *out++ = '(';
    }

    switch (node.kind)
    {
        case NodeKind::Literal:
            if (node.type == DataType::String)
            {
                *out++ = node.quote;
                for (char c : node.text)
                {
                    if (c == node.quote || c == '\\')
                    {
                        *out++ = '\\';
                    }

                    *out++ = c;
                }
                *out++ = node.quote;
                break;
            }

            [[fallthrough]];

        case NodeKind::Identifier:
            out = std::format_to(out, "{}", node.text);
            break;

        case NodeKind::BinaryOp:
            out = std::format_to(out, "{} {} {}", *node.children[0], node.text, *node.children[1]);
            break;

        case NodeKind::UnaryOp:
            out = std::format_to(out, "{}{}", node.text, *node.children[0]);
            break;

        case NodeKind::FunctionCall:
            out = std::format_to(out, "{}(", node.text);
            for (size_t i = 0; i < node.children.size(); ++i)
            {
                if (i > 0)
                {
                    out = std::format_to(out, ", ");
                }

                out = std::format_to(out, "{}", *node.children[i]);
            }
            *out++ = ')';
            break;
    }

    if (node.parenthesized)
    {
        *out++ = ')';
    }

    return out;
}

//------------------------------------------------------------------------------
inline std::ostream& quetzal::expression::operator<<(std::ostream& os, const Node& node)
{
    os << std::format("{}", node);
    return os;
}

#endif // QUETZAL_EXPRESSION_NODE_HPP
