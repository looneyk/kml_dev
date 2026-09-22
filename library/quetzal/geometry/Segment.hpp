#if !defined(QUETZAL_GEOMETRY_SEGMENT_HPP)
#define QUETZAL_GEOMETRY_SEGMENT_HPP
//------------------------------------------------------------------------------
// geometry
// Segment.hpp
//------------------------------------------------------------------------------

#include "PointRelation.hpp"
#include "Point.hpp"
#include "quetzal/common/string_util.hpp"
#include "quetzal/math/floating_point.hpp"
#include "quetzal/math/math_util.hpp"
#include <algorithm>
#include <format>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <cassert>

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    template<typename Traits>
    class Segment : public PointRelation<Traits>
    {
    public:

        using traits_type = Traits;
        using size_type = Traits::size_type;
        using value_type = Traits::value_type;
        using point_type = Point<Traits>;
        using vector_type = point_type::vector_type;
        using points_type = std::array<point_type, 2>;

        Segment();
        Segment(const points_type& points); // span? ...
        Segment(const point_type& a, const point_type& b);
        Segment(const Segment&) = default;
        Segment(Segment&&) = default;
        ~Segment() = default;

        Segment& operator=(const Segment&) = default;
        Segment& operator=(Segment&&) = default;

        const points_type& endpoints() const;
        const point_type& endpoint(size_type n) const;
        point_type& endpoint(size_type n);
        void set_endpoint(size_type n, const point_type& point);
        void set_endpoints(const point_type& a, const point_type& b);

        value_type length() const;
        vector_type vector() const; // directed segment support, vector connecting endpoints
        vector_type direction() const; // directed segment support, unit direction vector

        point_type point(value_type t) const;
        point_type midpoint() const;

        bool degenerate() const;
        bool float_degenerate() const;

        // Returns less, equivalent, greater for interior, boundary, exterior respectively
        std::partial_ordering compare(const point_type& point, int tolerance = math::tolerance_default) const override;

        point_type projection(const point_type& point) const;
        value_type projection_parameter(const point_type& point) const;

        void reverse(); // directed segment support

    private:

        points_type m_endpoints;

        // Cached direction vector calculated only when needed
        mutable std::optional<vector_type> m_direction;
    };

    // directed segment equality, ordering matters, geometrically it shouldn't ...
    template<typename Traits>
    bool operator==(const Segment<Traits>& lhs, const Segment<Traits>& rhs);

    // directed segment equality, ordering matters, geometrically it shouldn't ...
    template<typename Traits>
    bool float_eq(const Segment<Traits>& lhs, const Segment<Traits>& rhs);

    template<typename Traits>
    Segment<Traits> reverse(Segment<Traits> segment);

    template<typename Traits>
    std::ostream& operator<<(std::ostream& os, Segment<Traits> segment);

} // namespace quetzal::geometry

  //------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Segment<Traits>::Segment() :
    PointRelation<Traits>(),
    m_endpoints(),
    m_direction()
{
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Segment<Traits>::Segment(const points_type& points) :
    PointRelation<Traits>(),
    m_endpoints(points),
    m_direction()
{
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Segment<Traits>::Segment(const point_type& a, const point_type& b) :
    m_endpoints{a, b},
    m_direction()
{
}

//------------------------------------------------------------------------------
template<typename Traits>
const typename quetzal::geometry::Segment<Traits>::points_type& quetzal::geometry::Segment<Traits>::endpoints() const
{
    return m_endpoints;
}

//------------------------------------------------------------------------------
template<typename Traits>
const typename quetzal::geometry::Segment<Traits>::point_type& quetzal::geometry::Segment<Traits>::endpoint(size_type n) const
{
    assert(n <= 1);
    return m_endpoints[n];
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Segment<Traits>::point_type& quetzal::geometry::Segment<Traits>::endpoint(size_type n)
{
    assert(n <= 1);
    m_direction = std::nullopt;
    return m_endpoints[n];
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Segment<Traits>::set_endpoint(size_type n, const point_type& point)
{
    assert(n <= 1);
    m_direction = std::nullopt;
    m_endpoints[n] = point;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Segment<Traits>::set_endpoints(const point_type& a, const point_type& b)
{
    m_direction = std::nullopt;
    m_endpoints = {a, b};
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Segment<Traits>::value_type quetzal::geometry::Segment<Traits>::length() const
{
    return vector().norm();
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Segment<Traits>::vector_type quetzal::geometry::Segment<Traits>::vector() const
{
    return m_endpoints[1] - m_endpoints[0];
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Segment<Traits>::vector_type quetzal::geometry::Segment<Traits>::direction() const
{
    if (!m_direction)
    {
        m_direction = normalize(m_endpoints[1] - m_endpoints[0]);
    }
     
    return *m_direction;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Segment<Traits>::point_type quetzal::geometry::Segment<Traits>::point(value_type t) const
{
//    assert(math::float_clamped01(t));
    return lerp(m_endpoints[0], m_endpoints[1], t);
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Segment<Traits>::point_type quetzal::geometry::Segment<Traits>::midpoint() const
{
    return lerp(m_endpoints[0], m_endpoints[1], typename Traits::value_type(0.5));
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::Segment<Traits>::degenerate() const
{
    return m_endpoints[0] = m_endpoints[1];
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::Segment<Traits>::float_degenerate() const
{
    return float_eq(m_endpoints[0], m_endpoints[1]);
}

//------------------------------------------------------------------------------
template<typename Traits>
std::partial_ordering quetzal::geometry::Segment<Traits>::compare(const point_type& point, int tolerance) const
{
    if (float_eq(point, m_endpoints[0], tolerance) || float_eq(point, m_endpoints[1], tolerance))
    {
        return std::partial_ordering::equivalent;
    }

    value_type t = projection_parameter(point);
    return math::float_clamped01(t, tolerance) && float_eq(point, this->point(t), tolerance) ? std::partial_ordering::less : std::partial_ordering::greater;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Segment<Traits>::point_type quetzal::geometry::Segment<Traits>::projection(const point_type& point) const
{
    return this->point(projection_parameter(point));
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Segment<Traits>::value_type quetzal::geometry::Segment<Traits>::projection_parameter(const point_type& point) const
{
    return math::dot(point - m_endpoints[0], vector()) / vector().norm_squared();
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Segment<Traits>::reverse()
{
    m_direction = std::nullopt;
    std::swap(m_endpoints[0], m_endpoints[1]);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::operator==(const Segment<Traits>& lhs, const Segment<Traits>& rhs)
{
    return lhs.endpoints() == rhs.endpoints();
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::float_eq(const Segment<Traits>& lhs, const Segment<Traits>& rhs)
{
    return float_eq(lhs.endpoint(0), rhs.endpoint(0)) && float_eq(lhs.endpoint(1), rhs.endpoint(1));
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Segment<Traits> quetzal::geometry::reverse(Segment<Traits> segment)
{
    segment.reverse();
    return segment;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::ostream& quetzal::geometry::operator<<(std::ostream& os, Segment<Traits> segment)
{
    os << to_string(segment, os);
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits>
struct std::formatter<quetzal::geometry::Segment<Traits>>
{
    // Parses the whole format spec, and formats both endpoints
    std::formatter<quetzal::geometry::Point<Traits>> m_point_formatter;

    constexpr auto parse(std::format_parse_context& ctx)
    {
        return m_point_formatter.parse(ctx);
    }

    template<typename FormatContext>
    auto format(const quetzal::geometry::Segment<Traits>& segment, FormatContext& ctx) const
    {
        auto out = m_point_formatter.format(segment.endpoint(0), ctx);
        ctx.advance_to(out);
        *out++ = ',';
        *out++ = ' ';
        ctx.advance_to(out);

        return m_point_formatter.format(segment.endpoint(1), ctx);
    }
};

#endif // QUETZAL_GEOMETRY_SEGMENT_HPP
