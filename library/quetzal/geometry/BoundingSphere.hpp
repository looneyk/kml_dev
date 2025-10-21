#if !defined(QUETZAL_GEOMETRY_BOUNDINGSPHERE_HPP)
#define QUETZAL_GEOMETRY_BOUNDINGSPHERE_HPP
//------------------------------------------------------------------------------
// geometry
// BoundingSphere.hpp
//------------------------------------------------------------------------------

#include "BoundingVolume.hpp"
#include "quetzal/math/floating_point.hpp"
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

        void clear() override;

        // Returns -1, 0, 1: interior, boundary, exterior
        int compare(const point_type& point) const override;

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
void quetzal::geometry::BoundingSphere<Traits>::clear()
{
    m_center.clear();
    m_radius = std::numeric_limits<value_type>::lowest();
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
int quetzal::geometry::BoundingSphere<Traits>::compare(const point_type& point) const
{
    if (m_radius >= value_type(0))
    {
        value_type d = (point - m_center).norm_squared();
        value_type r2 = m_radius * m_radius;

        if (math::float_eq(d, r2))
        {
            return 0;
        }
        else if (d < r2)
        {
            return -1;
        }
    }

    return 1;
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
    os << to_string(m_center) << ", " << m_radius;
    return;
}

#endif // QUETZAL_GEOMETRY_BOUNDINGSPHERE_HPP
