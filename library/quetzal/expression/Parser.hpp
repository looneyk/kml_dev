#if !defined(QUETZAL_EXPRESSION_PARSER_HPP)
#define QUETZAL_EXPRESSION_PARSER_HPP
//------------------------------------------------------------------------------
// expression
// Parser.hpp
//------------------------------------------------------------------------------

#include "quetzal/expression/Node.hpp"
#include "quetzal/expression/Expression.hpp"
#include <string>
#include <map>
#include <optional>
#include <vector>
#include <iosfwd>

namespace quetzal::expression
{

    //--------------------------------------------------------------------------
    // Builds a Node tree, independent of any value type
    class Parser
    {
    public:

        Parser();
        Parser(const Parser&) = delete;
        ~Parser();

        Parser& operator=(const Parser&) = delete;

        // Returns std::nullopt on a syntax error
        std::optional<Expression> parse(const std::string& str);

        //----------------------------------------------------------------------
        enum class TokenType
        {
            Null,
            End,
            Literal,
            Identifier,
            FunctionCall,
            LeftParen,
            RightParen,
            Comma,
            OperatorAdd,
            OperatorSubtract,
            OperatorMultiply,
            OperatorDivide
        };

    private:

        //----------------------------------------------------------------------
        class Function
        {
        public:

            Function();
            Function(const std::string& name, size_t argument_count);

            std::string m_name;
            size_t m_argument_count;
            // signature (argument and return data types)
        };

        static Node::pointer_type make_node(NodeKind kind, std::string text, std::vector<Node::pointer_type> children);

        void init_functions();

        int precedence(TokenType op);

        Node::pointer_type expression();
        Node::pointer_type term();
        Node::pointer_type factor();
        Node::pointer_type function_call(const std::string& name);

        void next();
        bool accept(TokenType token_type);
        bool expect(TokenType token_type);

        using functions_type = std::map<std::string, Function>;
        functions_type m_functions;

        std::string m_expression;
        size_t m_pos;

        TokenType m_token_type;
        std::string m_token;

        friend
        std::ostream& operator<<(std::ostream& os, const Parser::TokenType& token_type);
    };

    std::ostream& operator<<(std::ostream& os, const Parser::TokenType& token_type);

} // namespace quetzal::expression

#endif // QUETZAL_EXPRESSION_PARSER_HPP
