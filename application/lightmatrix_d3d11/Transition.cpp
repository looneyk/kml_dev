//------------------------------------------------------------------------------
// Light Matrix Simulation
// Transition.cpp
//------------------------------------------------------------------------------

#include "Transition.hpp"
#include <cassert>

//------------------------------------------------------------------------------
quetzal::lightmatrix::Transition::Transition() :
    m_type(Transition::Cut),
    m_duration(0.0f),
    m_time(0.0f),
    m_transitioning(false)
{
}

//------------------------------------------------------------------------------
quetzal::lightmatrix::Transition::Type quetzal::lightmatrix::Transition::type() const
{
    return m_type;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Transition::set_type(Transition::Type type)
{
    m_type = type;
    return;
}

//------------------------------------------------------------------------------
float quetzal::lightmatrix::Transition::duration() const
{
    return m_duration;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Transition::set_duration(float duration)
{
    m_duration = duration;
    return;
}

//------------------------------------------------------------------------------
float quetzal::lightmatrix::Transition::progress() const
{
    assert(m_transitioning);
    assert(m_duration > 0.0f);

    return m_time / m_duration;
}

//------------------------------------------------------------------------------
bool quetzal::lightmatrix::Transition::transitioning() const
{
    return m_transitioning;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Transition::set_transitioning(bool activated)
{
    m_time = 0.0f;
    m_transitioning = activated;
    return;
}

//------------------------------------------------------------------------------
bool quetzal::lightmatrix::Transition::update(float dt)
{
    if (m_transitioning)
    {
        m_time += dt;
        if (m_time >= m_duration)
        {
            m_transitioning = false;
            return true;
        }
    }

    return false;
}
