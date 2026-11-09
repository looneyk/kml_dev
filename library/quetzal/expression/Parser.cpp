//------------------------------------------------------------------------------
// expression
// Parser.cpp
//------------------------------------------------------------------------------

#include "Parser.hpp"
#include <charconv>
#include <format>
#include <sstream>
#include <cassert>

using namespace std;

namespace
{

    constexpr string_view null_keyword = "null";

} // namespace

//------------------------------------------------------------------------------
quetzal::expression::Parser::Parser() :
    m_functions()
{
    init_functions();
}

//------------------------------------------------------------------------------
quetzal::expression::Parser::Function::Function(const string& name, vector<DataType> argument_types, DataType return_type) :
    m_name(name),
    m_argument_types(std::move(argument_types)),
    m_return_type(return_type)
{
}

//------------------------------------------------------------------------------
void quetzal::expression::Parser::fail(ParseError::Code code, size_t position, const string& message)
{
    throw ParseError {code, position, message};
}

//------------------------------------------------------------------------------
quetzal::expression::Node::pointer_type quetzal::expression::Parser::make_node(NodeKind kind, DataType type, string text, vector<Node::pointer_type> children)
{
    auto n = make_shared<Node>();
    n->kind = kind;
    n->type = type;
    n->text = std::move(text);
    n->children = std::move(children);
    return n;
}

//------------------------------------------------------------------------------
quetzal::expression::Node::pointer_type quetzal::expression::Parser::make_arithmetic_node(const ParseState& state, NodeKind kind, string op,
    vector<Node::pointer_type> operands)
{
    for (const auto& operand : operands)
    {
        if (operand->type == DataType::Null)
        {
            fail(ParseError::Code::NullOperand, state.pos, format("{} given null at position {}", op, state.pos));
        }

        if (operand->type != DataType::Number)
        {
            fail(ParseError::Code::StringOperand, state.pos, format("{} given a string at position {}", op, state.pos));
        }
    }

    return make_node(kind, DataType::Number, std::move(op), std::move(operands));
}

//------------------------------------------------------------------------------
void quetzal::expression::Parser::init_functions()
{
    for (const auto& function : {
        Function("abs", {DataType::Number}, DataType::Number),
        Function("min", {DataType::Number, DataType::Number}, DataType::Number),
        Function("max", {DataType::Number, DataType::Number}, DataType::Number),
        Function("mod", {DataType::Number, DataType::Number}, DataType::Number),
        Function("str", {DataType::Number}, DataType::String)
    })
    {
        m_functions.emplace(function.m_name, function);
    }

    return;
}

//------------------------------------------------------------------------------
quetzal::expression::Parser::result_type quetzal::expression::Parser::parse(std::string_view str, const SymbolTable& symbol_table) const
{
    try
    {
        ParseState state {.text = str, .symbol_table = symbol_table};
        next(state);
        return Expression(expression(state));
    }
    catch (const ParseError& error)
    {
        return std::unexpected(error);
    }
}

//------------------------------------------------------------------------------
int quetzal::expression::Parser::precedence(TokenType op)
{
    switch (op)
    {
        case TokenType::FunctionCall:
        case TokenType::LeftParen:
        case TokenType::RightParen:
        case TokenType::Comma:
            return 0;

        case TokenType::OperatorAdd:
        case TokenType::OperatorSubtract:
            return 1;

        case TokenType::OperatorMultiply:
        case TokenType::OperatorDivide:
            return 2;

        default:
            assert(false);
    }

    return 0;
}

//------------------------------------------------------------------------------
quetzal::expression::Node::pointer_type quetzal::expression::Parser::expression(ParseState& state) const
{
    Node::pointer_type node = term(state);
    assert(node);

    while (state.token_type != TokenType::End && precedence(state.token_type) >= precedence(TokenType::OperatorAdd))
    {
        if (accept(state, TokenType::OperatorAdd))
        {
            Node::pointer_type rhs = term(state);
            if (node->type == DataType::Null || rhs->type == DataType::Null)
            {
                fail(ParseError::Code::NullOperand, state.pos, format("+ given null at position {}", state.pos));
            }

            if (rhs->type != node->type)
            {
                fail(ParseError::Code::MixedTypes, state.pos, format("+ joins a number and a string at position {}", state.pos));
            }

            node = make_node(NodeKind::BinaryOp, node->type, "+", { node, rhs });
        }
        else if (accept(state, TokenType::OperatorSubtract))
        {
            node = make_arithmetic_node(state, NodeKind::BinaryOp, "-", { node, term(state) });
        }
        else
        {
            ostringstream oss;
            oss << "invalid token " << state.token_type << " at position " << state.pos;
            fail(ParseError::Code::UnexpectedToken, state.pos, oss.str());
        }
    }

    return node;
}

