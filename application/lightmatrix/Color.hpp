#if !defined(QUETZAL_LIGHTMATRIX_COLOR_HPP)
#define QUETZAL_LIGHTMATRIX_COLOR_HPP
//------------------------------------------------------------------------------
// Light Matrix Simulation
// Copyright (c) 2010 Keith Michael Looney. All Rights Reserved.
// Color.hpp
//------------------------------------------------------------------------------

#include <iosfwd>

namespace quetzal::lightmatrix
{

    //----------------------------------------------------------------------
    class Color
    {
    public:

        using component_type = float; // 0.0 .. 1.0

        Color();
        Color(component_type r, component_type g, component_type b, component_type a = 1.0f);
        Color(const Color&) = default;
        ~Color() = default;

        Color& operator=(const Color&) = default;

        component_type r() const;
        component_type g() const;
        component_type b() const;
        component_type a() const;

        void set_rgb(const Color& color); // either get rid of or add set_a counterpart ...

        void set_rgb(component_type r, component_type g, component_type b);
        void set_a(component_type a);

        // lerp? ...
        // should convert to HSV for circular hue ...
        // not really slerp, but conceptually similar in effect ...

    private:

        component_type m_r;
        component_type m_g;
        component_type m_b;
        component_type m_a;
    };

    std::ostream& operator<<(std::ostream& os, const quetzal::lightmatrix::Color& color);
    std::istream& operator>>(std::istream& is, quetzal::lightmatrix::Color& color);

} // namespace quetzal::lightmatrix

#endif // QUETZAL_LIGHTMATRIX_COLOR_HPP
