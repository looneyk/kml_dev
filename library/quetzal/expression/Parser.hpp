#if !defined(QUETZAL_EXPRESSION_PARSER_HPP)
#define QUETZAL_EXPRESSION_PARSER_HPP
//------------------------------------------------------------------------------
// Parser.hpp
//
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
    // Builds a Node tree only; carries no notion of a value type, so parse()
    // need not be a template and the resulting tree is reusable across types
    class Parser
    {
    public:

        Parser();
        Parser(const Parser&) = delete;
        ~Parser();

        Parser& operator=(const Parser&) = delete;

        // Returns std::nullopt on a syntax error rather than any kind of placeholder
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

            Function() :
                m_name(),
                m_nArguments(0)
            {
            }

            Function(const std::string& name, int nArguments) :
                m_name(name),
                m_nArguments(nArguments)
            {
            }

            std::string m_name;
            unsigned int m_nArguments;
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
        bool accept(TokenType tokenType);
        bool expect(TokenType tokenType);

        typedef std::map<std::string, Function> Functions;
        Functions m_functions;

        std::string m_expression;
        size_t m_pos;

        TokenType m_tokenType;
        std::string m_token;

        friend
        std::ostream& operator<<(std::ostream& os, const Parser::TokenType& tokenType);
    };

    std::ostream& operator<<(std::ostream& os, const Parser::TokenType& tokenType);

} // namespace quetzal::expression

#endif // QUETZAL_EXPRESSION_PARSER_HPP
