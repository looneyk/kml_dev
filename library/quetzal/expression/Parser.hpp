#if !defined(QUETZAL_EXPRESSION_PARSER_HPP)
#define QUETZAL_EXPRESSION_PARSER_HPP
//------------------------------------------------------------------------------
// expression
// Parser.hpp
//------------------------------------------------------------------------------

#include "quetzal/expression/Node.hpp"
#include "quetzal/expression/Expression.hpp"
#include "quetzal/expression/ParseError.hpp"
#include <expected>
#include <string>
#include <string_view>
#include <map>
#include <vector>
#include <iosfwd>

namespace quetzal::expression
{

    //--------------------------------------------------------------------------
    class Parser
    {
    public:

        using result_type = std::expected<Expression, ParseError>;

        Parser();

        // Only the types of the values in the symbol_table are used here. The literal null parses only as the whole text,
        // to an expression whose type is DataType::Null
        result_type parse(std::string_view str, const SymbolTable& symbol_table) const;

        //----------------------------------------------------------------------
        enum class TokenType
        {
            Null,
            End,
            NumberLiteral,
            StringLiteral,
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

            Function(const std::string& name, std::vector<DataType> argument_types, DataType return_type);

            std::string m_name;
            std::vector<DataType> m_argument_types;
            DataType m_return_type;
        };

        //----------------------------------------------------------------------
        struct ParseState
        {
            std::string_view text;
            const SymbolTable& symbol_table;
            size_t pos = 0;
            TokenType token_type = TokenType::Null;
            std::string token;
            char quote = '"';
        };

        [[noreturn]] static void fail(ParseError::Code code, std::size_t position, const std::string& message);

        static Node::pointer_type make_node(NodeKind kind, DataType type, std::string text, std::vector<Node::pointer_type> children);
        static Node::pointer_type make_arithmetic_node(const ParseState& state, NodeKind kind, std::string op, std::vector<Node::pointer_type> operands);

        void init_functions();

        static int precedence(TokenType op);

        Node::pointer_type expression(ParseState& state) const;
        Node::pointer_type term(ParseState& state) const;
        Node::pointer_type factor(ParseState& state) const;
        Node::pointer_type function_call(ParseState& state, const std::string& name) const;

        void next(ParseState& state) const;
        bool accept(ParseState& state, TokenType token_type) const;
        bool expect(ParseState& state, TokenType token_type) const;

        using functions_type = std::map<std::string, Function>;
        functions_type m_functions;

        friend
        std::ostream& operator<<(std::ostream& os, const Parser::TokenType& token_type);
    };

    std::ostream& operator<<(std::ostream& os, const Parser::TokenType& token_type);

} // namespace quetzal::expression

#endif // QUETZAL_EXPRESSION_PARSER_HPP
