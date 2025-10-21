#if !defined(QUETZAL_LIGHTMATRIX_PIXEL_HPP)
#define QUETZAL_LIGHTMATRIX_PIXEL_HPP
//------------------------------------------------------------------------------
// Light Matrix Simulation
// Copyright (c) 2010 Keith Michael Looney. All Rights Reserved.
// Pixel.hpp
//------------------------------------------------------------------------------

#include <iosfwd>
#include "Color.hpp"

namespace quetzal::lightmatrix
{

    //--------------------------------------------------------------------------
    class Pixel
    {
    public:

        using component_type = Color::component_type;

        Pixel();
        Pixel(const Pixel&) = default;
        ~Pixel() = default;

        Pixel& operator=(const Pixel&) = default;

        float radius() const;
        float x() const;
        float y() const;

        Color color() const;

        void set_radius(float r);
        void set_position(float x, float y);

        void set_color(Color color);
        void set_rgb(Color color);
        void set_a(Color color);

        void set_rgb(component_type r, component_type g, component_type b);
        void set_a(component_type a);

    private:

        float m_radius;

        float m_x;
        float m_y;

        Color m_color;
    };

    // These write and read only color values
    std::ostream& operator<<(std::ostream& os, const quetzal::lightmatrix::Pixel& pixel);
    std::istream& operator>>(std::istream& is, quetzal::lightmatrix::Pixel& pixel);

} // namespace quetzal::lightmatrix

#endif // QUETZAL_LIGHTMATRIX_PIXEL_HPP
