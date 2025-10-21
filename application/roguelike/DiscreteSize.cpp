//------------------------------------------------------------------------------
// DiscreteSize.cpp
//------------------------------------------------------------------------------

#include "DiscreteSize.hpp"

//------------------------------------------------------------------------------
quetzal::roguelike::DiscreteSize::DiscreteSize() :
    m_dx(0),
    m_dy(0),
    m_dz(0)
{
}

//------------------------------------------------------------------------------
quetzal::roguelike::DiscreteSize::DiscreteSize(int dx, int dy, int dz) :
    m_dx(dx),
    m_dy(dy),
    m_dz(dz)
{
}

//------------------------------------------------------------------------------
int quetzal::roguelike::DiscreteSize::dx()
{
    return m_dx;
}

//------------------------------------------------------------------------------
int quetzal::roguelike::DiscreteSize::dy()
{
    return m_dy;
}

//------------------------------------------------------------------------------
int quetzal::roguelike::DiscreteSize::dz()
{
    return m_dz;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::DiscreteSize::set(int dx, int dy, int dz)
{
    m_dx = dx;
    m_dy = dy;
    m_dz = dz;
    return;
}
