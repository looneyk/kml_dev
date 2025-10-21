//------------------------------------------------------------------------------
// hex_level
// Level.cpp
//------------------------------------------------------------------------------

#include "Level.hpp"

using namespace std;

//------------------------------------------------------------------------------
quetzal::hex_level::Level::Level() :
    m_position(),
    m_size(),
    m_cells()
{
}

//------------------------------------------------------------------------------
quetzal::hex_level::Level::Level(float x, float y, float z, float dx, float dy, float dz) :
    m_position(x, y, z),
    m_size(dx, dy, dz),
    m_cells(static_cast<int>(dx * dy))
{
}

//------------------------------------------------------------------------------
quetzal::hex_level::Position quetzal::hex_level::Level::position() const
{
    return m_position;
}

//------------------------------------------------------------------------------
quetzal::hex_level::Size quetzal::hex_level::Level::size() const
{
    return m_size;
}

//------------------------------------------------------------------------------
const vector<quetzal::hex_level::Cell>& quetzal::hex_level::Level::cells() const
{
    return m_cells;
}

//------------------------------------------------------------------------------
void quetzal::hex_level::Level::set_position(float x, float y, float z)
{
    m_position.set(x, y, z);
    return;
}

//------------------------------------------------------------------------------
void quetzal::hex_level::Level::set_size(float dx, float dy, float dz)
{
    m_size.set(dx, dy, dz);
    m_cells.resize(static_cast<int>(dx * dy)); // grid, not float dimensions ...
    return;
}

//------------------------------------------------------------------------------
vector<quetzal::hex_level::Cell>& quetzal::hex_level::Level::cells()
{
    return m_cells;
}

//------------------------------------------------------------------------------
quetzal::hex_level::Cell& quetzal::hex_level::Level::cell(int x, int y)
{
    return m_cells[y * /*m_size.dx()*/0 + x]; // grid dimensions ...
}
