#if !defined(QUETZAL_GEOMETRY_BOX_HPP)
#define QUETZAL_GEOMETRY_BOX_HPP
//------------------------------------------------------------------------------
// geometry
// Box.hpp
//------------------------------------------------------------------------------

#include "Point.hpp"
#include "PointRelation.hpp"
#include "quetzal/math/floating_point.hpp"
#include <iostream>

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    template<typename Traits>
    class Box : public PointRelation<Traits>
    {
    public:

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

        virtual void print(std::ostream& os) const;

    private:

        bool ordered() const;
        void order();

        point_type m_lower;
        point_type m_upper;
    };

    template<typename Traits>
    std::ostream& operator<<(std::ostream& os, const Box<Traits>& box);

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
            if (math::float_ne(point[i], m_lower[i], tolerance) || math::float_ne(point[i], m_upper[i], tolerance))
            {
                return std::partial_ordering::less;
            }
        }
        else
        {
            if (point[i] != m_lower[i] || point[i] != m_upper[i])
            {
                return std::partial_ordering::less;
            }
        }
    }

    return std::partial_ordering::equivalent;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Box<Traits>::print(std::ostream& os) const
{
    os << to_string(m_lower) << " .. " << to_string(m_upper);
    return;
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
    box.print(os);
    return os;
}

#endif // QUETZAL_GEOMETRY_BOX_HPP
