//------------------------------------------------------------------------------
// Light Matrix Simulation
// Viewport.cpp
//------------------------------------------------------------------------------

#include "Viewport.hpp"

//------------------------------------------------------------------------------
quetzal::lightmatrix::Viewport::Viewport(const point_type& position, const vector_type& size) :
    m_position(position),
    m_size(size)
{
}

//------------------------------------------------------------------------------
quetzal::lightmatrix::point_type quetzal::lightmatrix::Viewport::position() const
{
    return m_position;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Viewport::set_position(const point_type& position)
{
    m_position = position;
    return;
}

//------------------------------------------------------------------------------
quetzal::lightmatrix::vector_type quetzal::lightmatrix::Viewport::size() const
{
    return m_size;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Viewport::set_size(const vector_type& size)
{
    m_size = size;
    return;
}
