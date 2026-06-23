//------------------------------------------------------------------------------
// Light Matrix Simulation
// Animation.cpp
//------------------------------------------------------------------------------

#include "Animation.hpp"
#include <cassert>

using namespace std;

//------------------------------------------------------------------------------
quetzal::lightmatrix::Animation::Animation() :
    m_shift_color(false),
    m_shift_brightness(false),
    m_smoothing(Smoothing::Nearest),
    m_velocity_orig(),
    m_velocity(),
    m_direction(),
    m_animating(false)
{
}

//------------------------------------------------------------------------------
const bool quetzal::lightmatrix::Animation::shift_color() const
{
    return m_shift_color;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Animation::set_shift_color(bool shift)
{
    m_shift_color = shift;
    set_animating(m_shift_color || m_shift_brightness); // in update? ...
    return;
}

//------------------------------------------------------------------------------
const bool quetzal::lightmatrix::Animation::shift_brightness() const
{
    return m_shift_brightness;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Animation::set_shift_brightness(bool shift)
{
    m_shift_brightness = shift;
    set_animating(m_shift_color || m_shift_brightness); // in update? ...
    return;
}

//------------------------------------------------------------------------------
const quetzal::lightmatrix::Smoothing quetzal::lightmatrix::Animation::smoothing() const
{
    return m_smoothing;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Animation::set_smoothing(Smoothing smoothing)
{
    m_smoothing = smoothing;
    return;
}

//------------------------------------------------------------------------------
const quetzal::lightmatrix::vector_type quetzal::lightmatrix::Animation::velocity() const
{
    return m_velocity;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Animation::set_velocity_default(const vector_type& v)
{
    m_velocity_orig = v;
    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Animation::set_velocity(const vector_type& v)
{
    m_velocity = v;
    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Animation::increment_velocity(const vector_type& dv)
{
    m_velocity += dv;
    m_velocity.set_x(max(m_velocity.x(), 0.0f));
    m_velocity.set_y(max(m_velocity.y(), 0.0f));
    return;
}

//------------------------------------------------------------------------------
const quetzal::lightmatrix::vector_type quetzal::lightmatrix::Animation::direction() const
{
    return m_direction;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Animation::set_direction(const vector_type& v)
{
    m_direction = v;
    return;
}

//------------------------------------------------------------------------------
const bool quetzal::lightmatrix::Animation::animating() const
{
    return m_animating;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Animation::set_animating(bool activated)
{
    m_animating = activated;
    return;
}
