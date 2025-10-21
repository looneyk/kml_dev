#if !defined(QUETZAL_GEOMETRY_AXISALIGNEDBOUNDINGBOX_HPP)
#define QUETZAL_GEOMETRY_AXISALIGNEDBOUNDINGBOX_HPP
//------------------------------------------------------------------------------
// geometry
// AxisAlignedBoundingBox.hpp
//------------------------------------------------------------------------------

#include "BoundingVolume.hpp"
#include "quetzal/math/floating_point.hpp"
#include <iostream>
#include <cassert>

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    template<typename Traits>
    class AxisAlignedBoundingBox : public BoundingVolume<Traits>
    {
    public:

        using value_type = Traits::value_type;
        using BoundingVolume<Traits>::point_type;
        using vector_type = point_type::vector_type;

        AxisAlignedBoundingBox();
        AxisAlignedBoundingBox(const point_type& lower, const point_type& upper);
        AxisAlignedBoundingBox(std::span<const point_type> points);
        AxisAlignedBoundingBox(const AxisAlignedBoundingBox&) = default;
        AxisAlignedBoundingBox(AxisAlignedBoundingBox&&) = default;
        ~AxisAlignedBoundingBox() = default;

        AxisAlignedBoundingBox& operator=(const AxisAlignedBoundingBox&) = default;
        AxisAlignedBoundingBox& operator=(AxisAlignedBoundingBox&&) = default;

        // operators |= | &= & call op_union, op_intersection ...
        // if at the BoundingVolume level, can be specialized for mixed types ...
        // insert is |= (union) with point, points, or bounds ...

        const point_type& lower() const;
        point_type& lower(); // allows for efficient component access
        void set_lower(const point_type& point);

        const point_type& upper() const;
        point_type& upper(); // allows for efficient component access
        void set_upper(const point_type& point);

        void set_bounds(const point_type& lower, const point_type& upper);

        vector_type extent() const; // This is the full first octant/quadrant/... vector extent, not relative from the center as is done in direct3d11, and not necessarily semantically equivalent to other usages ...
        value_type extent(size_t n) const; // The n'th component of the extent (? ...)

        void insert(const point_type& point);
        void insert(std::span<const point_type> points);
        void insert(const AxisAlignedBoundingBox& bounds);

        void clear() override;

        // Returns less, equivalent, greater for interior, boundary, exterior respectively
        std::partial_ordering compare(const point_type& point) const override;

        bool empty() const override;

        void print(std::ostream& os) const override;

    private:

        bool ordered() const;
        void order();

        point_type m_lower;
        point_type m_upper;
    };

    template<typename Traits>
    bool operator==(const AxisAlignedBoundingBox<Traits>& lhs, const AxisAlignedBoundingBox<Traits>& rhs);

    // The bounding boxes do not intersect
    template<typename Traits>
    bool disjoint(const AxisAlignedBoundingBox<Traits>& a, const AxisAlignedBoundingBox<Traits>& b);

    // The minimum bounding box that contains both
    template<typename Traits>
    AxisAlignedBoundingBox<Traits> op_union(const AxisAlignedBoundingBox<Traits>& a, const AxisAlignedBoundingBox<Traits>& b);

    template<typename Traits>
    AxisAlignedBoundingBox<Traits> op_intersection(const AxisAlignedBoundingBox<Traits>& a, const AxisAlignedBoundingBox<Traits>& b);

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::AxisAlignedBoundingBox<Traits>::AxisAlignedBoundingBox() :
    AxisAlignedBoundingBox(point_type::highest(), point_type::lowest())
{
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::AxisAlignedBoundingBox<Traits>::AxisAlignedBoundingBox(const point_type& lower, const point_type& upper) :
    m_lower(lower),
    m_upper(upper)
{
    order();
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::AxisAlignedBoundingBox<Traits>::AxisAlignedBoundingBox(std::span<const point_type> points) :
    AxisAlignedBoundingBox()
{
    insert(points);
}

//------------------------------------------------------------------------------
template<typename Traits>
const typename quetzal::geometry::AxisAlignedBoundingBox<Traits>::point_type& quetzal::geometry::AxisAlignedBoundingBox<Traits>::lower() const
{
    return m_lower;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::AxisAlignedBoundingBox<Traits>::point_type& quetzal::geometry::AxisAlignedBoundingBox<Traits>::lower()
{
    return m_lower;
}

//------------------------------------------------------------------------------
template<typename Traits>
const typename quetzal::geometry::AxisAlignedBoundingBox<Traits>::point_type& quetzal::geometry::AxisAlignedBoundingBox<Traits>::upper() const
{
    return m_upper;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::AxisAlignedBoundingBox<Traits>::point_type& quetzal::geometry::AxisAlignedBoundingBox<Traits>::upper()
{
    return m_upper;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::AxisAlignedBoundingBox<Traits>::set_lower(const point_type& point)
{
    m_lower = point;
    order();
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::AxisAlignedBoundingBox<Traits>::set_upper(const point_type& point)
{
    m_upper = point;
    order();
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::AxisAlignedBoundingBox<Traits>::set_bounds(const point_type& lower, const point_type& upper)
{
    m_lower = lower;
    m_upper = upper;
    order();
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::AxisAlignedBoundingBox<Traits>::vector_type quetzal::geometry::AxisAlignedBoundingBox<Traits>::extent() const
{
    return m_upper - m_lower;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::AxisAlignedBoundingBox<Traits>::value_type quetzal::geometry::AxisAlignedBoundingBox<Traits>::extent(size_t n) const
{
    assert(n < Traits::dimension);
    return m_upper[n] - m_lower[n];
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::AxisAlignedBoundingBox<Traits>::insert(const point_type& point)
{
    assert(ordered());
    m_lower = min(m_lower, point);
    m_upper = max(m_upper, point);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::AxisAlignedBoundingBox<Traits>::insert(std::span<const point_type> points)
{
    assert(ordered());
    for (const auto& point : points)
    {
        insert(point);
    }
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::AxisAlignedBoundingBox<Traits>::insert(const AxisAlignedBoundingBox& bounds)
{
    assert(ordered());
    m_lower = min(m_lower, bounds.lower());
    m_upper = max(m_upper, bounds.upper());
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::AxisAlignedBoundingBox<Traits>::clear()
{
    m_lower = point_type::highest();
    m_upper = point_type::lowest();
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::partial_ordering quetzal::geometry::AxisAlignedBoundingBox<Traits>::compare(const point_type& point) const
{
    assert(ordered());

    std::partial_ordering result = std::partial_ordering::less;
    for (size_t i = 0; i < Traits::dimension; ++i)
    {
        if constexpr (std::is_floating_point_v<Traits::value_type>)
        {
            if (math::float_lt(point[i], m_lower[i]) || math::float_gt(point[i], m_upper[i]))
            {
                return std::partial_ordering::greater;
            }

            if (math::float_eq(point[i], m_lower[i]) || math::float_eq(point[i], m_upper[i]))
            {
                result = std::partial_ordering::equivalent;
            }
        }
        else // is there actually a use case for non floating point types? ...
        {
            if (point[i] < m_lower[i] || point[i] > m_upper[i])
            {
                return std::partial_ordering::greater;
            }

            if (point[i] == m_lower[i] || point[i] == m_upper[i])
            {
                result = std::partial_ordering::equivalent;
            }
        }
    }

    return result;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::AxisAlignedBoundingBox<Traits>::empty() const
{
    // this is only the extreme case, that created from default constructor ...
    // any inverted condition should also work, check for that instead? ...
    return m_lower == point_type::highest() && m_upper == point_type::lowest();
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::AxisAlignedBoundingBox<Traits>::print(std::ostream& os) const
{
    os << to_string(m_lower) << " .. " << to_string(m_upper);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::AxisAlignedBoundingBox<Traits>::ordered() const
{
    if (empty())
    {
        return true;
    }

    for (size_t i = 0; i < Traits::dimension; ++i)
    {
        if (m_upper[i] < m_lower[i])
        {
            return false;
        }
    }

    return true;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::AxisAlignedBoundingBox<Traits>::order()
{
    if (empty())
    {
        return;
    }

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
bool quetzal::geometry::operator==(const AxisAlignedBoundingBox<Traits>& lhs, const AxisAlignedBoundingBox<Traits>& rhs)
{
    return lhs.lower() == rhs.lower() && lhs.upper() == rhs.upper();
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::disjoint(const AxisAlignedBoundingBox<Traits>& a, const AxisAlignedBoundingBox<Traits>& b)
{
    for (size_t i = 0; i < Traits::dimension; ++i)
    {
        if (a.upper()[i] < b.lower()[i] || b.upper()[i] < a.lower()[i])
        {
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::AxisAlignedBoundingBox<Traits> quetzal::geometry::op_union(const AxisAlignedBoundingBox<Traits>& a, const AxisAlignedBoundingBox<Traits>& b)
{
    return {min(a.lower(), b.lower()), max(a.upper(), b.upper())};
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::AxisAlignedBoundingBox<Traits> quetzal::geometry::op_intersection(const AxisAlignedBoundingBox<Traits>& a, const AxisAlignedBoundingBox<Traits>& b)
{
    if (disjoint(a, b))
    {
        return {};
    }

    return {max(a.lower(), b.lower()), min(a.upper(), b.upper())};
}

#endif // QUETZAL_GEOMETRY_AXISALIGNEDBOUNDINGBOX_HPP
