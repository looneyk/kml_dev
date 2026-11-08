//------------------------------------------------------------------------------
// expression
// Parser.cpp
//------------------------------------------------------------------------------

#include "Parser.hpp"
#include "quetzal/common/Exception.hpp"
#include <sstream>
#include <cassert>

using namespace std;

//------------------------------------------------------------------------------
quetzal::expression::Parser::Parser() :
    m_functions(),
    m_expression(),
    m_pos(0),
    m_token_type(TokenType::Null),
    m_token(),
    m_quote('"')
{
    init_functions();
}

//------------------------------------------------------------------------------
quetzal::expression::Parser::~Parser()
{
}

//------------------------------------------------------------------------------
quetzal::expression::Parser::Function::Function(const string& name, vector<DataType> argument_types, DataType return_type) :
    m_name(name),
    m_argument_types(std::move(argument_types)),
    m_return_type(return_type)
{
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
quetzal::expression::Node::pointer_type quetzal::expression::Parser::make_arithmetic_node(NodeKind kind, string op, vector<Node::pointer_type> operands) const
{
    for (const auto& operand : operands)
    {
        if (operand->type != DataType::Number)
        {
            ostringstream oss;
            oss << "Parser::make_arithmetic_node - " << op << " given a string at position " << m_pos;
            throw Exception(oss.str().c_str(), __FILE__, __LINE__);
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
std::optional<quetzal::expression::Expression> quetzal::expression::Parser::parse(const string& str, const SymbolTable& symbol_table)
{
    try
    {
        m_expression = str;
        m_pos = 0;

        next();
        return Expression(expression(symbol_table));
    }
    catch (...)
    {
    }

    return std::nullopt;
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
quetzal::expression::Node::pointer_type quetzal::expression::Parser::expression(const SymbolTable& symbol_table)
{
    Node::pointer_type node = term(symbol_table);
    assert(node);

    while (m_token_type != TokenType::End && precedence(m_token_type) >= precedence(TokenType::OperatorAdd))
    {
        if (accept(TokenType::OperatorAdd))
        {
            Node::pointer_type rhs = term(symbol_table);
            if (rhs->type != node->type)
            {
                ostringstream oss;
                oss << "Parser::expression - + joins a number and a string at position " << m_pos;
                throw Exception(oss.str().c_str(), __FILE__, __LINE__);
            }

            node = make_node(NodeKind::BinaryOp, node->type, "+", { node, rhs });
        }
        else if (accept(TokenType::OperatorSubtract))
        {
            node = make_arithmetic_node(NodeKind::BinaryOp, "-", { node, term(symbol_table) });
        }
        else
        {
            ostringstream oss;
            oss << "Parser::expression - invalid token " << m_token_type << " at position " << m_pos;
            throw Exception(oss.str().c_str(), __FILE__, __LINE__);
        }
    }

    return node;
}

//------------------------------------------------------------------------------
quetzal::expression::Node::pointer_type quetzal::expression::Parser::term(const SymbolTable& symbol_table)
{
    Node::pointer_type node = factor(symbol_table);
    assert(node);

    while (m_token_type != TokenType::End && precedence(m_token_type) >= precedence(TokenType::OperatorMultiply))
    {
        if (accept(TokenType::OperatorMultiply))
        {
            node = make_arithmetic_node(NodeKind::BinaryOp, "*", { node, factor(symbol_table) });
        }
        else if (accept(TokenType::OperatorDivide))
        {
            node = make_arithmetic_node(NodeKind::BinaryOp, "/", { node, factor(symbol_table) });
        }
        else
        {
            ostringstream oss;
            oss << "Parser::term - invalid token " << m_token_type << " at position " << m_pos;
            throw Exception(oss.str().c_str(), __FILE__, __LINE__);
        }
    }

    return node;
}

//------------------------------------------------------------------------------
quetzal::expression::Node::pointer_type quetzal::expression::Parser::factor(const SymbolTable& symbol_table)
{
    string previous_token = m_token;
    char previous_quote = m_quote;

    if (accept(TokenType::NumberLiteral))
    {
        return make_node(NodeKind::Literal, DataType::Number, previous_token, {});
    }
    else if (accept(TokenType::StringLiteral))
    {
        Node::pointer_type node = make_node(NodeKind::Literal, DataType::String, previous_token, {});
        node->quote = previous_quote;
        return node;
    }
    else if (accept(TokenType::Identifier))
    {
        auto symbol = symbol_table.find(previous_token);
        if (symbol == symbol_table.end())
        {
            ostringstream oss;
            oss << "Parser::factor - identifier not found " << previous_token << " at position " << m_pos;
            throw Exception(oss.str().c_str(), __FILE__, __LINE__);
        }

        return make_node(NodeKind::Identifier, symbol->second.type, previous_token, {});
    }
    else if (accept(TokenType::FunctionCall))
    {
        return function_call(previous_token, symbol_table);
    }
    else if (accept(TokenType::LeftParen))
    {
        Node::pointer_type node = expression(symbol_table);
        assert(node);
        node->parenthesized = true;
        expect(TokenType::RightParen);
        return node;
    }
    else if (accept(TokenType::OperatorSubtract))
    {
        return make_arithmetic_node(NodeKind::UnaryOp, "-", { factor(symbol_table) });
    }

    ostringstream oss;
    oss << "Parser::factor - invalid token " << m_token_type << " at position " << m_pos;
    throw Exception(oss.str().c_str(), __FILE__, __LINE__);
}

//------------------------------------------------------------------------------
quetzal::expression::Node::pointer_type quetzal::expression::Parser::function_call(const string& name, const SymbolTable& symbol_table)
{
    auto i = m_functions.find(name);
    assert(i != m_functions.end());

    expect(TokenType::LeftParen);

    vector<Node::pointer_type> arguments;
    bool first = true;

    for (; ; )
    {
        if (accept(TokenType::RightParen))
        {
            const Function& function = i->second;
            if (arguments.size() != function.m_argument_types.size())
            {
                ostringstream oss;
                oss << "Parser::function_call - " << name << ", "<< arguments.size() << " arguments found, " << function.m_argument_types.size()
                    << " arguments expected at position " << m_pos;
                throw Exception(oss.str().c_str(), __FILE__, __LINE__);
            }

            for (size_t slot = 0; slot < arguments.size(); ++slot)
            {
                if (arguments[slot]->type != function.m_argument_types[slot])
                {
                    ostringstream oss;
                    oss << "Parser::function_call - " << name << ", argument " << slot + 1 << " has the wrong type at position " << m_pos;
                    throw Exception(oss.str().c_str(), __FILE__, __LINE__);
                }
            }

            return make_node(NodeKind::FunctionCall, function.m_return_type, name, arguments);
        }
        else if (first)
        {
            Node::pointer_type argument = expression(symbol_table);
            assert(argument);

            arguments.push_back(argument);

            first = false;
        }
        else if (accept(TokenType::Comma))
        {
            Node::pointer_type argument = expression(symbol_table);
            assert(argument);

            arguments.push_back(argument);
        }
        else
        {
            break;
        }
    }

    ostringstream oss;
    oss << "Parser::function_call - invalid token " << m_token_type << " at position " << m_pos;
    throw Exception(oss.str().c_str(), __FILE__, __LINE__);
}

//------------------------------------------------------------------------------
void quetzal::expression::Parser::next()
{
    if (m_pos == string::npos)
    {
        assert(m_token_type == TokenType::End);
        return;
    }

    m_pos = m_expression.find_first_not_of(' ', m_pos); // Skip spaces

    if (m_pos == string::npos)
    {
        m_token_type = TokenType::End;
        m_token.erase();
        return;
    }

    if (isdigit(m_expression[m_pos]))
    {
        m_token_type = TokenType::NumberLiteral;
        char* end = nullptr;
        strtod(&m_expression.c_str()[m_pos], &end);
        size_t length = end - &m_expression.c_str()[m_pos];
        m_token = m_expression.substr(m_pos, length);
        m_pos += length;
        return;
    }

    // A backslash escapes the character after it, so a string literal can contain a quote or a backslash
    if (m_expression[m_pos] == '"' || m_expression[m_pos] == '\'')
    {
        const size_t start = m_pos;
        m_quote = m_expression[m_pos];
        m_token.erase();

        for (++m_pos; m_pos < m_expression.size() && m_expression[m_pos] != m_quote; ++m_pos)
        {
            if (m_expression[m_pos] == '\\' && m_pos + 1 < m_expression.size())
            {
                ++m_pos;
            }

            m_token += m_expression[m_pos];
        }

        if (m_pos == m_expression.size())
        {
            ostringstream oss;
            oss << "Parser::next - unterminated string literal at position " << start;
            throw Exception(oss.str().c_str(), __FILE__, __LINE__);
        }

        m_token_type = TokenType::StringLiteral;
        ++m_pos;
        return;
    }

    if (isalpha(m_expression[m_pos]) || m_expression[m_pos] == '_')
    {
        size_t length = 0;
        while (m_pos + length < m_expression.size()
            && (isalnum(m_expression[m_pos + length]) || m_expression[m_pos + length] == '_' || m_expression[m_pos + length] == '.'))
        {
            ++length;
        }
        m_token = m_expression.substr(m_pos, length);
        m_pos += length;

        if (m_functions.find(m_token) != m_functions.end())
        {
            m_token_type = TokenType::FunctionCall;
            return;
        }

        m_token_type = TokenType::Identifier;
        return;
    }

    switch (m_expression[m_pos])
    {
        case '+':
            m_token_type = TokenType::OperatorAdd;
            m_token = m_expression[m_pos];
            ++m_pos;
            return;

        case '-':
            m_token_type = TokenType::OperatorSubtract;
            m_token = m_expression[m_pos];
            ++m_pos;
            return;

        case '*':
            m_token_type = TokenType::OperatorMultiply;
            m_token = m_expression[m_pos];
            ++m_pos;
            return;

        case '/':
            m_token_type = TokenType::OperatorDivide;
            m_token = m_expression[m_pos];
            ++m_pos;
            return;

        case '(':
            m_token_type = TokenType::LeftParen;
            m_token = m_expression[m_pos];
            ++m_pos;
            return;

        case ')':
            m_token_type = TokenType::RightParen;
            m_token = m_expression[m_pos];
            ++m_pos;
            return;

        case ',':
            m_token_type = TokenType::Comma;
            m_token = m_expression[m_pos];
            ++m_pos;
            return;
    }

    ostringstream oss;
    oss << "Parser::next - invalid character encountered " << m_expression[m_pos] << " at position " << m_pos;
    throw Exception(oss.str().c_str(), __FILE__, __LINE__);
}

//------------------------------------------------------------------------------
bool quetzal::expression::Parser::accept(TokenType token_type)
{
    if (m_token_type == token_type)
    {
        next();
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
bool quetzal::expression::Parser::expect(TokenType token_type)
{
    if (accept(token_type))
    {
        return true;
    }

    ostringstream oss;
    oss << "Parser::expect - invalid token " << m_token_type << ", " << token_type << " expected at position " << m_pos;
    throw Exception(oss.str().c_str(), __FILE__, __LINE__);
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
