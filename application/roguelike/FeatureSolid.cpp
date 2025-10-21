//------------------------------------------------------------------------------
// FeatureSolid.cpp
//------------------------------------------------------------------------------

#include "FeatureSolid.hpp"

using namespace std;

//------------------------------------------------------------------------------
quetzal::roguelike::FeatureSolid::FeatureSolid(id_type template_id, id_type id) :
    Feature(template_id, id)
    m_walls()
{
}

//------------------------------------------------------------------------------
quetzal::roguelike::id_type quetzal::roguelike::FeatureSolid::factory_id()
{
    return "solid";
}

//------------------------------------------------------------------------------
bool quetzal::roguelike::FeatureSolid::blocked() const
{
    return true;
}

//------------------------------------------------------------------------------
bool quetzal::roguelike::FeatureSolid::visible() const
{
    return true;
}
