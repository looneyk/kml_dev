#if !defined(QUETZAL_SPATIAL_SEARCH_POINTCONTAINER_HPP)
#define QUETZAL_SPATIAL_SEARCH_POINTCONTAINER_HPP
//------------------------------------------------------------------------------
// spatial_search
// PointContainer.hpp
//------------------------------------------------------------------------------

#include "KdTreeBox.hpp"
#include "quetzal/geometry/Point.hpp"
#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <vector>
#include <cassert>

namespace quetzal::spatial_search
{

    //--------------------------------------------------------------------------
    template<typename Traits>
    class PointContainer
    {
    public:

        using value_type = Traits::value_type;
        using point_type = Point<Traits>;
        using points_type = std::span<const point_type*>; // note pointers to points ...

        PointContainer();

        template<typename InputIterator>
        PointContainer(InputIterator first, InputIterator last);

        const KdTreeBox<Traits>& bounding_box() const;
        const KdTreeBox<Traits>& minimum_bounding_box() const;

        size_t dimension() const;
        void set_dimension(size_t nComponent);

        int max_tight_span_coord_balanced(value_type Aspect_ratio) const;

        value_type max_span_upper_without_dim(size_t nComponent) const;
        value_type balanced_fair(size_t nComponent, value_type Aspect_ratio);
        value_type balanced_sliding_fair(size_t nComponent, value_type Aspect_ratio);

        // points
        size_t size() const;

        const_iterator begin() const;
        const_iterator end() const;
        iterator begin();
        iterator end();

        bool empty() const;

        template<typename InputIterator>
        void set_range(InputIterator first, InputIterator last);

        void recompute_minimum_bounding_box();

        bool is_valid() const;

        // note that splitting is restricted to the built coordinate
        template <class Separator>
        void split(PointContainer<Traits>& c, Separator& separator, bool sliding = false);

        value_type median(const int split_dimension);

    private:

        // the iterator range of the PointContainer
        points_type m_points;

        KdTreeBox<Traits> m_box; // bounding box
        KdTreeBox<Traits> m_boxMin; // minimum bounding box
        size_t m_nComponent;    // the dimension for which the pointer list is built
    };

