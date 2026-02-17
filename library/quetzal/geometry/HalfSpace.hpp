#if !defined(QUETZAL_GEOMETRY_HALFSPACE_HPP)
#define QUETZAL_GEOMETRY_HALFSPACE_HPP
//------------------------------------------------------------------------------
// geometry
// HalfSpace.hpp
//------------------------------------------------------------------------------

#include "Line.hpp"
#include "Plane.hpp"
#include "Point.hpp"
#include "PointRelation.hpp"
#include "quetzal/math/floating_point.hpp"
#include <ostream>
#include <cassert>

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    template<typename Traits>
    class HalfSpace : public PointRelation<Traits>
    {
    public:

        using traits_type = Traits;
        using point_type = Point<Traits>;
        using vector_type = point_type::vector_type;
        using separator_type = std::conditional<Traits::dimension >= 3, Plane<Traits>, std::conditional<Traits::dimension == 2, Line<Traits>, point_type>>::type;

        HalfSpace() = default;
        HalfSpace(const separator_type& separator, const vector_type& exterior_direction);
        HalfSpace(const HalfSpace&) = default;
        HalfSpace(HalfSpace&&) = default;
        ~HalfSpace() = default;

        HalfSpace& operator=(const HalfSpace&) = default;
        HalfSpace& operator=(HalfSpace&&) = default;

        const separator_type& separator() const;
        const vector_type& exterior_direction() const;

        // Returns less, equivalent, greater for interior, boundary, exterior respectively
        std::partial_ordering compare(const point_type& point) const override;

    private:

        separator_type m_separator;
        vector_type m_exterior_direction;
    };

    template<typename Traits>
    std::ostream& operator<<(std::ostream& os, HalfSpace<Traits> halfspace);

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::HalfSpace<Traits>::HalfSpace(const separator_type& separator, const vector_type& exterior_direction) :
    PointRelation<Traits>(),
    m_separator(separator),
    m_exterior_direction(exterior_direction)
{
}

//------------------------------------------------------------------------------
template<typename Traits>
const quetzal::geometry::HalfSpace<Traits>::separator_type& quetzal::geometry::HalfSpace<Traits>::separator() const
{
    return m_separator;
}

//------------------------------------------------------------------------------
template<typename Traits>
const quetzal::geometry::HalfSpace<Traits>::vector_type& quetzal::geometry::HalfSpace<Traits>::exterior_direction() const
{
    return m_exterior_direction;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::partial_ordering quetzal::geometry::HalfSpace<Traits>::compare(const point_type& position) const
{
    point_type reference_position;
    if constexpr(Traits::dimension >= 2)
    {
        reference_position = m_separator.point();
    }
    else
    {
        reference_position = m_separator.position();
    }

    auto d = dot(m_exterior_direction, (position - reference_position));
    return math::float_compare0(d);
}

//------------------------------------------------------------------------------
template<typename Traits>
std::ostream& quetzal::geometry::operator<<(std::ostream& os, HalfSpace<Traits> halfspace)
{
    os << halfspace.separator() << ' ' << halfspace.exterior_direction();
    return os;
}

#endif // QUETZAL_GEOMETRY_HALFSPACE_HPP
