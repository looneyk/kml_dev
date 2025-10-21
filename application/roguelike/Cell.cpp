//------------------------------------------------------------------------------
// roguelike
// Cell.cpp
//------------------------------------------------------------------------------

#include "Cell.hpp"
#include "Level.hpp"

using namespace std;

//------------------------------------------------------------------------------
quetzal::roguelike::Cell::Cell() :
    Container(),
    m_plevel(nullptr),
    m_walls()
{
}

//------------------------------------------------------------------------------
quetzal::roguelike::Level* quetzal::roguelike::Cell::level() const
{
    return m_plevel;
}

//------------------------------------------------------------------------------
const quetzal::roguelike::Cell::walls_type& quetzal::roguelike::Cell::walls() const
{
    return m_walls;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Cell::set_level(Level* plevel)
{
    m_plevel = plevel;
    return;
}

//------------------------------------------------------------------------------
quetzal::roguelike::Cell::walls_type& quetzal::roguelike::Cell::walls()
{
    return m_walls;
}
