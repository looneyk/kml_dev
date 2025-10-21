//------------------------------------------------------------------------------
// Wall.cpp
//------------------------------------------------------------------------------

#include "Wall.hpp"
#include "Cell.hpp"

using namespace std;

//------------------------------------------------------------------------------
quetzal::hex_level::Wall::Wall() :
    m_pcell(nullptr),
    m_position(),
    m_normal(),
    m_pmodel()
{
}

//------------------------------------------------------------------------------
quetzal::hex_level::Cell* quetzal::hex_level::Wall::cell() const
{
    return m_pcell;
}

//------------------------------------------------------------------------------
quetzal::hex_level::Position quetzal::hex_level::Wall::position() const
{
    return m_position;
}

//------------------------------------------------------------------------------
quetzal::hex_level::Position quetzal::hex_level::Wall::normal() const
{
    return m_position;
}

//------------------------------------------------------------------------------
const shared_ptr<quetzal::hex_level::Model> quetzal::hex_level::Wall::model() const
{
    return m_pmodel;
}

//------------------------------------------------------------------------------
void quetzal::hex_level::Wall::set_cell(Cell* pcell)
{
    m_pcell = pcell;
    return;
}

//------------------------------------------------------------------------------
void quetzal::hex_level::Wall::set_position(float x, float y, float z)
{
    m_position.set(x, y, z);
    return;
}

//------------------------------------------------------------------------------
void quetzal::hex_level::Wall::set_normal(float x, float y, float z)
{
    m_normal.set(x, y, z);
    return;
}

//------------------------------------------------------------------------------
void quetzal::hex_level::Wall::set_model(std::shared_ptr<Model> pmodel)
{
    m_pmodel = pmodel;
    return;
}
