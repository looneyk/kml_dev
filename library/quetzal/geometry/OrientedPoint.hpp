#if !defined(QUETZAL_GEOMETRY_ORIENTEDPOINT_HPP)
#define QUETZAL_GEOMETRY_ORIENTEDPOINT_HPP
//------------------------------------------------------------------------------
// geometry
// OrientedPoint.hpp
//------------------------------------------------------------------------------

#include "Orientation.hpp"
#include "Point.hpp"
#include <iostream>

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    template<typename Traits>
    class OrientedPoint
    {
    public:

		static_assert(Traits::dimension >= 3);

        using value_type = Traits::value_type;
        using orientation_type = Orientation<Traits>;
        using vector_type = orientation_type::vector_type;
        using point_type = Point<Traits>;

        OrientedPoint() = default;
        OrientedPoint(const point_type& position, const orientation_type& orientation);
        OrientedPoint(const OrientedPoint&) = default;
        ~OrientedPoint() = default;

        OrientedPoint& operator=(const OrientedPoint&) = default;

        point_type position() const;
        point_type& position();
        void set_position(const point_type& position);

        orientation_type orientation() const;
        orientation_type& orientation();
        void set_orientation(const orientation_type& orientation);

    private:

        point_type m_position;
        orientation_type m_orientation;
    };

    template<typename Traits>
    std::ostream& operator<<(std::ostream& os, OrientedPoint<Traits> plane);

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::OrientedPoint<Traits>::OrientedPoint(const point_type& position, const orientation_type& orientation) :
    m_position(position),
    m_orientation(orientation)
{
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::OrientedPoint<Traits>::point_type quetzal::geometry::OrientedPoint<Traits>::position() const
{
    return m_position;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::OrientedPoint<Traits>::point_type& quetzal::geometry::OrientedPoint<Traits>::position()
{
    return m_position;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::OrientedPoint<Traits>::set_position(const point_type& position)
{
    m_position = position;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::OrientedPoint<Traits>::orientation_type quetzal::geometry::OrientedPoint<Traits>::orientation() const
{
    return m_orientation;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::OrientedPoint<Traits>::orientation_type& quetzal::geometry::OrientedPoint<Traits>::orientation()
{
    return m_orientation;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::OrientedPoint<Traits>::set_orientation(const orientation_type& orientation)
{
    m_orientation = orientation;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::ostream& quetzal::geometry::operator<<(std::ostream& os, OrientedPoint<Traits> op)
{
    os << to_string(op.position()) << ", " << to_string(op.orientation());
    return os;
}

#endif // QUETZAL_GEOMETRY_ORIENTEDPOINT_HPP
