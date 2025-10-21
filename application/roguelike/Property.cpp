//------------------------------------------------------------------------------
// Property.cpp
//
//------------------------------------------------------------------------------

#include "Property.hpp"

using namespace std;

//------------------------------------------------------------------------------
quetzal::roguelike::Property::Property(const string& name, const string& value) :
    m_name(name),
    m_value(value)
{
}

//------------------------------------------------------------------------------
quetzal::roguelike::Property::~Property()
{
}

//------------------------------------------------------------------------------
string quetzal::roguelike::Property::name() const
{
    return m_name;
}

//----------------------------------------------------------------------
string quetzal::roguelike::Property::value() const
{
    return m_value;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Property::set_name(const string& name)
{
    m_name = name;
    return;
}

//----------------------------------------------------------------------
void quetzal::roguelike::Property::set_value(const string& value)
{
    m_value = value;
    return;
}
