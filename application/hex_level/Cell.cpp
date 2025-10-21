//------------------------------------------------------------------------------
// hex_level
// Cell.cpp
//------------------------------------------------------------------------------

#include "Cell.hpp"
#include "Level.hpp"

using namespace std;

//------------------------------------------------------------------------------
quetzal::hex_level::Cell::Cell() :
    m_plevel(nullptr),
    m_position(),
    m_walls()
{
}

//------------------------------------------------------------------------------
quetzal::hex_level::Level* quetzal::hex_level::Cell::level() const
{
    return m_plevel;
}

//------------------------------------------------------------------------------
quetzal::hex_level::Position quetzal::hex_level::Cell::position() const
{
    return m_position;
}

//------------------------------------------------------------------------------
const quetzal::hex_level::Cell::walls_type& quetzal::hex_level::Cell::walls() const
{
    return m_walls;
}

//------------------------------------------------------------------------------
void quetzal::hex_level::Cell::set_level(Level* plevel)
{
    m_plevel = plevel;
    return;
}

//------------------------------------------------------------------------------
void quetzal::hex_level::Cell::set_position(float x, float y, float z)
{
    m_position.set(x, y, z);
    return;
}

//------------------------------------------------------------------------------
quetzal::hex_level::Cell::walls_type& quetzal::hex_level::Cell::walls()
{
    return m_walls;
}
