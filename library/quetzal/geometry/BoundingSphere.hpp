#if !defined(QUETZAL_GEOMETRY_BOUNDINGSPHERE_HPP)
#define QUETZAL_GEOMETRY_BOUNDINGSPHERE_HPP
//------------------------------------------------------------------------------
// geometry
// BoundingSphere.hpp
//------------------------------------------------------------------------------

#include "BoundingVolume.hpp"
#include "quetzal/common/string_util.hpp"
#include "quetzal/math/floating_point.hpp"
#include <format>
#include <iostream>
#include <limits>

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    template<typename Traits>
    class BoundingSphere : public BoundingVolume<Traits>
    {
    public:

        using value_type = Traits::value_type;
        using BoundingVolume<Traits>::point_type;
        using vector_type = point_type::vector_type;

        BoundingSphere();
        BoundingSphere(const point_type& center, value_type radius);
        BoundingSphere(const BoundingSphere&) = default;
        BoundingSphere(BoundingSphere&&) = default;
        ~BoundingSphere() = default;

        BoundingSphere& operator=(const BoundingSphere&) = default;
        BoundingSphere& operator=(BoundingSphere&&) = default;

        point_type center() const;
        void set_center(const point_type& center);

        value_type radius() const;
        void set_radius(value_type radius);

        void set_bounds(const point_type& center, value_type radius);

        point_type centroid() const override;

        void clear() override;

        // Returns less, equivalent, greater for interior, boundary, exterior respectively
        std::partial_ordering compare(const point_type& point, int tolerance = math::tolerance_default) const override;

        bool empty() const override;

        void print(std::ostream& os) const override;

    private:

        point_type m_center;
        value_type m_radius;
    };

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::BoundingSphere<Traits>::BoundingSphere() :
    BoundingSphere({}, std::numeric_limits<value_type>::lowest())
{
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::BoundingSphere<Traits>::BoundingSphere(const point_type& center, value_type radius) :
    m_center(center),
    m_radius(radius)
{
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::BoundingSphere<Traits>::point_type quetzal::geometry::BoundingSphere<Traits>::center() const
{
    return m_center;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::BoundingSphere<Traits>::set_center(const point_type& center)
{
    m_center = center;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::BoundingSphere<Traits>::value_type quetzal::geometry::BoundingSphere<Traits>::radius() const
{
    return m_radius;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::BoundingSphere<Traits>::set_radius(value_type radius)
{
    m_radius = radius;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::BoundingSphere<Traits>::set_bounds(const point_type& center, value_type radius)
{
    m_center = center;
    m_radius = radius;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::BoundingSphere<Traits>::point_type quetzal::geometry::BoundingSphere<Traits>::centroid() const
{
    return m_center;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::BoundingSphere<Traits>::clear()
{
    m_center.clear();
    m_radius = std::numeric_limits<value_type>::lowest();
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::partial_ordering quetzal::geometry::BoundingSphere<Traits>::compare(const point_type& point, int tolerance) const
{
    if (m_radius >= value_type(0))
    {
        value_type d = (point - m_center).norm_squared();
        value_type r2 = m_radius * m_radius;

        if (math::float_eq(d, r2, tolerance))
        {
            return std::partial_ordering::equivalent;
        }
        else if (d < r2)
        {
            return std::partial_ordering::less;
        }
    }

    return std::partial_ordering::greater;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::BoundingSphere<Traits>::empty() const
{
    return m_radius < value_type(0);
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::BoundingSphere<Traits>::print(std::ostream& os) const
{
    os << to_array_string(*this, os);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
struct std::formatter<quetzal::geometry::BoundingSphere<Traits>>
{
    // BoundingSphere has no formatting knobs of its own; the whole spec belongs to (and is entirely
    // consumed by) the Point formatter, reused for the center, with its element formatter reused
    // directly for the bare scalar radius (no Vector-style wrapper needed for a single value).
    std::formatter<quetzal::geometry::Point<Traits>> m_point_formatter;

    constexpr auto parse(std::format_parse_context& ctx)
    {
        return m_point_formatter.parse(ctx);
    }

    auto format(const quetzal::geometry::BoundingSphere<Traits>& sphere, std::format_context& ctx) const
    {
        auto out = m_point_formatter.format(sphere.center(), ctx);
        ctx.advance_to(out);
        *out++ = ',';
        *out++ = ' ';
        ctx.advance_to(out);

        return m_point_formatter.m_element_formatter.format(sphere.radius(), ctx);
    }
};

#endif // QUETZAL_GEOMETRY_BOUNDINGSPHERE_HPP
