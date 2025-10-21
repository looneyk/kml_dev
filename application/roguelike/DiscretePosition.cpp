//------------------------------------------------------------------------------
// DiscretePosition.cpp
//------------------------------------------------------------------------------

#include "DiscretePosition.hpp"

//------------------------------------------------------------------------------
quetzal::roguelike::DiscretePosition::DiscretePosition() :
    m_x(0),
    m_y(0),
    m_z(0)
{
}

//------------------------------------------------------------------------------
quetzal::roguelike::DiscretePosition::DiscretePosition(int x, int y, int z) :
    m_x(x),
    m_y(y),
    m_z(z)
{
}

//------------------------------------------------------------------------------
int quetzal::roguelike::DiscretePosition::x()
{
    return m_x;
}

//------------------------------------------------------------------------------
int quetzal::roguelike::DiscretePosition::y()
{
    return m_y;
}

//------------------------------------------------------------------------------
int quetzal::roguelike::DiscretePosition::z()
{
    return m_z;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::DiscretePosition::set(int x, int y)
{
    m_x = x;
    m_y = y;
    return;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::DiscretePosition::set(int x, int y, int z)
{
    m_x = x;
    m_y = y;
    m_z = z;
    return;
}
