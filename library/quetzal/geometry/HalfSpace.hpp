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
#include "quetzal/common/string_util.hpp"
#include "quetzal/math/floating_point.hpp"
#include <format>
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
        HalfSpace(const separator_type& separator); // Uses plane normal or line perp for exterior_direction
        HalfSpace(const HalfSpace&) = default;
        HalfSpace(HalfSpace&&) = default;
        ~HalfSpace() = default;

        HalfSpace& operator=(const HalfSpace&) = default;
        HalfSpace& operator=(HalfSpace&&) = default;

        const separator_type& separator() const;
        const vector_type& exterior_direction() const;

        void set(const separator_type& separator, const vector_type& exterior_direction);

        // Returns less, equivalent, greater for interior, boundary, exterior respectively
        std::partial_ordering compare(const point_type& point, int tolerance = math::tolerance_default) const override;

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
quetzal::geometry::HalfSpace<Traits>::HalfSpace(const separator_type& separator) :
    PointRelation<Traits>(),
    m_separator(separator),
    m_exterior_direction([&](){if constexpr (Traits::dimension >= 3) {return separator.normal();} else {return separator.perp();};}())
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
void quetzal::geometry::HalfSpace<Traits>::set(const separator_type& separator, const vector_type& exterior_direction)
{
    m_separator = separator;
    m_exterior_direction = exterior_direction;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::partial_ordering quetzal::geometry::HalfSpace<Traits>::compare(const point_type& position, int tolerance) const
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
    return math::float_compare0(d, tolerance);
}

//------------------------------------------------------------------------------
template<typename Traits>
std::ostream& quetzal::geometry::operator<<(std::ostream& os, HalfSpace<Traits> halfspace)
{
    os << to_string(halfspace, os);
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits>
struct std::formatter<quetzal::geometry::HalfSpace<Traits>>
{
    // HalfSpace has no formatting knobs of its own; the whole spec belongs to (and is entirely
    // consumed by) the Point formatter. separator_type is a Plane, Line, or Point depending on
    // dimension -- all of which either hold or are a Point formatter with the identical fields.
    std::formatter<quetzal::geometry::Point<Traits>> m_point_formatter;

    constexpr auto parse(std::format_parse_context& ctx)
    {
        return m_point_formatter.parse(ctx);
    }

    template<typename FormatContext>
    auto format(const quetzal::geometry::HalfSpace<Traits>& halfspace, FormatContext& ctx) const
    {
        std::formatter<quetzal::math::Vector<Traits>> vector_formatter;
        vector_formatter.m_object_delimiters = m_point_formatter.m_object_delimiters;
        vector_formatter.m_element_delimiter = m_point_formatter.m_element_delimiter;
        vector_formatter.m_element_formatter = m_point_formatter.m_element_formatter;

        auto out = ctx.out();
        if constexpr (Traits::dimension >= 3)
        {
            std::formatter<quetzal::geometry::Plane<Traits>> separator_formatter;
            separator_formatter.m_point_formatter = m_point_formatter;
            out = separator_formatter.format(halfspace.separator(), ctx);
        }
        else if constexpr (Traits::dimension == 2)
        {
            std::formatter<quetzal::geometry::Line<Traits>> separator_formatter;
            separator_formatter.m_point_formatter = m_point_formatter;
            out = separator_formatter.format(halfspace.separator(), ctx);
        }
        else
        {
            out = m_point_formatter.format(halfspace.separator(), ctx);
        }

        ctx.advance_to(out);
        *out++ = ' ';
        ctx.advance_to(out);

        return vector_formatter.format(halfspace.exterior_direction(), ctx);
    }
};

#endif // QUETZAL_GEOMETRY_HALFSPACE_HPP
