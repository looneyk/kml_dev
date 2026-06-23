#if !defined(QUETZAL_LIGHTMATRIX_PIXEL_HPP)
#define QUETZAL_LIGHTMATRIX_PIXEL_HPP
//------------------------------------------------------------------------------
// Light Matrix Simulation
// Pixel.hpp
//------------------------------------------------------------------------------

#include "Color.hpp"
#include "types.hpp"
#include <iosfwd>

namespace quetzal::lightmatrix
{

    //--------------------------------------------------------------------------
    class Pixel
    {
    public:

        using component_type = Color::component_type;

        Pixel();
        Pixel(const Pixel&) = default;
        Pixel(Pixel&&) = default;
        ~Pixel() = default;

        Pixel& operator=(const Pixel&) = default;
        Pixel& operator=(Pixel&&) = default;

        point_type position() const;
        void set_position(point_type position);

        float radius() const;
        void set_radius(float r);

        Color color() const;
        void set_color(Color color);
        void set_rgb(Color color);
        void set_a(Color color);

        void set_rgb(component_type r, component_type g, component_type b);
        void set_a(component_type a);

    private:

        point_type m_position;
        float m_radius;
        Color m_color;
    };

    // These operate on only color values

    Pixel lerp(const Pixel& pixel0, const Pixel& pixel1, float t);

    std::ostream& operator<<(std::ostream& os, const quetzal::lightmatrix::Pixel& pixel);
    std::istream& operator>>(std::istream& is, quetzal::lightmatrix::Pixel& pixel);

} // namespace quetzal::lightmatrix

#endif // QUETZAL_LIGHTMATRIX_PIXEL_HPP
