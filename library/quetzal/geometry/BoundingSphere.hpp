#if !defined(QUETZAL_GEOMETRY_BOUNDINGSPHERE_HPP)
#define QUETZAL_GEOMETRY_BOUNDINGSPHERE_HPP
//------------------------------------------------------------------------------
// geometry
// BoundingSphere.hpp
//------------------------------------------------------------------------------

#include "BoundingVolume.hpp"
#include "quetzal/common/string_util.hpp"
#include "quetzal/math/floating_point.hpp"
#include "quetzal/math/math_util.hpp"
#include <charconv>
#include <format>
#include <iostream>
#include <limits>
#include <string_view>
#include <system_error>

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
        value_type volume() const override;
        bool empty() const override;

        void clear() override;

        // Returns less, equivalent, greater for interior, boundary, exterior respectively
        std::partial_ordering compare(const point_type& point, int tolerance = math::tolerance_default) const override;

        void print(std::ostream& os) const override;

        point_type support(const vector_type& direction) const override;

    private:

        point_type m_center;
        value_type m_radius;
    };

    template<typename Traits>
    std::istream& operator>>(std::istream& is, BoundingSphere<Traits>& sphere);

    template<typename Traits>
    std::from_chars_result from_chars(const char* first, const char* last, BoundingSphere<Traits>& sphere, const FormatSpec& spec = {});

    template<typename Traits>
    std::to_chars_result to_chars(char* first, char* last, const BoundingSphere<Traits>& sphere, const FormatSpec& spec = {});

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
typename quetzal::geometry::BoundingSphere<Traits>::value_type quetzal::geometry::BoundingSphere<Traits>::volume() const
{
    if constexpr (Traits::dimension == 1)
    {
        return value_type(2) * m_radius;
    }
    else if constexpr (Traits::dimension == 2)
    {
        return math::Pi<value_type> * m_radius * m_radius;
    }
    else if constexpr (Traits::dimension == 3)
    {
        return value_type(4) / value_type(3) * math::Pi<value_type> * m_radius * m_radius * m_radius;
    }
    else
    {
        static_assert(false, "Dimension not supported.");
    }
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::BoundingSphere<Traits>::empty() const
{
    return m_radius < value_type(0);
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
void quetzal::geometry::BoundingSphere<Traits>::print(std::ostream& os) const
{
    os << quetzal::to_string(*this, os);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::istream& quetzal::geometry::operator>>(std::istream& is, BoundingSphere<Traits>& sphere)
{
    typename BoundingSphere<Traits>::point_type center;
    typename BoundingSphere<Traits>::value_type radius{};
    if (quetzal::read_components(is,center, radius))
    {
        sphere.set_bounds(center, radius);
    }

    return is;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::from_chars_result quetzal::geometry::from_chars(const char* first, const char* last, BoundingSphere<Traits>& sphere, const FormatSpec& spec)
{
    typename BoundingSphere<Traits>::point_type center;
    typename BoundingSphere<Traits>::value_type radius{};
    auto result = quetzal::components_from_chars(first, last, spec,center, radius);
    if (result.ec == std::errc{})
    {
        sphere.set_bounds(center, radius);
    }

    return result;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::to_chars_result quetzal::geometry::to_chars(char* first, char* last, const BoundingSphere<Traits>& sphere, const FormatSpec& spec)
{
    return quetzal::components_to_chars(first, last, spec,sphere.center(), sphere.radius());
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::BoundingSphere<Traits>::point_type quetzal::geometry::BoundingSphere<Traits>::support(const vector_type& direction) const
{
    vector_type unit_direction = direction.zero() ? vector_type::x_direction() : math::normalize(direction);
    return m_center + m_radius * unit_direction;
}

//------------------------------------------------------------------------------
template<typename Traits>
struct std::formatter<quetzal::geometry::BoundingSphere<Traits>> : quetzal::series_formatter<quetzal::geometry::Point<Traits>>
{
    template<typename FormatContext>
    auto format(const quetzal::geometry::BoundingSphere<Traits>& sphere, FormatContext& ctx) const
    {
        ctx.advance_to(this->m_element_formatter.format(sphere.center(), ctx));
        this->format_series_delimiter(ctx);
        return this->m_element_formatter.m_element_formatter.format(sphere.radius(), ctx);
    }
};

#endif // QUETZAL_GEOMETRY_BOUNDINGSPHERE_HPP