//------------------------------------------------------------------------------
quetzal::expression::Node::pointer_type quetzal::expression::Parser::term(ParseState& state) const
{
    Node::pointer_type node = factor(state);
    assert(node);

    while (state.token_type != TokenType::End && precedence(state.token_type) >= precedence(TokenType::OperatorMultiply))
    {
        if (accept(state, TokenType::OperatorMultiply))
        {
            node = make_arithmetic_node(state, NodeKind::BinaryOp, "*", { node, factor(state) });
        }
        else if (accept(state, TokenType::OperatorDivide))
        {
            node = make_arithmetic_node(state, NodeKind::BinaryOp, "/", { node, factor(state) });
        }
        else
        {
            ostringstream oss;
            oss << "invalid token " << state.token_type << " at position " << state.pos;
            fail(ParseError::Code::UnexpectedToken, state.pos, oss.str());
        }
    }

    return node;
}

//------------------------------------------------------------------------------
quetzal::expression::Node::pointer_type quetzal::expression::Parser::factor(ParseState& state) const
{
    string previous_token = state.token;
    char previous_quote = state.quote;

    if (accept(state, TokenType::NumberLiteral))
    {
        return make_node(NodeKind::Literal, DataType::Number, previous_token, {});
    }
    else if (accept(state, TokenType::StringLiteral))
    {
        Node::pointer_type node = make_node(NodeKind::Literal, DataType::String, previous_token, {});
        node->quote = previous_quote;
        return node;
    }
    else if (accept(state, TokenType::Identifier))
    {
        if (previous_token == null_keyword)
        {
            return make_node(NodeKind::Literal, DataType::Null, previous_token, {});
        }

        auto symbol = state.symbol_table.find(previous_token);
        if (symbol == state.symbol_table.end())
        {
            fail(ParseError::Code::UnknownIdentifier, state.pos, format("identifier not found {} at position {}", previous_token, state.pos));
        }

        return make_node(NodeKind::Identifier, symbol->second.type, previous_token, {});
    }
    else if (accept(state, TokenType::FunctionCall))
    {
        return function_call(state, previous_token);
    }
    else if (accept(state, TokenType::LeftParen))
    {
        Node::pointer_type node = expression(state);
        assert(node);
        node->parenthesized = true;
        expect(state, TokenType::RightParen);
        return node;
    }
    else if (accept(state, TokenType::OperatorSubtract))
    {
        return make_arithmetic_node(state, NodeKind::UnaryOp, "-", { factor(state) });
    }

    ostringstream oss;
    oss << "invalid token " << state.token_type << " at position " << state.pos;
    fail(ParseError::Code::UnexpectedToken, state.pos, oss.str());
}

//------------------------------------------------------------------------------
quetzal::expression::Node::pointer_type quetzal::expression::Parser::function_call(ParseState& state, const string& name) const
{
    auto i = m_functions.find(name);
    assert(i != m_functions.end());

    expect(state, TokenType::LeftParen);

    vector<Node::pointer_type> arguments;
    bool first = true;

    for (; ; )
    {
        if (accept(state, TokenType::RightParen))
        {
            const Function& function = i->second;
            if (arguments.size() != function.m_argument_types.size())
            {
                fail(ParseError::Code::ArgumentCount, state.pos, format("{}, {} arguments found, {} arguments expected at position {}", name,
                    arguments.size(), function.m_argument_types.size(), state.pos));
            }

            for (size_t slot = 0; slot < arguments.size(); ++slot)
            {
                if (arguments[slot]->type == DataType::Null)
                {
                    fail(ParseError::Code::NullOperand, state.pos, format("{}, argument {} is null at position {}", name, slot + 1, state.pos));
                }

                if (arguments[slot]->type != function.m_argument_types[slot])
                {
                    fail(ParseError::Code::ArgumentType, state.pos, format("{}, argument {} has the wrong type at position {}", name, slot + 1,
                        state.pos));
                }
            }

            return make_node(NodeKind::FunctionCall, function.m_return_type, name, arguments);
        }
        else if (first)
        {
            Node::pointer_type argument = expression(state);
            assert(argument);

            arguments.push_back(argument);

            first = false;
        }
        else if (accept(state, TokenType::Comma))
        {
            Node::pointer_type argument = expression(state);
            assert(argument);

            arguments.push_back(argument);
        }
        else
        {
            break;
        }
    }

    ostringstream oss;
    oss << "invalid token " << state.token_type << " at position " << state.pos;
    fail(ParseError::Code::UnexpectedToken, state.pos, oss.str());
}

