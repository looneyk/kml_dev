//------------------------------------------------------------------------------
// PropertyTarget.cpp
//
//------------------------------------------------------------------------------

#include "PropertyTarget.hpp"
#include <cassert>

using namespace std;

//------------------------------------------------------------------------------
quetzal::roguelike::PropertyTarget::PropertyTarget() :
    m_properties()
{
}

//------------------------------------------------------------------------------
string quetzal::roguelike::PropertyTarget::property(const string& name) const
{
    const auto& i = m_properties.find(name);
    if (i != m_properties.end())
    {
        return i->second->value();
    }

    return string();
}

//------------------------------------------------------------------------------
void quetzal::roguelike::PropertyTarget::set_property(const string& name, const string& value)
{
    const auto& i = m_properties.find(name);
    if (i != m_properties.end())
    {
        i->second->set_value(value);
        return;
    }

    register_property(name, value);
    return;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::PropertyTarget::register_property(const string& name, const string& value)
{
    assert(m_properties.find(name) == m_properties.end());

    m_properties[name] = std::shared_ptr<Property>(new Property(name, value));
    return;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::PropertyTarget::remove_property(const string& name)
{
    Properties::const_iterator i = m_properties.find(name);
    if (i != m_properties.end())
    {
        m_properties.erase(i);
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::PropertyTarget::clear_properties()
{
    for (Properties::const_iterator i = m_properties.begin(); i != m_properties.end(); ++i)
    {
        m_properties.erase(i);
    }

    return;
}
