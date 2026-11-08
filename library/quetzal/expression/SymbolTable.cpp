//------------------------------------------------------------------------------
// expression
// SymbolTable.cpp
//------------------------------------------------------------------------------

#include "SymbolTable.hpp"
#include <utility>

using namespace std;

//------------------------------------------------------------------------------
quetzal::expression::Symbol::Symbol(DataType type, string value) :
    type(type),
    value(std::move(value))
{
}

//------------------------------------------------------------------------------
quetzal::expression::Symbol::Symbol(const char* value) :
    type(DataType::String),
    value(value)
{
}

//------------------------------------------------------------------------------
quetzal::expression::Symbol::Symbol(const string& value) :
    type(DataType::String),
    value(value)
{
}