//------------------------------------------------------------------------------
void quetzal::expression::Parser::next(ParseState& state) const
{
    const string_view& text = state.text;
    size_t& pos = state.pos;

    if (pos == string_view::npos)
    {
        assert(state.token_type == TokenType::End);
        return;
    }

    pos = text.find_first_not_of(' ', pos); // Skip spaces

    if (pos == string_view::npos)
    {
        state.token_type = TokenType::End;
        state.token.erase();
        return;
    }

    if (isdigit(text[pos]))
    {
        state.token_type = TokenType::NumberLiteral;
        double value = 0.0;
        const char* end = from_chars(text.data() + pos, text.data() + text.size(), value).ptr;
        size_t length = end - (text.data() + pos);
        state.token = string(text.substr(pos, length));
        pos += length;
        return;
    }

    // A backslash escapes the character after it, so a string literal can contain a quote or a backslash
    if (text[pos] == '"' || text[pos] == '\'')
    {
        const size_t start = pos;
        state.quote = text[pos];
        state.token.erase();

        for (++pos; pos < text.size() && text[pos] != state.quote; ++pos)
        {
            if (text[pos] == '\\' && pos + 1 < text.size())
            {
                ++pos;
            }

            state.token += text[pos];
        }

        if (pos == text.size())
        {
            fail(ParseError::Code::UnterminatedString, start, format("unterminated string literal at position {}", start));
        }

        state.token_type = TokenType::StringLiteral;
        ++pos;
        return;
    }

    if (isalpha(text[pos]) || text[pos] == '_')
    {
        size_t length = 0;
        while (pos + length < text.size() && (isalnum(text[pos + length]) || text[pos + length] == '_' || text[pos + length] == '.'))
        {
            ++length;
        }
        state.token = string(text.substr(pos, length));
        pos += length;

        if (m_functions.find(state.token) != m_functions.end())
        {
            state.token_type = TokenType::FunctionCall;
            return;
        }

        state.token_type = TokenType::Identifier;
        return;
    }

    switch (text[pos])
    {
        case '+':
            state.token_type = TokenType::OperatorAdd;
            state.token = text[pos];
            ++pos;
            return;

        case '-':
            state.token_type = TokenType::OperatorSubtract;
            state.token = text[pos];
            ++pos;
            return;

        case '*':
            state.token_type = TokenType::OperatorMultiply;
            state.token = text[pos];
            ++pos;
            return;

        case '/':
            state.token_type = TokenType::OperatorDivide;
            state.token = text[pos];
            ++pos;
            return;

        case '(':
            state.token_type = TokenType::LeftParen;
            state.token = text[pos];
            ++pos;
            return;

        case ')':
            state.token_type = TokenType::RightParen;
            state.token = text[pos];
            ++pos;
            return;

        case ',':
            state.token_type = TokenType::Comma;
            state.token = text[pos];
            ++pos;
            return;
    }

    fail(ParseError::Code::InvalidCharacter, pos, format("invalid character {} at position {}", text[pos], pos));
}

//------------------------------------------------------------------------------
bool quetzal::expression::Parser::accept(ParseState& state, TokenType token_type) const
{
    if (state.token_type == token_type)
    {
        next(state);
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
bool quetzal::expression::Parser::expect(ParseState& state, TokenType token_type) const
{
    if (accept(state, token_type))
    {
        return true;
    }

    ostringstream oss;
    oss << "invalid token " << state.token_type << ", " << token_type << " expected at position " << state.pos;
    fail(ParseError::Code::UnexpectedToken, state.pos, oss.str());
}

//------------------------------------------------------------------------------
ostream& quetzal::expression::operator<<(ostream& os, const Parser::TokenType& token_type)
{
    switch (token_type)
    {
        case Parser::TokenType::Null:
            os << "null";
            break;

        case Parser::TokenType::End:
            os << "end";
            break;

        case Parser::TokenType::NumberLiteral:
            os << "number literal";
            break;

        case Parser::TokenType::StringLiteral:
            os << "string literal";
            break;

        case Parser::TokenType::Identifier:
            os << "identifier";
            break;

        case Parser::TokenType::FunctionCall:
            os << "call()";
            break;

        case Parser::TokenType::LeftParen:
            os << "(";
            break;

        case Parser::TokenType::RightParen:
            os << ")";
            break;

        case Parser::TokenType::Comma:
            os << ",";
            break;

        case Parser::TokenType::OperatorAdd:
            os << "+";
            break;

        case Parser::TokenType::OperatorSubtract:
            os << "-";
            break;

        case Parser::TokenType::OperatorMultiply:
            os << "*";
            break;

        case Parser::TokenType::OperatorDivide:
            os << "/";
            break;

        default:
            assert(false);
    }

    return os;
}
