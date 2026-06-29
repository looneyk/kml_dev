//------------------------------------------------------------------------------
// Parser.cpp
//
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
    m_tokenType(TokenType::Null),
    m_token()
{
    init_functions();
}

//------------------------------------------------------------------------------
quetzal::expression::Parser::~Parser()
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
    Node::pointer_type pterm = term();
    assert(pterm);

    while (m_tokenType != TokenType::End && precedence(m_tokenType) >= precedence(TokenType::OperatorAdd))
    {
        if (accept(TokenType::OperatorAdd))
        {
            pterm = make_node(NodeKind::BinaryOp, "+", { pterm, term() });
        }
        else if (accept(TokenType::OperatorSubtract))
        {
            pterm = make_node(NodeKind::BinaryOp, "-", { pterm, term() });
        }
        else
        {
            ostringstream oss;
            oss << "Parser::expression - invalid token " << m_tokenType << " at position " << m_pos;
            throw Exception(oss.str().c_str(), __FILE__, __LINE__);
        }
    }

    return pterm;
}

//------------------------------------------------------------------------------
quetzal::expression::Node::pointer_type quetzal::expression::Parser::term()
{
    Node::pointer_type pfactor = factor();
    assert(pfactor);

    while (m_tokenType != TokenType::End && precedence(m_tokenType) >= precedence(TokenType::OperatorMultiply))
    {
        if (accept(TokenType::OperatorMultiply))
        {
            pfactor = make_node(NodeKind::BinaryOp, "*", { pfactor, factor() });
        }
        else if (accept(TokenType::OperatorDivide))
        {
            pfactor = make_node(NodeKind::BinaryOp, "/", { pfactor, factor() });
        }
        else
        {
            ostringstream oss;
            oss << "Parser::term - invalid token " << m_tokenType << " at position " << m_pos;
            throw Exception(oss.str().c_str(), __FILE__, __LINE__);
        }
    }

    return pfactor;
}

//------------------------------------------------------------------------------
quetzal::expression::Node::pointer_type quetzal::expression::Parser::factor()
{
    string tokenPrev = m_token;

    if (accept(TokenType::Literal))
    {
        return make_node(NodeKind::Literal, tokenPrev, {});
    }
    else if (accept(TokenType::Identifier))
    {
        return make_node(NodeKind::Identifier, tokenPrev, {});
    }
    else if (accept(TokenType::FunctionCall))
    {
        return function_call(tokenPrev);
    }
    else if (accept(TokenType::LeftParen))
    {
        Node::pointer_type pe = expression();
        assert(pe);
        pe->parenthesized = true;
        expect(TokenType::RightParen);
        return pe;
    }
    else if (accept(TokenType::OperatorSubtract))
    {
        return make_node(NodeKind::UnaryOp, "-", { factor() });
    }

    ostringstream oss;
    oss << "Parser::factor - invalid token " << m_tokenType << " at position " << m_pos;
    throw Exception(oss.str().c_str(), __FILE__, __LINE__);
}

//------------------------------------------------------------------------------
quetzal::expression::Node::pointer_type quetzal::expression::Parser::function_call(const string& name)
{
    Functions::const_iterator i = m_functions.find(name);
    assert(i != m_functions.end());

    expect(TokenType::LeftParen);

    vector<Node::pointer_type> pes;
    bool bFirst = true;

    for (; ; )
    {
        if (accept(TokenType::RightParen))
        {
            if (pes.size() != i->second.m_nArguments)
            {
                ostringstream oss;
                oss << "Parser::function_call - " << name << ", "<< pes.size() << " arguments found, " << i->second.m_nArguments << " arguments expected at position " << m_pos;
                throw Exception(oss.str().c_str(), __FILE__, __LINE__);
            }

            return make_node(NodeKind::FunctionCall, name, pes);
        }
        else if (bFirst)
        {
            Node::pointer_type pe = expression();
            assert(pe);

            pes.push_back(pe);

            bFirst = false;
        }
        else if (accept(TokenType::Comma))
        {
            Node::pointer_type pe = expression();
            assert(pe);

            pes.push_back(pe);
        }
        else
        {
            break;
        }
    }

    ostringstream oss;
    oss << "Parser::function_call - invalid token " << m_tokenType << " at position " << m_pos;
    throw Exception(oss.str().c_str(), __FILE__, __LINE__);
}

//------------------------------------------------------------------------------
void quetzal::expression::Parser::next()
{
    if (m_pos == string::npos)
    {
        assert(m_tokenType == TokenType::End);
        return;
    }

    m_pos = m_expression.find_first_not_of(' ', m_pos); // Skip spaces

    if (m_pos == string::npos)
    {
        m_tokenType = TokenType::End;
        m_token.erase();
        return;
    }

    if (isdigit(m_expression[m_pos]))
    {
        m_tokenType = TokenType::Literal;
        char* pend = nullptr;
        strtod(&m_expression.c_str()[m_pos], &pend);
        size_t length = pend - &m_expression.c_str()[m_pos];
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
            m_tokenType = TokenType::FunctionCall;
            return;
        }

        m_tokenType = TokenType::Identifier;
        return;
    }

    switch (m_expression[m_pos])
    {
        case '+':
            m_tokenType = TokenType::OperatorAdd;
            m_token = m_expression[m_pos];
            ++m_pos;
            return;

        case '-':
            m_tokenType = TokenType::OperatorSubtract;
            m_token = m_expression[m_pos];
            ++m_pos;
            return;

        case '*':
            m_tokenType = TokenType::OperatorMultiply;
            m_token = m_expression[m_pos];
            ++m_pos;
            return;

        case '/':
            m_tokenType = TokenType::OperatorDivide;
            m_token = m_expression[m_pos];
            ++m_pos;
            return;

        case '(':
            m_tokenType = TokenType::LeftParen;
            m_token = m_expression[m_pos];
            ++m_pos;
            return;

        case ')':
            m_tokenType = TokenType::RightParen;
            m_token = m_expression[m_pos];
            ++m_pos;
            return;

        case ',':
            m_tokenType = TokenType::Comma;
            m_token = m_expression[m_pos];
            ++m_pos;
            return;
    }

    ostringstream oss;
    oss << "Parser::next - invalid character encountered " << m_expression[m_pos] << " at position " << m_pos;
    throw Exception(oss.str().c_str(), __FILE__, __LINE__);
}

//------------------------------------------------------------------------------
bool quetzal::expression::Parser::accept(TokenType tokenType)
{
    if (m_tokenType == tokenType)
    {
        next();
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
bool quetzal::expression::Parser::expect(TokenType tokenType)
{
    if (accept(tokenType))
    {
        return true;
    }

    ostringstream oss;
    oss << "Parser::expect - invalid token " << m_tokenType << ", " << tokenType << " expected at position " << m_pos;
    throw Exception(oss.str().c_str(), __FILE__, __LINE__);
}

//------------------------------------------------------------------------------
ostream& quetzal::expression::operator<<(ostream& os, const Parser::TokenType& tokenType)
{
    switch (tokenType)
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
