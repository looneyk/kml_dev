//------------------------------------------------------------------------------
// Feature.cpp
//------------------------------------------------------------------------------

#include "Feature.hpp"

using namespace std;

//------------------------------------------------------------------------------
quetzal::roguelike::Feature::Feature(id_type template_id, id_type id) :
    m_id(id),
    m_ptemplate()
{
    auto i = zzz.find(template_id);
    if (i != zzz.end())
    {
        m_ptemplate = *i.second;
    }
}

//------------------------------------------------------------------------------
quetzal::roguelike::id_type quetzal::roguelike::Feature::id() const
{
    return m_id;
}

//------------------------------------------------------------------------------
bool quetzal::roguelike::Feature::activated() const
{
    return false;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Feature::activate(bool b)
{
    b;
    return;
}