    template<class Traits>
    std::ostream& operator<<(std::ostream& os, PointContainer<Traits>& c);

} // namespace quetzal::spatial_search

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::spatial_search::PointContainer<Traits>::PointContainer() :
    m_box(),
    m_boxMin()
{
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename InputIterator>
quetzal::spatial_search::PointContainer<Traits>::PointContainer(InputIterator first, InputIterator last) :
    m_points(first, last),
    m_box(first, fast),
    m_boxMin(m_box),
    m_nComponent(m_box.max_extent_dimension())
{
}

//------------------------------------------------------------------------------
template<typename Traits>
const quetzal::spatial_search::KdTreeBox<Traits>& quetzal::spatial_search::PointContainer<Traits>::bounding_box() const
{
    return m_box;
}

//------------------------------------------------------------------------------
template<typename Traits>
const quetzal::spatial_search::KdTreeBox<Traits>& quetzal::spatial_search::PointContainer<Traits>::minimum_bounding_box() const
{
    return m_boxMin;
}

//------------------------------------------------------------------------------
template<typename Traits>
size_t quetzal::spatial_search::PointContainer<Traits>::dimension() const
{
    return m_nComponent;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::spatial_search::PointContainer<Traits>::set_dimension(size_t nComponent)
{
    m_nComponent = nComponent;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
int quetzal::spatial_search::PointContainer<Traits>::max_tight_span_coord_balanced(value_type Aspect_ratio) const
{
    int cut_dim = -1;
    value_type max_spread_points = value_type(-1);
    value_type max_length = m_box.max_extent();  // length of longest side of box
    for (int i = 0; i < Traits::dimension; ++i)
    {
        value_type length = m_box.extent(i);
        if (value_type(2) * max_length / length <= Aspect_ratio)
        {
            value_type spread = m_boxMin.extent(i);
            if (spread > max_spread_points)
            {
                max_spread_points = spread;
                cut_dim = i;
            }
        }
    }

    return cut_dim;
}

//------------------------------------------------------------------------------
template<typename Traits>
Traits::value_type quetzal::spatial_search::PointContainer<Traits>::max_span_upper_without_dim(size_t nComponent) const
{
    value_type max_extent = value_type(0);
    for (int i = 0; i < Traits::dimension; ++i)
    {
        value_type extent = m_box.extent(i);
        if (i != nComponent && extent > max_extent)
        {
            max_extent = extent;
        }
    }

    return max_extent;
}

//------------------------------------------------------------------------------
template<typename Traits>
Traits::value_type quetzal::spatial_search::PointContainer<Traits>::balanced_fair(size_t nComponent, value_type Aspect_ratio)
{
    value_type small_piece = max_span_upper_without_dim(nComponent) / Aspect_ratio;
    value_type low_cut = m_box.lower_component(nComponent) + small_piece; // lowest legal cut;
    value_type high_cut = m_box.upper_component(nComponent) - small_piece; // highest legal cut;
    // assert (high_cut >= low_cut);
    value_type split_value = median(nComponent);
    if (split_value < low_cut)
        split_value = low_cut;
    if (split_value > high_cut)
        split_value = high_cut;
    return split_value;
}

//------------------------------------------------------------------------------
template<typename Traits>
Traits::value_type quetzal::spatial_search::PointContainer<Traits>::balanced_sliding_fair(size_t nComponent, value_type Aspect_ratio)
{
    value_type small_piece = max_span_upper_without_dim(nComponent) / Aspect_ratio;
    value_type low_cut = m_box.lower_component(nComponent) + small_piece; // lowest legal cut;
    value_type high_cut = m_box.upper_component(nComponent) - small_piece; // highest legal cut;
    assert(high_cut >= low_cut);
    value_type split_value = median(nComponent);
    if (split_value < low_cut)
        split_value = m_boxMin.lower_component(nComponent);
    if (split_value > high_cut)
        split_value = m_boxMin.upper_component(nComponent);
    return split_value;
}

//  points
//------------------------------------------------------------------------------
template<typename Traits>
size_t quetzal::spatial_search::PointContainer<Traits>::size() const
{
    return m_points.size();
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::spatial_search::PointContainer<Traits>::const_iterator quetzal::spatial_search::PointContainer<Traits>::begin() const
{
    return m_points.begin();
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::spatial_search::PointContainer<Traits>::const_iterator quetzal::spatial_search::PointContainer<Traits>::end() const
{
    return m_points.end();
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::spatial_search::PointContainer<Traits>::iterator quetzal::spatial_search::PointContainer<Traits>::begin()
{
    return m_points.begin();
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::spatial_search::PointContainer<Traits>::iterator quetzal::spatial_search::PointContainer<Traits>::end()
{
    return m_points.end();
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::spatial_search::PointContainer<Traits>::empty() const
{
    return m_points.empty();
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename InputIterator>
void quetzal::spatial_search::PointContainer<Traits>::set_range(InputIterator first, InputIterator last)
{
    m_points = std::span(first, last);
    return;
}
/*
template<class Traits2>
struct Cmp
{
    typedef typename Traits2::value_type value_type;
    typedef typename Traits2::Point_d Point_d;

    int split_coord;
    value_type value;
    const typename Traits2::Construct_cartesian_const_iterator_d& construct_it;

    Cmp(int s, value_type c,const typename Traits2::Construct_cartesian_const_iterator_d& cst_it) :
        split_coord(s), value(c), construct_it(cst_it)
    {
    }

    bool operator()(const Point_d* pt) const
    {
        typename Traits2::Cartesian_const_iterator_d ptit;
        ptit = construct_it(*pt);
        return  *(ptit+split_coord) < value;
    }
};

template<class Traits2>
struct Between
{
    typedef typename Traits2::value_type value_type;
    typedef typename Traits2::Point_d Point_d;

    int split_coord;
    value_type low, high;
    const typename Traits2::Construct_cartesian_const_iterator_d& construct_it;

    Between(int s, value_type l, value_type h,const typename Traits2::Construct_cartesian_const_iterator_d& cst_it) :
        split_coord(s),
        low(l),
        high(h),
        construct_it(cst_it)
    {
    }

    bool operator()(const Point_d* pt) const
    {
        typename Traits2::Cartesian_const_iterator_d ptit = construct_it(*pt);
        if (!(*(ptit + split_coord) <= high))
        {
            //        std::cerr << "Point " << *pt << " exceeds " << high << " in dimension " << split_coord << std::endl;
            return false;
        }
        if (!(*(ptit + split_coord) >= low))
        {
            //std::cerr << "Point " << *pt << " below " << low << " in dimension " << split_coord << std::endl;
            return false;
        }
        return true;
    }
};

template <class Traits2, class Value>
struct comp_coord_val
{
private:
    Value coord;
    const typename Traits2::Construct_cartesian_const_iterator_d& construct_it;

    typedef typename Traits2::Point_d Point_d;
public:
    comp_coord_val (const Value& coordinate, const typename Traits2::Construct_cartesian_const_iterator_d& cst_it) :
        coord(coordinate),
        construct_it(cst_it)
    {
    }

    bool operator()(const Point_d *a, const Point_d *b) const
    {
        typename Traits2::Cartesian_const_iterator_d ait = construct_it(*a), bit = construct_it(*b);
        return *(ait + coord) < *(bit + coord);
    }
};
*/
//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::spatial_search::PointContainer<Traits>::recompute_minimum_bounding_box()
{
    m_boxMin.template update_from_point_pointers<typename Traits::Construct_cartesian_const_iterator_d>(begin(), end(), Traits::construct_cartesian_const_iterator_d_object());
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::spatial_search::PointContainer<Traits>::is_valid() const
{
    if (empty())
        return true;

    bool b = true;
    for (int i = 0; i < Traits::dimension; ++i)
    {
        b = (b && (m_box.lower_component(i) <= m_boxMin.lower_component(i)));
        assert(b);
        b = (b && (m_box.upper_component(i) >= m_boxMin.upper_component(i)));
        assert(b);

        typename Traits::Construct_cartesian_const_iterator_d construct_it=Traits::construct_cartesian_const_iterator_d_object();

//        Between<Traits> between(i, m_boxMin.lower_component(i), m_boxMin.upper_component(i), construct_it);
        auto between = [&](const Point& point)
        { 
            typename Traits2::Cartesian_const_iterator_d ptit = construct_it(*pt);
            if (!(*(ptit + i) <= m_boxMin.upper_component(i)))
            {
                // std::cerr << "Point " << *pt << " exceeds " << high << " in dimension " << split_coord << std::endl;
                return false;
            }
            if (!(*(ptit + i) >= m_boxMin.lower_component(i)))
            {
                // std::cerr << "Point " << *pt << " below " << low << " in dimension " << split_coord << std::endl;
                return false;
            }
            return true;
        };

        for (const_iterator j = begin(); j != end(); ++j)
        {
            b = (b && between(*j));
        }
    }

    return b;
}

// note that splitting is restricted to the built coordinate
//------------------------------------------------------------------------------
template<typename Traits>
template <class Separator>
void quetzal::spatial_search::PointContainer<Traits>::split(PointContainer<Traits>& c, Separator& separator, bool sliding)
{
    assert(is_valid());
    c.m_box = m_box;

    size_t split_dimension = separator.dimension();
    value_type split_value = separator.value();

    m_nComponent = split_dimension;
    c.set_dimension(split_dimension);

    typename Traits::Construct_cartesian_const_iterator_d construct_it = Traits::construct_cartesian_const_iterator_d_object();

//    Cmp<Traits> cmp(split_dimension, split_value, construct_it);
//    iterator it = std::partition(begin(), end(), cmp);
    auto component_less = [&](const Point& point) {
        typename Traits2::Cartesian_const_iterator_d ptit = construct_it(*pt);
        return *(ptit + split_dimension) < split_value;
    };

    // should be an operation of the point type? ...
    auto component_compare = [&](const Point_d *a, const Point_d *b) {
        typename Traits2::Cartesian_const_iterator_d ait = construct_it(*a), bit = construct_it(*b);
        return *(ait + split_dimension) < *(bit + split_dimension);
    };

    iterator it = std::partition(begin(), end(), component_less);
    // now [begin, it) are lower and [it, end) are upper
    if (sliding) // avoid empty lists
    {
        if (it == begin())
        {
//            iterator minelt = std::min_element(begin(), end(), comp_coord_val<Traits,int>(split_dimension, construct_it));
            iterator minelt = std::min_element(begin(), end(), component_compare);
            if (minelt != it)
            {
                std::iter_swap(minelt,it);
            }
            split_value = *(construct_it(**it) + split_dimension);
            separator.set_value(split_value);
            ++it;
        }
        if (it == end())
        {
//            iterator maxelt = std::max_element(begin(), end(), comp_coord_val<Traits,int>(split_dimension, construct_it));
            iterator maxelt = std::max_element(begin(), end(), component_compare);
            --it;
            if (maxelt != it)
            {
                std::iter_swap(maxelt, it);
            }
            split_value = *(construct_it(**it) + split_dimension);
            separator.set_value(split_value);
        }
    }

    c.set_range(begin(), it);
    set_range(it, end());
    // adjusting boxes
    m_box.set_lower_bound(split_dimension, split_value);
    m_boxMin. template update_from_point_pointers<typename Traits::Construct_cartesian_const_iterator_d>(begin(),end(),construct_it);
    c.m_box.set_upper_bound(split_dimension, split_value);
    c.m_boxMin. template update_from_point_pointers<typename Traits::Construct_cartesian_const_iterator_d>(c.begin(),c.end(),construct_it);

    assert(is_valid());
    assert(c.is_valid());
}

//------------------------------------------------------------------------------
template<typename Traits>
Traits::value_type quetzal::spatial_search::PointContainer<Traits>::median(const int split_dimension)
{
    typename Traits::Construct_cartesian_const_iterator_d construct_it = Traits::construct_cartesian_const_iterator_d_object();
    iterator mid = begin() + (end() - begin()) / 2;

//    std::nth_element(begin(), mid, end(), comp_coord_val<Traits,int>(split_dimension, construct_it));
    auto component_compare = [&](const Point_d *a, const Point_d *b)
    {
        typename Traits2::Cartesian_const_iterator_d ait = construct_it(*a), bit = construct_it(*b);
        return *(ait + split_dimension) < *(bit + split_dimension);
    };
    std::nth_element(begin(), mid, end(), component_compare);

    typename Traits::Cartesian_const_iterator_d mpit = construct_it((*(*mid)));
    value_type val1 = *(mpit + split_dimension);
    ++mid;
    mpit = construct_it((*(*mid)));
    value_type val2 = *(mpit + split_dimension);
    return (val1+val2)/value_type(2);
}

//------------------------------------------------------------------------------
template<typename Traits>
std::ostream& quetzal::spatial_search::operator<<(std::ostream& os, PointContainer<Traits>& c)
{
    os << "Points container of size " << c.size() << std::endl;
    os << "cell: " << c.bounding_box() << std::endl;
    os << "minimum bounding box: " << c.minimum_bounding_box() << std::endl;
    return os;
}

#endif // QUETZAL_SPATIAL_SEARCH_POINTCONTAINER_HPP
