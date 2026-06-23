//------------------------------------------------------------------------------
// Light Matrix Simulation
// Pixel.cpp
//------------------------------------------------------------------------------

#include "Pixel.hpp"
#include <iostream>
#include <cassert>

using namespace std;

//------------------------------------------------------------------------------
quetzal::lightmatrix::Pixel::Pixel() :
    m_position(),
    m_radius(1.0f),
    m_color()
{
}

//------------------------------------------------------------------------------
quetzal::lightmatrix::point_type quetzal::lightmatrix::Pixel::position() const
{
    return m_position;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Pixel::set_position(point_type position)
{
    m_position = position;
    return;
}

//------------------------------------------------------------------------------
float quetzal::lightmatrix::Pixel::radius() const
{
    return m_radius;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Pixel::set_radius(float r)
{
    m_radius = r;
    return;
}

//------------------------------------------------------------------------------
quetzal::lightmatrix::Color quetzal::lightmatrix::Pixel::color() const
{
    return m_color;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Pixel::set_color(Color color)
{
    m_color = color;

    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Pixel::set_rgb(Color color)
{
    m_color.set_rgb(color);

    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Pixel::set_a(Color color)
{
    m_color.set_a(color.a());

    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Pixel::set_rgb(component_type r, component_type g, component_type b)
{
    m_color.set_rgb(r, g, b);

    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Pixel::set_a(component_type a)
{
    m_color.set_a(a);

    return;
}

//------------------------------------------------------------------------------
quetzal::lightmatrix::Pixel quetzal::lightmatrix::lerp(const Pixel& pixel0, const Pixel& pixel1, float t)
{
    Pixel pixel = pixel0;
    pixel.set_color(lerp(pixel0.color(), pixel1.color(), t));
    return pixel;
}

//------------------------------------------------------------------------------
ostream& quetzal::lightmatrix::operator<<(ostream& os, const quetzal::lightmatrix::Pixel& pixel)
{
    os << pixel.color();

    return os;
}

//------------------------------------------------------------------------------
istream& quetzal::lightmatrix::operator>>(istream& is, quetzal::lightmatrix::Pixel& pixel)
{
    Color color;

    is >> color;
    pixel.set_color(color);

    return is;
}
