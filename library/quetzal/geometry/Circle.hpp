#if !defined(QUETZAL_GEOMETRY_CIRCLE_HPP)
#define QUETZAL_GEOMETRY_CIRCLE_HPP
//------------------------------------------------------------------------------
// geometry
// Circle.hpp
//------------------------------------------------------------------------------

#include "PointRelation.hpp"
#include "Point.hpp"
#include "quetzal/math/floating_point.hpp"
#include <iostream>

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    template<typename Traits>
    class Circle : public PointRelation<Traits>
    {
    public:

        using value_type = Traits::value_type;
        using point_type = Point<Traits>;

        Circle() = default;
        Circle(const point_type& center, const value_type& radius);
        Circle(const Circle&) = default;
        Circle(Circle&&) = default;
        ~Circle() = default;

        Circle& operator=(const Circle&) = default;
        Circle& operator=(Circle&&) = default;

        point_type center() const;
        void set_center(const point_type& center);

        value_type radius() const;
        void set_radius(const value_type& radius);

        point_type point(value_type phi) const;

        // Returns -1, 0, 1: interior, boundary, exterior
        virtual int compare(const point_type& point) const = 0;

    private:

        point_type m_center;
        value_type m_radius;
    };

    template<typename Traits>
    std::ostream& operator<<(std::ostream& os, const Circle<Traits>& circle);

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Circle<Traits>::Circle(const point_type& center, const value_type& radius) :
    PointRelation<Traits>(),
    m_center(center),
    m_radius(radius)
{
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Circle<Traits>::point_type quetzal::geometry::Circle<Traits>::center() const
{
    return m_center;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Circle<Traits>::set_center(const point_type& center)
{
    m_center = center;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Circle<Traits>::value_type quetzal::geometry::Circle<Traits>::radius() const
{
    return m_radius;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Circle<Traits>::set_radius(const value_type& radius)
{
    m_radius = radius;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Circle<Traits>::point_type quetzal::geometry::Circle<Traits>::point(value_type phi) const
{
    if constexpr (Traits::dimension == 3)
    {
        return {m_radius * cos(phi), m_radius * sin(phi), value_type(0)};
    }
    else if constexpr (Traits::dimension == 2)
    {
        return {m_radius * cos(phi), m_radius * sin(phi)};
    }
    else
    {
        static_assert(false, "Dimension not supported.");
    }
}

//------------------------------------------------------------------------------
template<typename Traits>
int quetzal::geometry::Circle<Traits>::compare(const point_type& point)
{
    value_type d2 = (point - m_center).norm_squared();
    value_type r2 = m_radius * m_radius;
    return float_compare(d2, r2);
}

//------------------------------------------------------------------------------
template<typename Traits>
std::ostream& quetzal::geometry::operator<<(std::ostream& os, const Circle<Traits>& circle)
{
    os << to_string(circle.center()) << ", " << circle.radius();
    return os;
}

#endif // QUETZAL_GEOMETRY_CIRCLE_HPP
