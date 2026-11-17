#if !defined(QUETZAL_GEOMETRY_BOX_HPP)
#define QUETZAL_GEOMETRY_BOX_HPP
//------------------------------------------------------------------------------
// geometry
// Box.hpp
//------------------------------------------------------------------------------

#include "Point.hpp"
#include "PointRelation.hpp"
#include "quetzal/common/string_util.hpp"
#include "quetzal/math/floating_point.hpp"
#include <array>
#include <charconv>
#include <format>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    template<typename Traits>
    class Box : public PointRelation<Traits>
    {
    public:

        using value_type = Traits::value_type;
        using PointRelation<Traits>::point_type;

        Box() = default;
        Box(const point_type& lower, const point_type& upper);
        Box(const Box&) = default;
        Box(Box&&) = default;
        virtual ~Box() = default;

        Box& operator=(const Box&) = default;
        Box& operator=(Box&&) = default;

        const point_type lower() const;
        void set_lower(const point_type& point);

        const point_type upper() const;
        void set_upper(const point_type& point);

        void set_bounds(const point_type& lower, const point_type& upper);

        // Returns less, equivalent, greater for interior, boundary, exterior respectively
        std::partial_ordering compare(const point_type& point, int tolerance = math::tolerance_default) const override;

    private:

        bool ordered() const;
        void order();

        point_type m_lower;
        point_type m_upper;
    };

    template<typename Traits>
    std::ostream& operator<<(std::ostream& os, const Box<Traits>& box);

    template<typename Traits>
    std::istream& operator>>(std::istream& is, Box<Traits>& box);

    template<typename Traits>
    std::from_chars_result from_chars(const char* first, const char* last, Box<Traits>& box, const FormatSpec& spec = {});

    template<typename Traits>
    std::to_chars_result to_chars(char* first, char* last, const Box<Traits>& box, const FormatSpec& spec = {});

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Box<Traits>::Box(const point_type& lower, const point_type& upper) :
    m_lower(lower),
    m_upper(upper)
{
}

//------------------------------------------------------------------------------
template<typename Traits>
const typename quetzal::geometry::Box<Traits>::point_type quetzal::geometry::Box<Traits>::lower() const
{
    return m_lower;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Box<Traits>::set_lower(const point_type& point)
{
    m_lower = point;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
const typename quetzal::geometry::Box<Traits>::point_type quetzal::geometry::Box<Traits>::upper() const
{
    return m_upper;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Box<Traits>::set_upper(const point_type& point)
{
    m_upper = point;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Box<Traits>::set_bounds(const point_type& lower, const point_type& upper)
{
    m_lower = lower;
    m_upper = upper;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::partial_ordering quetzal::geometry::Box<Traits>::compare(const point_type& point, int tolerance) const
{
// expose so it can be handled by caller, or just leave it to caller and assert ...
//    order();

    for (size_t i = 0; i < Traits::dimension; ++i)
    {
        if constexpr (std::is_floating_point_v<Traits::value_type>)
        {
            if (math::float_lt(point[i], m_lower[i], tolerance) || math::float_gt(point[i], m_upper[i], tolerance))
            {
                return std::partial_ordering::greater;
            }
        }
        else
        {
            if (point[i] < m_lower[i] || point[i] > m_upper[i])
            {
                return std::partial_ordering::greater;
            }
        }
    }

    for (size_t i = 0; i < Traits::dimension; ++i)
    {
        if constexpr (std::is_floating_point_v<Traits::value_type>)
        {
            if (math::float_eq(point[i], m_lower[i], tolerance) || math::float_eq(point[i], m_upper[i], tolerance))
            {
                return std::partial_ordering::equivalent;
            }
        }
        else
        {
            if (point[i] == m_lower[i] || point[i] == m_upper[i])
            {
                return std::partial_ordering::equivalent;
            }
        }
    }

    return std::partial_ordering::less;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::Box<Traits>::ordered() const
{
    for (size_t i = 0; i < Traits::dimension; ++i)
    {
        if (m_lower[i] > m_upper[i])
        {
            return false;
        }
    }

    return true;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Box<Traits>::order()
{
    for (size_t i = 0; i < Traits::dimension; ++i)
    {
        if (m_lower[i] > m_upper[i])
        {
            std::swap(m_lower[i], m_upper[i]);
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::ostream& quetzal::geometry::operator<<(std::ostream& os, const Box<Traits>& box)
{
    os << quetzal::to_string(box, os);
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::istream& quetzal::geometry::operator>>(std::istream& is, Box<Traits>& box)
{
    std::array<Point<Traits>, 2> points;
    if (quetzal::read_series(is, points, quetzal::bounds_delimiter_default))
    {
        box.set_bounds(points[0], points[1]);
    }

    return is;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::from_chars_result quetzal::geometry::from_chars(const char* first, const char* last, Box<Traits>& box, const FormatSpec& spec)
{
    std::array<Point<Traits>, 2> points;
    auto result = quetzal::series_from_chars(first, last, points, spec, quetzal::bounds_delimiter_default);
    if (result.ec == std::errc{})
    {
        box.set_bounds(points[0], points[1]);
    }

    return result;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::to_chars_result quetzal::geometry::to_chars(char* first, char* last, const Box<Traits>& box, const FormatSpec& spec)
{
    const std::array<Point<Traits>, 2> points{box.lower(), box.upper()};
    return quetzal::series_to_chars(first, last, points, spec, quetzal::bounds_delimiter_default);
}

//------------------------------------------------------------------------------
template<typename Traits>
struct std::formatter<quetzal::geometry::Box<Traits>> : quetzal::series_formatter<quetzal::geometry::Point<Traits>>
{
    constexpr formatter() :
        quetzal::series_formatter<quetzal::geometry::Point<Traits>>({.series_delimiter = std::string(quetzal::bounds_delimiter_default)})
    {
    }

    template<typename FormatContext>
    auto format(const quetzal::geometry::Box<Traits>& box, FormatContext& ctx) const
    {
        const std::array<quetzal::geometry::Point<Traits>, 2> points{box.lower(), box.upper()};
        return this->format_series(points, ctx);
    }
};

#endif // QUETZAL_GEOMETRY_BOX_HPP
