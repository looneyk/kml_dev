//------------------------------------------------------------------------------
// roguelike
// Level.cpp
//------------------------------------------------------------------------------

#include "Level.hpp"
#include "Dungeon.hpp"

using namespace std;

//------------------------------------------------------------------------------
quetzal::roguelike::Level::Level() :
    m_pdungeon(nullptr),
    m_position(),
    m_size(),
    m_cells()
{
}

//------------------------------------------------------------------------------
quetzal::roguelike::Level::Level(Dungeon* pdungeon, int z, int dx, int dy) :
    m_pdungeon(pdungeon),
    m_position(0, 0, z),
    m_size(dx, dy),
    m_cells(static_cast<int>(dx * dy))
{
}

//------------------------------------------------------------------------------
quetzal::roguelike::Dungeon* quetzal::roguelike::Level::dungeon() const
{
    return m_pdungeon;
}

//------------------------------------------------------------------------------
quetzal::roguelike::DiscretePosition quetzal::roguelike::Level::position() const
{
    return m_position;
}

//------------------------------------------------------------------------------
quetzal::roguelike::DiscreteSize quetzal::roguelike::Level::size() const
{
    return m_size;
}

//------------------------------------------------------------------------------
const vector<quetzal::roguelike::Cell>& quetzal::roguelike::Level::cells() const
{
    return m_cells;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Level::set_dungeon(Dungeon* pdungeon)
{
    m_pdungeon = pdungeon;
    return;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Level::set_position(int x, int y, int z)
{
    m_position.set(x, y, z);
    return;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Level::set_size(int dx, int dy)
{
    m_size.set(dx, dy);
    m_cells.resize(static_cast<int>(dx * dy));
    return;
}

//------------------------------------------------------------------------------
vector<quetzal::roguelike::Cell>& quetzal::roguelike::Level::cells()
{
    return m_cells;
}

//------------------------------------------------------------------------------
quetzal::roguelike::Cell& quetzal::roguelike::Level::cell(int x, int y)
{
    return m_cells[y * m_size.dx() + x];
}
