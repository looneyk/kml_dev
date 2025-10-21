//------------------------------------------------------------------------------
// Position.cpp
//------------------------------------------------------------------------------

#include "Position.hpp"

//------------------------------------------------------------------------------
quetzal::hex_level::Position::Position() :
    m_x(0.f),
    m_y(0.f),
    m_z(0.f)
{
}

//------------------------------------------------------------------------------
quetzal::hex_level::Position::Position(float x, float y, float z) :
    m_x(x),
    m_y(y),
    m_z(z)
{
}

//------------------------------------------------------------------------------
quetzal::hex_level::Position& quetzal::hex_level::Position::operator+=(const Position& other)
{
    m_x += other.x();
    m_y += other.y();
    m_z += other.z();
    return *this;
}

//------------------------------------------------------------------------------
float quetzal::hex_level::Position::x() const
{
    return m_x;
}

//------------------------------------------------------------------------------
float quetzal::hex_level::Position::y() const
{
    return m_y;
}

//------------------------------------------------------------------------------
float quetzal::hex_level::Position::z() const
{
    return m_z;
}

//------------------------------------------------------------------------------
void quetzal::hex_level::Position::set(float x, float y, float z)
{
    m_x = x;
    m_y = y;
    m_z = z;
    return;
}

//------------------------------------------------------------------------------
quetzal::hex_level::Position quetzal::hex_level::operator+(const Position& lhs, const Position& rhs)
{
    return Position(lhs.x() + rhs.x(), lhs.y() + rhs.y(), lhs.z() + rhs.z());
}
