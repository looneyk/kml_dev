//------------------------------------------------------------------------------
// DiscreteDirection.cpp
//------------------------------------------------------------------------------

#include "DiscreteDirection.hpp"

//------------------------------------------------------------------------------
quetzal::roguelike::DiscreteDirection::DiscreteDirection() :
    m_x(0),
    m_y(0),
    m_z(0)
{
}

//------------------------------------------------------------------------------
quetzal::roguelike::DiscreteDirection::DiscreteDirection(int x, int y, int z) :
    m_x(x),
    m_y(y),
    m_z(z)
{
}

//------------------------------------------------------------------------------
int quetzal::roguelike::DiscreteDirection::x()
{
    return m_x;
}

//------------------------------------------------------------------------------
int quetzal::roguelike::DiscreteDirection::y()
{
    return m_y;
}

//------------------------------------------------------------------------------
int quetzal::roguelike::DiscreteDirection::z()
{
    return m_z;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::DiscreteDirection::set(int x, int y)
{
    m_x = x;
    m_y = y;
    return;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::DiscreteDirection::set(int x, int y, int z)
{
    m_x = x;
    m_y = y;
    m_z = z;
    return;
}
