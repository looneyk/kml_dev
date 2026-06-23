//------------------------------------------------------------------------------
// Light Matrix Simulation
// Panel.cpp
//------------------------------------------------------------------------------

#include "Frame.hpp"
#include "Panel.hpp"
#include <cmath>
#include <numbers>

//------------------------------------------------------------------------------
quetzal::lightmatrix::Panel::Panel(Arrangement arrangement, const vector_type& size, float pixel_radius) :
    m_arrangement(arrangement),
    m_size(size),
    m_pixel_radius(pixel_radius)
{
}

//------------------------------------------------------------------------------
quetzal::lightmatrix::Arrangement quetzal::lightmatrix::Panel::arrangement() const
{
    return m_arrangement;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Panel::set_arrangement(Arrangement arrangement)
{
    m_arrangement = arrangement;
    return;
}

//------------------------------------------------------------------------------
quetzal::lightmatrix::vector_type quetzal::lightmatrix::Panel::size() const
{
    return m_size;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Panel::set_size(const vector_type& size)
{
    m_size = size;
    return;
}

//------------------------------------------------------------------------------
float quetzal::lightmatrix::Panel::pixel_radius() const
{
    return m_pixel_radius;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Panel::set_pixel_radius(float radius)
{
    m_pixel_radius = radius;
    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Panel::lay_out(Frame& frame) const
{
    const auto [nx, ny] = frame.size();
    if (nx == 0 || ny == 0)
    {
        return;
    }

    if (m_arrangement == Arrangement::Radial)
    {
        float outer_radius = m_size.x() / 2.0f;
        float ring_spacing = outer_radius / static_cast<float>(ny);
        float dphi = 2.0f * std::numbers::pi_v<float> / static_cast<float>(nx);

        for (size_t j = 0; j < ny; ++j)
        {
            float radius = ring_spacing * (static_cast<float>(j) + 1.0f);

            for (size_t i = 0; i < nx; ++i)
            {
                float phi = -std::numbers::pi_v<float> / 2.0f + dphi * static_cast<float>(i);

                Pixel& pixel = frame.pixel(i, j);
                pixel.set_position({radius * std::cos(phi), radius * std::sin(phi)});
                pixel.set_radius(m_pixel_radius);
            }
        }
    }
    else
    {
        float spacing_x = m_size.x() / static_cast<float>(nx);
        float spacing_y = m_size.y() / static_cast<float>(ny);

        for (size_t j = 0; j < ny; ++j)
        {
            float y = spacing_y * (static_cast<float>(j) + 0.5f - static_cast<float>(ny) / 2.0f);

            for (size_t i = 0; i < nx; ++i)
            {
                float x = spacing_x * (static_cast<float>(i) + 0.5f - static_cast<float>(nx) / 2.0f);

                Pixel& pixel = frame.pixel(i, j);
                pixel.set_position({x, y});
                pixel.set_radius(m_pixel_radius);
            }
        }
    }

    return;
}
