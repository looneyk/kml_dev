//------------------------------------------------------------------------------
// Light Matrix Simulation
// Scene.cpp
//------------------------------------------------------------------------------

#include "Scene.hpp"
#include <cmath>

using namespace std;

namespace
{

    //--------------------------------------------------------------------------
    quetzal::lightmatrix::Color sample_pattern(const quetzal::lightmatrix::Pattern& pattern, float u, float v, quetzal::lightmatrix::Smoothing smoothing)
    {
        using namespace quetzal::lightmatrix;

        const auto [w, h] = pattern.size();
        if (w == 0 || h == 0)
        {
            return Color();
        }

        u -= floor(u);
        v -= floor(v);

        float fx = u * static_cast<float>(w);
        float fy = v * static_cast<float>(h);

        size_t x0 = static_cast<size_t>(fx) % w;
        size_t y0 = static_cast<size_t>(fy) % h;

        if (smoothing == Smoothing::Nearest)
        {
            return pattern.color(x0, y0);
        }

        float tx = fx - floor(fx);
        float ty = fy - floor(fy);

        size_t x1 = (x0 + 1) % w;
        size_t y1 = (y0 + 1) % h;

        Color c0 = lerp(pattern.color(x0, y0), pattern.color(x1, y0), tx);
        Color c1 = lerp(pattern.color(x0, y1), pattern.color(x1, y1), tx);
        return lerp(c0, c1, ty);
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::lightmatrix::Scene::Scene() :
    m_panel(),
    m_frame(),
    m_patterns(),
    m_viewport(),
    m_animation(),
    m_transition()
{
}

//------------------------------------------------------------------------------
const quetzal::lightmatrix::Panel& quetzal::lightmatrix::Scene::panel() const
{
    return m_panel;
}

//------------------------------------------------------------------------------
quetzal::lightmatrix::Panel& quetzal::lightmatrix::Scene::panel()
{
    return m_panel;
}

//------------------------------------------------------------------------------
const quetzal::lightmatrix::Frame& quetzal::lightmatrix::Scene::frame() const
{
    return m_frame;
}

//------------------------------------------------------------------------------
quetzal::lightmatrix::Frame& quetzal::lightmatrix::Scene::frame()
{
    return m_frame;
}

//------------------------------------------------------------------------------
const quetzal::lightmatrix::Scene::patterns_type& quetzal::lightmatrix::Scene::patterns() const
{
    return m_patterns;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Scene::set_pattern(Pattern&& pattern)
{
    m_patterns[0] = std::move(pattern);
    m_transition.set_transitioning(false);
    paint_frame(true, true);
    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Scene::set_transition_pattern(Pattern&& pattern)
{
    if (m_transition.type() == Transition::Cut)
    {
        set_pattern(std::move(pattern));
        return;
    }

    if (m_transition.transitioning())
    {
        m_patterns[0] = lerp(m_patterns[0], m_patterns[1], m_transition.progress()); // is that all? is that correct? ...
    }

    m_patterns[1] = std::move(pattern);
    m_transition.set_transitioning(true);
    paint_frame(true, true);
    return;
}

//------------------------------------------------------------------------------
const quetzal::lightmatrix::Viewport& quetzal::lightmatrix::Scene::viewport() const
{
    return m_viewport;
}

//------------------------------------------------------------------------------
quetzal::lightmatrix::Viewport& quetzal::lightmatrix::Scene::viewport()
{
    return m_viewport;
}

//------------------------------------------------------------------------------
const quetzal::lightmatrix::Animation& quetzal::lightmatrix::Scene::animation() const
{
    return m_animation;
}

//------------------------------------------------------------------------------
quetzal::lightmatrix::Animation& quetzal::lightmatrix::Scene::animation()
{
    return m_animation;
}

//------------------------------------------------------------------------------
const quetzal::lightmatrix::Transition& quetzal::lightmatrix::Scene::transition() const
{
    return m_transition;
}

//------------------------------------------------------------------------------
quetzal::lightmatrix::Transition& quetzal::lightmatrix::Scene::transition()
{
    return m_transition;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Scene::update(float dt)
{
    bool transition_ended = m_transition.update(dt);
    if (transition_ended)
    {
        m_patterns[0] = std::move(m_patterns[1]);
    }

    point_type position = m_viewport.position();
    position += m_animation.direction() * m_animation.velocity() * dt;
    position.set_x(position.x() - floor(position.x()));
    position.set_y(position.y() - floor(position.y()));
    m_viewport.set_position(position);

    paint_frame(m_animation.shift_color(), m_animation.shift_brightness());

    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Scene::paint_frame(bool bRgb, bool bAlpha)
{
    if (!bRgb && !bAlpha)
    {
        return;
    }

    point_type position = m_viewport.position();
    const auto [nx, ny] = m_frame.size();

    for (size_t j = 0; j < ny; ++j)
    {
        for (size_t i = 0; i < nx; ++i)
        {
            float u = position.x() + (static_cast<float>(i) + 0.5f) / static_cast<float>(nx);
            float v = position.y() + (static_cast<float>(j) + 0.5f) / static_cast<float>(ny);

            Color color;

            if (m_transition.transitioning())
            {
                Color color0 = sample_pattern(m_patterns[0], u, v, m_animation.smoothing());
                Color color1 = sample_pattern(m_patterns[1], u, v, m_animation.smoothing());
                color = lerp(color0, color1, m_transition.progress());
            }
            else
            {
                color = sample_pattern(m_patterns[0], u, v, m_animation.smoothing());
            }

            Pixel& pixel = m_frame.pixel(i, j);

            if (bRgb)
            {
                pixel.set_rgb(color);
            }

            if (bAlpha)
            {
                pixel.set_a(color);
            }
        }
    }

    return;
}
