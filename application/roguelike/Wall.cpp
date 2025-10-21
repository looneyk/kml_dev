//------------------------------------------------------------------------------
// Wall.cpp
//------------------------------------------------------------------------------

#include "Wall.hpp"
#include "Cell.hpp"

using namespace std;

//------------------------------------------------------------------------------
quetzal::roguelike::Wall::Wall() :
    Container(),
    m_pcell(nullptr)
{
}

//------------------------------------------------------------------------------
quetzal::roguelike::Cell* quetzal::roguelike::Wall::cell() const
{
    return m_pcell;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Wall::set_cell(Cell* pcell)
{
    m_pcell = pcell;
    return;
}
