
#include "Expression.h"

// Initialize the static memory value to 0
float MemoryExpression::value = 0.0f;

// Expression base class implementation with memory storage flag
Expression::Expression() :
    bStore(false)
{
}

void Expression::set_store(bool b)
{
    bStore = b;
}

void Expression::check_store(float f)
{
    if (bStore)
    {
        MemoryExpression::set(f);
    }
}

float ConstantExpression::evaluate()
{
    check_store(value);
	return value;
}

BinaryExpression::BinaryExpression()
:	left(0),
	right(0)
{
}

BinaryExpression::~BinaryExpression()
{
	delete left;
	delete right;
}

float BinaryPlus::evaluate()
{
	float value = left->evaluate() + right->evaluate();
    check_store(value);
	return value;
}

float BinaryMinus::evaluate()
{
	float value = left->evaluate() - right->evaluate();
    check_store(value);
	return value;
}

float BinaryMultiply::evaluate()
{
	float value = left->evaluate() * right->evaluate();
    check_store(value);
	return value;
}

float BinaryDivide::evaluate()
{
	float value = left->evaluate() / right->evaluate();
    check_store(value);
	return value;
}

// MemoryExpression implementation
float MemoryExpression::evaluate()
{
    check_store(value);
	return value;
}

void MemoryExpression::set(float f)
{
	value = f;
}
