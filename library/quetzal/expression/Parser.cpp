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
    m_token()
{
    init_functions();
}

//------------------------------------------------------------------------------
quetzal::expression::Parser::~Parser()
{
}

//------------------------------------------------------------------------------
quetzal::expression::Parser::Function::Function() :
    m_name(),
    m_argument_count(0)
{
}

//------------------------------------------------------------------------------
quetzal::expression::Parser::Function::Function(const string& name, size_t argument_count) :
    m_name(name),
    m_argument_count(argument_count)
{
}

//------------------------------------------------------------------------------
quetzal::expression::Node::pointer_type quetzal::expression::Parser::make_node(NodeKind kind, string text, vector<Node::pointer_type> children)
{
    auto n = make_shared<Node>();
    n->kind = kind;
    n->text = std::move(text);
    n->children = std::move(children);
    return n;
}

//------------------------------------------------------------------------------
void quetzal::expression::Parser::init_functions()
{
    m_functions["abs"] = Function("abs", 1);
    m_functions["min"] = Function("min", 2);
    m_functions["max"] = Function("max", 2);
    m_functions["mod"] = Function("mod", 2);

    return;
}

//------------------------------------------------------------------------------
std::optional<quetzal::expression::Expression> quetzal::expression::Parser::parse(const string& str)
{
    try
    {
        m_expression = str;
        m_pos = 0;

        next();
        return Expression(expression());
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
quetzal::expression::Node::pointer_type quetzal::expression::Parser::expression()
{
    Node::pointer_type node = term();
    assert(node);

    while (m_token_type != TokenType::End && precedence(m_token_type) >= precedence(TokenType::OperatorAdd))
    {
        if (accept(TokenType::OperatorAdd))
        {
            node = make_node(NodeKind::BinaryOp, "+", { node, term() });
        }
        else if (accept(TokenType::OperatorSubtract))
        {
            node = make_node(NodeKind::BinaryOp, "-", { node, term() });
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
quetzal::expression::Node::pointer_type quetzal::expression::Parser::term()
{
    Node::pointer_type node = factor();
    assert(node);

    while (m_token_type != TokenType::End && precedence(m_token_type) >= precedence(TokenType::OperatorMultiply))
    {
        if (accept(TokenType::OperatorMultiply))
        {
            node = make_node(NodeKind::BinaryOp, "*", { node, factor() });
        }
        else if (accept(TokenType::OperatorDivide))
        {
            node = make_node(NodeKind::BinaryOp, "/", { node, factor() });
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
quetzal::expression::Node::pointer_type quetzal::expression::Parser::factor()
{
    string previous_token = m_token;

    if (accept(TokenType::Literal))
    {
        return make_node(NodeKind::Literal, previous_token, {});
    }
    else if (accept(TokenType::Identifier))
    {
        return make_node(NodeKind::Identifier, previous_token, {});
    }
    else if (accept(TokenType::FunctionCall))
    {
        return function_call(previous_token);
    }
    else if (accept(TokenType::LeftParen))
    {
        Node::pointer_type node = expression();
        assert(node);
        node->parenthesized = true;
        expect(TokenType::RightParen);
        return node;
    }
    else if (accept(TokenType::OperatorSubtract))
    {
        return make_node(NodeKind::UnaryOp, "-", { factor() });
    }

    ostringstream oss;
    oss << "Parser::factor - invalid token " << m_token_type << " at position " << m_pos;
    throw Exception(oss.str().c_str(), __FILE__, __LINE__);
}

//------------------------------------------------------------------------------
quetzal::expression::Node::pointer_type quetzal::expression::Parser::function_call(const string& name)
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
            if (arguments.size() != i->second.m_argument_count)
            {
                ostringstream oss;
                oss << "Parser::function_call - " << name << ", "<< arguments.size() << " arguments found, " << i->second.m_argument_count
                    << " arguments expected at position " << m_pos;
                throw Exception(oss.str().c_str(), __FILE__, __LINE__);
            }

            return make_node(NodeKind::FunctionCall, name, arguments);
        }
        else if (first)
        {
            Node::pointer_type argument = expression();
            assert(argument);

            arguments.push_back(argument);

            first = false;
        }
        else if (accept(TokenType::Comma))
        {
            Node::pointer_type argument = expression();
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
        m_token_type = TokenType::Literal;
        char* end = nullptr;
        strtod(&m_expression.c_str()[m_pos], &end);
        size_t length = end - &m_expression.c_str()[m_pos];
        m_token = m_expression.substr(m_pos, length);
        m_pos += length;
        return;
    }

    if (isalpha(m_expression[m_pos]) || m_expression[m_pos] == '_')
    {
        size_t length = 0;
        while (m_pos + length < m_expression.size() && (isalnum(m_expression[m_pos + length]) || m_expression[m_pos + length] == '_'))
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

        case Parser::TokenType::Literal:
            os << "literal";
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
