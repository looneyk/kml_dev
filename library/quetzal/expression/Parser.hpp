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
    class Parser
    {
    public:

        Parser();
        Parser(const Parser&) = delete;
        Parser(Parser&&) = delete;
        ~Parser();

        Parser& operator=(const Parser&) = delete;
        Parser& operator=(Parser&&) = delete;

        // Only the types of the values in the symbol_table are used here
        // Returns std::nullopt on a syntax error, or for an identifier that is not one of the symbols
        std::optional<Expression> parse(const std::string& str, const SymbolTable& symbol_table);

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

        static Node::pointer_type make_node(NodeKind kind, DataType type, std::string text, std::vector<Node::pointer_type> children);
        Node::pointer_type make_arithmetic_node(NodeKind kind, std::string op, std::vector<Node::pointer_type> operands) const;

        void init_functions();

        int precedence(TokenType op);

        Node::pointer_type expression(const SymbolTable& symbol_table);
        Node::pointer_type term(const SymbolTable& symbol_table);
        Node::pointer_type factor(const SymbolTable& symbol_table);
        Node::pointer_type function_call(const std::string& name, const SymbolTable& symbol_table);

        void next();
        bool accept(TokenType token_type);
        bool expect(TokenType token_type);

        using functions_type = std::map<std::string, Function>;
        functions_type m_functions;

        std::string m_expression;
        size_t m_pos;

        TokenType m_token_type;
        std::string m_token;
        char m_quote;

        friend
        std::ostream& operator<<(std::ostream& os, const Parser::TokenType& token_type);
    };

    std::ostream& operator<<(std::ostream& os, const Parser::TokenType& token_type);

} // namespace quetzal::expression

#endif // QUETZAL_EXPRESSION_PARSER_HPP
