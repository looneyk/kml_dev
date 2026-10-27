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
#include <charconv>
#include <format>
#include <istream>
#include <ostream>
#include <string_view>
#include <system_error>
#include <cassert>

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    template<typename Traits>
    class HalfSpace : public PointRelation<Traits>
    {
    public:

        using traits_type = Traits;
        using value_type = Traits::value_type;
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

    template<typename Traits>
    std::istream& operator>>(std::istream& is, HalfSpace<Traits>& halfspace);

    template<typename Traits>
    std::from_chars_result from_chars(const char* first, const char* last, HalfSpace<Traits>& halfspace, const FormatSpec& spec = {});

    template<typename Traits>
    std::to_chars_result to_chars(char* first, char* last, const HalfSpace<Traits>& halfspace, const FormatSpec& spec = {});

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
    os << quetzal::to_string(halfspace, os);
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::istream& quetzal::geometry::operator>>(std::istream& is, HalfSpace<Traits>& halfspace)
{
    typename HalfSpace<Traits>::separator_type separator;
    typename HalfSpace<Traits>::vector_type exterior_direction;
    if (quetzal::read_components(is,separator, exterior_direction))
    {
        halfspace = HalfSpace<Traits>(separator, exterior_direction);
    }

    return is;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::from_chars_result quetzal::geometry::from_chars(const char* first, const char* last, HalfSpace<Traits>& halfspace, const FormatSpec& spec)
{
    typename HalfSpace<Traits>::separator_type halfspace_separator;
    typename HalfSpace<Traits>::vector_type exterior_direction;
    auto result = quetzal::components_from_chars(first, last, spec,halfspace_separator, exterior_direction);
    if (result.ec == std::errc {})
    {
        halfspace = HalfSpace<Traits>(halfspace_separator, exterior_direction);
    }

    return result;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::to_chars_result quetzal::geometry::to_chars(char* first, char* last, const HalfSpace<Traits>& halfspace, const FormatSpec& spec)
{
    return quetzal::components_to_chars(first, last, spec,halfspace.separator(), halfspace.exterior_direction());
}

//------------------------------------------------------------------------------
template<typename Traits>
struct std::formatter<quetzal::geometry::HalfSpace<Traits>> : quetzal::series_formatter<quetzal::geometry::Point<Traits>>
{
    template<typename FormatContext>
    auto format(const quetzal::geometry::HalfSpace<Traits>& halfspace, FormatContext& ctx) const
    {
        if constexpr (Traits::dimension >= 2)
        {
            std::formatter<typename quetzal::geometry::HalfSpace<Traits>::separator_type> separator_formatter;
            separator_formatter.m_element_formatter = this->m_element_formatter;
            separator_formatter.set_format_spec(this->m_format_spec);
            ctx.advance_to(separator_formatter.format(halfspace.separator(), ctx));
        }
        else
        {
            ctx.advance_to(this->m_element_formatter.format(halfspace.separator(), ctx));
        }

        this->format_series_delimiter(ctx);
        return quetzal::matching_formatter<quetzal::math::Vector<Traits>>(this->m_element_formatter).format(halfspace.exterior_direction(), ctx);
    }
};

#endif // QUETZAL_GEOMETRY_HALFSPACE_HPP
