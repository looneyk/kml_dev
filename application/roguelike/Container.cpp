//------------------------------------------------------------------------------
// roguelike
// Container.cpp
//------------------------------------------------------------------------------

#include "Container.hpp"

//------------------------------------------------------------------------------
quetzal::roguelike::Container::Container() :
    Item(),
    m_items()
{
}

//------------------------------------------------------------------------------
float quetzal::roguelike::Container::weight() const
{
    return Item::weight() + m_items.weight();
}

//------------------------------------------------------------------------------
quetzal::roguelike::Items& quetzal::roguelike::Container::contents()
{
    return m_items;
}
