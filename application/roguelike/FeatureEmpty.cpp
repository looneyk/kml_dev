//------------------------------------------------------------------------------
// FeatureEmpty.cpp
//------------------------------------------------------------------------------

#include "FeatureEmpty.hpp"

using namespace std;

//------------------------------------------------------------------------------
quetzal::roguelike::FeatureEmpty::FeatureEmpty(id_type template_id, id_type id) :
    Feature(template_id, id)
    m_walls()
{
}

//------------------------------------------------------------------------------
quetzal::roguelike::id_type quetzal::roguelike::FeatureEmpty::factory_id()
{
    return "empty";
}

//------------------------------------------------------------------------------
bool quetzal::roguelike::FeatureEmpty::blocked() const
{
    return false;
}

//------------------------------------------------------------------------------
bool quetzal::roguelike::FeatureEmpty::visible() const
{
    return true;
}
