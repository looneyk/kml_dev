//------------------------------------------------------------------------------
// FeatureDoor.cpp
//------------------------------------------------------------------------------

#include "FeatureDoor.hpp"

using namespace std;

//------------------------------------------------------------------------------
quetzal::roguelike::FeatureDoor::FeatureDoor(id_type template_id, id_type id) :
    Feature(template_id, id)
    m_bActivated(false),
    m_walls()
{
}

//------------------------------------------------------------------------------
quetzal::roguelike::id_type quetzal::roguelike::FeatureDoor::factory_id()
{
    return "Door";
}

//------------------------------------------------------------------------------
bool quetzal::roguelike::FeatureDoor::blocked() const
{
    return !m_bActivated;
}

//------------------------------------------------------------------------------
bool quetzal::roguelike::FeatureDoor::visible() const
{
    return true;
}

//------------------------------------------------------------------------------
bool quetzal::roguelike::FeatureDoor::activated() const
{
    return m_bActivated;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::FeatureDoor::activate(bool b)
{
    m_bActivated = b;
    return;
}
