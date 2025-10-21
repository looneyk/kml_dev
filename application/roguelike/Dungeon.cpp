//------------------------------------------------------------------------------
// roguelike
// Dungeon.cpp
//------------------------------------------------------------------------------

#include "Dungeon.hpp"

using namespace std;

//------------------------------------------------------------------------------
quetzal::roguelike::Dungeon::Dungeon() :
    m_levels()
{
}

//------------------------------------------------------------------------------
const vector<quetzal::roguelike::Level>& quetzal::roguelike::Dungeon::levels() const
{
    return m_levels;
}

//------------------------------------------------------------------------------
vector<quetzal::roguelike::Level>& quetzal::roguelike::Dungeon::levels()
{
    return m_levels;
}

//------------------------------------------------------------------------------
quetzal::roguelike::Level& quetzal::roguelike::Dungeon::level(int z)
{
    return m_levels[z];
}
