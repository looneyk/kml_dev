
#include "Parse.h"

#include "Expression.h"
#include <cassert>

using namespace std;


namespace
{

    // We love our operators.  Order here determines precedence.
    const string operators = "-+/*"; // operators were in the reverse order, fixed
	const string whitespace = " \n\t"; // moved whitespace here so it can be used by lex too

} // namespace

// Clean those dirty tokens
string clean(string token)
{
	string result;
	for (unsigned int i = 0; i < token.length(); i++)
	{
		if (whitespace.find(token[i]) == string::npos)
		{
			result.push_back(token[i]);
		}
	}

	return result;
}

// Takes a string and splits it into tokens
list<string> lex(string input)
{
    // Check for, strip off, and flag memory storage character M
    // Do this only if it is not part of an expression (previous token is not an operator)
    // Replace it with S to be used by parse (so a final M in an expression itself won't create ambiguity)
    bool bStore = false;
    bool bFound = false;
    size_t nStrip = 0;
    for (auto i = input.rbegin(); i != input.rend(); ++i)
    {
        if (whitespace.find(*i) != string::npos)
        {
            continue;
        }

        if (!bFound)
        {
            if (*i == 'M')
            {
                bFound = true;
            }
        }
        else
        {
            if (operators.find(*i) == string::npos)
            {
                bStore = true;
                input.resize(input.size() - nStrip);
            }
            break;
        }

        ++nStrip;
    }

	list<string> tokens;

	string token;
	for (unsigned int i = 0; i < input.length(); i++)
	{
		if (operators.find(input[i]) != string::npos)
		{
			// Add any token we've created so far
			if (token.length())
			{
				tokens.push_back(token);
				token.clear();
			}

			// Add this operator as a separate token
			token.push_back(input[i]);
			tokens.push_back(token);
			token.clear();
		}
		else
		{
			// Grow the current token
			token.push_back(input[i]);
		}
	}

	// Any stragglers?
	if (token.length())
	{
		tokens.push_back(token);
	}

    // Add S token to indicate storage of result in memory (S to differentiate it from M in an expression)
    if (bStore)
    {
        tokens.push_back("S");
    }

	// Clean 'em up
	for (list<string>::iterator i = tokens.begin(); i != tokens.end(); ++i)
	{
		*i = clean(*i);
	}

	return tokens;
}

// Constants
Expression* constantExpression(float rValue)
{
	ConstantExpression* result = new ConstantExpression;
	result->value = rValue;
	return result;
}

// Parse given a left, right, and op
Expression* parseBinary(list<string> left, string op, list<string> right)
{
	BinaryExpression* result = 0;
	if (op == "+")
	{
		result = new BinaryPlus;
	}
	else if (op == "-")
	{
		result = new BinaryMinus;
	}
	else if (op == "*")
	{
		result = new BinaryMultiply;
	}
	else if (op == "/")
	{
		result = new BinaryDivide;
	}

	result->left = parse(left, false);
	result->right = parse(right, false);
	return result;
}

// Parse using a given subdivision operator
Expression* splitAndParseBinary(list<string> tokens, string op)
{
	for (list<string>::iterator i = tokens.begin(); i != tokens.end(); ++i)
	{
		if (*i == op)
		{
			list<string> left;
			for (list<string>::iterator j = tokens.begin(); j != i; ++j)
			{
				left.push_back(*j);
			}
			list<string> right;
			for (list<string>::iterator j = ++i; j != tokens.end(); ++j)
			{
				right.push_back(*j);
			}

			return parseBinary(left, op, right);
		}
	}

	return 0;
}

// Takes a list of tokens and returns the expression tree
Expression* parse(list<string> tokens, bool bTop)
{
    // Check for and strip off S token at end of expression (translated by lex)
    bool bStore = false;
    if (bTop)
    {
        if (!tokens.empty() && tokens.back() == "S")
        {
            // Flag that expression result should be stored
            tokens.resize(tokens.size() - 1);
            bStore = true;
        }
    }

	Expression* result = nullptr;

	// The order of these operators determines the precedence
	for (unsigned int i = 0; i < operators.length(); i++)
	{
		string op;
		op.push_back(operators[i]);
		result = splitAndParseBinary(tokens, op);
		if (result)
		{
            break;
		}
	}

	if (result == 0) // Must be a constant or memory.
    {
	    if(tokens.begin() != tokens.end())  //  Could also be !tokens.empty()
	    {
		    const string& token = *tokens.begin();
            if (token == "M")
            {
                result = new MemoryExpression();
            }
            else
            {
    		    float rValue = (float)atof(token.c_str());
                result = constantExpression(rValue);
            }
	    }
        else
        {
            result = constantExpression(0.0f);
        }
    }

    assert(result != nullptr);
    result->set_store(bStore);
    return result;
}

