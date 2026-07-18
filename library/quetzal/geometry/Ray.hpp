#if !defined(QUETZAL_GEOMETRY_RAY_HPP)
#define QUETZAL_GEOMETRY_RAY_HPP
//------------------------------------------------------------------------------
// geometry
// Ray.hpp
//------------------------------------------------------------------------------

#include "PointRelation.hpp"
#include "Point.hpp"
#include "quetzal/common/string_util.hpp"
#include "quetzal/math/floating_point.hpp"
#include "quetzal/math/math_util.hpp"
#include <format>
#include <iostream>
#include <sstream>
#include <cassert>

namespace quetzal::geometry
{

    // Client responsibility to ensure that the direction vector is unit length

    //--------------------------------------------------------------------------
    template<typename Traits>
    class Ray : public PointRelation<Traits>
    {
    public:

        using traits_type = Traits;
        using value_type = Traits::value_type;
        using point_type = Point<Traits>;
        using vector_type = point_type::vector_type;

        Ray() = default;
        Ray(const point_type& point, const vector_type& direction);
        Ray(const Ray&) = default;
        Ray(Ray&&) = default;
        ~Ray() = default;

        Ray& operator=(const Ray&) = default;
        Ray& operator=(Ray&&) = default;

        const point_type& endpoint() const;
        point_type& endpoint();
        void set_endpoint(const point_type& point);

        vector_type direction() const;
        void set_direction(const vector_type& direction);

        point_type point(value_type t) const;

        // Returns less, equivalent, greater for interior, boundary, exterior respectively
        std::partial_ordering compare(const point_type& point, int tolerance = math::tolerance_default) const override;

        point_type projection(const point_type& point) const;
        value_type projection_parameter(const point_type& point) const;

    private:

        point_type m_endpoint;
        vector_type m_direction;
    };

    template<typename Traits>
    std::ostream& operator<<(std::ostream& os, Ray<Traits> ray);

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Ray<Traits>::Ray(const point_type& point, const vector_type& direction) :
    PointRelation<Traits>(),
    m_endpoint(point),
    m_direction(direction)
{
    assert(direction.unit());
}

//------------------------------------------------------------------------------
template<typename Traits>
const typename quetzal::geometry::Ray<Traits>::point_type& quetzal::geometry::Ray<Traits>::endpoint() const
{
    return m_endpoint;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Ray<Traits>::point_type& quetzal::geometry::Ray<Traits>::endpoint()
{
    return m_endpoint;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Ray<Traits>::set_endpoint(const point_type& point)
{
    m_endpoint = point;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Ray<Traits>::vector_type quetzal::geometry::Ray<Traits>::direction() const
{
    return m_direction;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Ray<Traits>::set_direction(const vector_type& direction)
{
    assert(direction.unit());

    m_direction = direction;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Ray<Traits>::point_type quetzal::geometry::Ray<Traits>::point(value_type t) const
{
    assert(math::float_ge0(t));
    return m_endpoint + math::clamp0(t) * m_direction;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::partial_ordering quetzal::geometry::Ray<Traits>::compare(const point_type& point, int tolerance) const
{
    if (float_eq(point, m_endpoint, tolerance))
    {
        return std::partial_ordering::equivalent;
    }

    value_type t = dot(point - m_endpoint, m_direction);
    return math::float_ge0(t, tolerance) && float_eq(point, this->point(t), tolerance) ? std::partial_ordering::less : std::partial_ordering::greater;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Ray<Traits>::point_type quetzal::geometry::Ray<Traits>::projection(const point_type& point) const
{
    return this->point(projection_parameter(point));
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Ray<Traits>::value_type quetzal::geometry::Ray<Traits>::projection_parameter(const point_type& point) const
{
    return math::dot(point - m_endpoint, m_direction);
}

//------------------------------------------------------------------------------
template<typename Traits>
std::ostream& quetzal::geometry::operator<<(std::ostream& os, Ray<Traits> ray)
{
    os << to_string(ray, os);
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits>
struct std::formatter<quetzal::geometry::Ray<Traits>>
{
    // Ray has no formatting knobs of its own; the whole spec belongs to (and is entirely
    // consumed by) the Point formatter, whose parsed state is then shared with a Vector
    // formatter for the direction (they carry the identical set of fields).
    std::formatter<quetzal::geometry::Point<Traits>> m_point_formatter;

    constexpr auto parse(std::format_parse_context& ctx)
    {
        return m_point_formatter.parse(ctx);
    }

    auto format(const quetzal::geometry::Ray<Traits>& ray, std::format_context& ctx) const
    {
        std::formatter<quetzal::math::Vector<Traits>> vector_formatter;
        vector_formatter.m_object_delimiters = m_point_formatter.m_object_delimiters;
        vector_formatter.m_element_delimiter = m_point_formatter.m_element_delimiter;
        vector_formatter.m_element_formatter = m_point_formatter.m_element_formatter;

        auto out = m_point_formatter.format(ray.endpoint(), ctx);
        ctx.advance_to(out);
        *out++ = ',';
        *out++ = ' ';
        ctx.advance_to(out);

        return vector_formatter.format(ray.direction(), ctx);
    }
};

#endif // QUETZAL_GEOMETRY_RAY_HPP
