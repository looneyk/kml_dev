//------------------------------------------------------------------------------
// Model.cpp
//------------------------------------------------------------------------------

#include "Model.hpp"

using namespace std;

//------------------------------------------------------------------------------
quetzal::hex_level::Model::Model() :
    m_normal()
{
}

//------------------------------------------------------------------------------
quetzal::hex_level::Position quetzal::hex_level::Model::normal() const
{
    return m_normal;
}

//------------------------------------------------------------------------------
void quetzal::hex_level::Model::set_normal(float x, float y, float z)
{
    m_normal.set(x, y, z);
    return;
}
