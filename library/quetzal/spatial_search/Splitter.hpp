#if !defined(QUETZAL_SPATIAL_SEARCH_SPLITTER_HPP)
#define QUETZAL_SPATIAL_SEARCH_SPLITTER_HPP
//------------------------------------------------------------------------------
// spatial_search
// Splitter.hpp
//------------------------------------------------------------------------------

#include "PointContainer.hpp"
#include "SeparatorPlane.hpp"
#include <iostream>
#include <cassert>

namespace quetzal::spatial_search
{

    //--------------------------------------------------------------------------
    template<typename Traits, typename Separator = SeparatorPlane<typename Traits::value_type>>
    class Splitter
    {
    public:

        using value_type = typename Traits::value_type;
        using container_type = PointContainer<Traits>;

        explicit Splitter(size_t bucketsize = 10, value_type aspectratio = value_type(3));
        Splitter(const Splitter&) = default;
        Splitter(Splitter&&) = default;
        virtual ~Splitter() = default;

        Splitter& operator=(const Splitter&) = default;
        Splitter& operator=(Splitter&&) = default;

        size_t bucket_size() const;
        value_type aspect_ratio() const;

        virtual void operator()(container_type& containerA, container_type& containerB, Separator& separator) const = 0;

    protected:

        size_t m_bucketsize;
        value_type m_aspectratio;
    };

    //--------------------------------------------------------------------------
    template<typename Traits, typename Separator = SeparatorPlane<typename Traits::value_type>>
    class Median_of_max_spread : public Splitter<Traits, Separator>
    {
    public:

        using Splitter::value_type;
        using Splitter::container_type;

        Median_of_max_spread();
        Median_of_max_spread(size_t bucketsize);

        void operator()(container_type& containerA, container_type& containerB, Separator& separator) const override;
    };

    //--------------------------------------------------------------------------
    template<typename Traits, typename Separator = SeparatorPlane<typename Traits::value_type>>
    class Fair : public Splitter<Traits, Separator>
    {
    public:

        using Splitter::value_type;
        using Splitter::container_type;

        Fair();
        Fair(size_t bucketsize, value_type aspectratio = value_type(3));

        void operator()(container_type& containerA, container_type& containerB, Separator& separator) const override;
    };

    //--------------------------------------------------------------------------
    template<typename Traits, typename Separator = SeparatorPlane<typename Traits::value_type>>
    class Sliding_fair : public Splitter<Traits, Separator>
    {
    public:

        using Splitter::value_type;
        using Splitter::container_type;

        Sliding_fair();
        Sliding_fair(size_t bucketsize, value_type aspectratio = value_type(3));

        void operator()(container_type& containerA, container_type& containerB, Separator& separator) const override;
    };

    //--------------------------------------------------------------------------
    template<typename Traits, typename Separator = SeparatorPlane<typename Traits::value_type>>
    class Sliding_midpoint : public Splitter<Traits, Separator>
    {
    public:

        using Splitter::value_type;
        using Splitter::container_type;

        Sliding_midpoint();
        Sliding_midpoint(size_t bucketsize);

        void operator()(container_type& containerA, container_type& containerB, Separator& separator) const override;
    };

    //--------------------------------------------------------------------------
    template<typename Traits, typename Separator = SeparatorPlane<typename Traits::value_type>>
    class Median_of_rectangle : public Splitter<Traits, Separator>
    {
    public:

        using Splitter::value_type;
        using Splitter::container_type;

        Median_of_rectangle();
        Median_of_rectangle(size_t bucketsize);

        void operator()(container_type& containerA, container_type& containerB, Separator& separator) const override;
    };

    //--------------------------------------------------------------------------
    template<typename Traits, typename Separator = SeparatorPlane<typename Traits::value_type>>
    class Midpoint_of_max_spread : public Splitter<Traits, Separator>
    {
    public:

        using Splitter::value_type;
        using Splitter::container_type;

        Midpoint_of_max_spread();
        Midpoint_of_max_spread(size_t bucketsize);

        void operator()(container_type& containerA, container_type& containerB, Separator& separator) const override;
    };

    //--------------------------------------------------------------------------
    template<typename Traits, typename Separator = SeparatorPlane<typename Traits::value_type>>
    class Midpoint_of_rectangle : public Splitter<Traits, Separator>
    {
    public:

        using Splitter::value_type;
        using Splitter::container_type;

        Midpoint_of_rectangle();
        Midpoint_of_rectangle(size_t bucketsize);

        void operator()(container_type& containerA, container_type& containerB, Separator& separator) const override;
    };

} // namespace quetzal::spatial_search

//------------------------------------------------------------------------------
template<typename Traits, typename Separator>
quetzal::spatial_search::Splitter<Traits, Separator>::Splitter(size_t bucketsize, value_type aspectratio) :
    m_bucketsize(bucketsize),
    m_aspectratio(aspectratio)
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename Separator>
size_t quetzal::spatial_search::Splitter<Traits, Separator>::bucket_size() const
{
    return m_bucketsize;
}

//------------------------------------------------------------------------------
template<typename Traits, typename Separator>
quetzal::spatial_search::Splitter<Traits, Separator>::value_type quetzal::spatial_search::Splitter<Traits, Separator>::aspect_ratio() const
{
    return m_aspectratio;
}

//------------------------------------------------------------------------------
template<typename Traits, typename Separator>
quetzal::spatial_search::Median_of_max_spread<Traits, Separator>::Median_of_max_spread() :
    Splitter()
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename Separator>
quetzal::spatial_search::Median_of_max_spread<Traits, Separator>::Median_of_max_spread(size_t bucketsize) :
    Splitter(bucketsize)
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename Separator>
void quetzal::spatial_search::Median_of_max_spread<Traits, Separator>::operator()(container_type& containerA, container_type& containerB, Separator& separator) const
{
    separator.set(containerA.minimum_bounding_box().max_extent_dimension(), containerA.median(separator.dimension());
    containerA.split(containerB, separator, true);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename Separator>
quetzal::spatial_search::Fair<Traits, Separator>::Fair() :
    Splitter()
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename Separator>
quetzal::spatial_search::Fair<Traits, Separator>::Fair(size_t bucketsize, value_type aspectratio) :
    Splitter(bucketsize, aspectratio)
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename Separator>
void quetzal::spatial_search::Fair<Traits, Separator>::operator()(container_type& containerA, container_type& containerB, Separator& separator) const
{
    separator.set(containerA.max_tight_span_coord_balanced(m_aspectratio), containerA.balanced_fair(separator.dimension(), m_aspectratio));
    containerA.split(containerB, separator);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename Separator>
quetzal::spatial_search::Sliding_fair<Traits, Separator>::Sliding_fair() :
    Splitter()
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename Separator>
quetzal::spatial_search::Sliding_fair<Traits, Separator>::Sliding_fair(size_t bucketsize, value_type aspectratio) :
    Splitter(bucketsize, aspectratio)
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename Separator>
void quetzal::spatial_search::Sliding_fair<Traits, Separator>::operator()(container_type& containerA, container_type& containerB, Separator& separator) const
{
    separator.set(containerA.max_tight_span_coord_balanced(m_aspectratio), containerA.balanced_sliding_fair(separator.dimension(), m_aspectratio));
    containerA.split(containerB, separator, true);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename Separator>
quetzal::spatial_search::Sliding_midpoint<Traits, Separator>::Sliding_midpoint() :
    Splitter()
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename Separator>
quetzal::spatial_search::Sliding_midpoint<Traits, Separator>::Sliding_midpoint(size_t bucketsize) :
    Splitter(bucketsize)
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename Separator>
void quetzal::spatial_search::Sliding_midpoint<Traits, Separator>::operator()(container_type& containerA, container_type& containerB, Separator& separator) const
{
    assert(containerA.is_valid());
    assert(containerB.is_valid());

    int nComponent = containerA.bounding_box().max_extent_dimension();

    // Avoid linear tree in degenerate cases
    if (containerA.minimum_bounding_box().lower(nComponent) != containerA.minimum_bounding_box().upper(nComponent))
    {
        separator.set(nComponent, containerA.bounding_box().max_extent_midpoint());
    }
    else
    {
        nComponent = containerA.minimum_bounding_box().max_extent_dimension();
        separator.set(nComponent, containerA.minimum_bounding_box().max_extent_midpoint());
    }

    value_type extent_lower = containerA.minimum_bounding_box().lower(nComponent);
    assert(extent_lower >= containerA.bounding_box().lower(nComponent));

    value_type extent_upper = containerA.minimum_bounding_box().upper(nComponent);
    assert(extent_upper <= containerA.bounding_box().upper(nComponent));

    if (extent_upper <= separator.value())
    {
        separator.set_value(extent_upper);
    }

    if (extent_lower >= separator.value())
    {
        separator.set_value(extent_lower);
    }

    containerA.split(containerB, separator, true);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename Separator>
quetzal::spatial_search::Median_of_rectangle<Traits, Separator>::Median_of_rectangle() :
    Splitter()
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename Separator>
quetzal::spatial_search::Median_of_rectangle<Traits, Separator>::Median_of_rectangle(size_t bucketsize) :
    Splitter(bucketsize)
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename Separator>
void quetzal::spatial_search::Median_of_rectangle<Traits, Separator>::operator()(container_type& containerA, container_type& containerB, Separator& separator) const
{
    separator.set(containerA.bounding_box().max_extent_dimension(), containerA.median(separator.dimension());
    containerA.split(containerB, separator, true);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename Separator>
quetzal::spatial_search::Midpoint_of_max_spread<Traits, Separator>::Midpoint_of_max_spread() :
    Splitter()
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename Separator>
quetzal::spatial_search::Midpoint_of_max_spread<Traits, Separator>::Midpoint_of_max_spread(size_t bucketsize) :
    Splitter(bucketsize)
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename Separator>
void quetzal::spatial_search::Midpoint_of_max_spread<Traits, Separator>::operator()(container_type& containerA, container_type& containerB, Separator& separator) const
{
    separator.set(containerA.minimum_bounding_box().max_extent_dimension(), containerA.minimum_bounding_box().max_extent_midpoint());
    containerA.split(containerB, separator);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename Separator>
quetzal::spatial_search::Midpoint_of_rectangle<Traits, Separator>::Midpoint_of_rectangle() :
    Splitter()
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename Separator>
quetzal::spatial_search::Midpoint_of_rectangle<Traits, Separator>::Midpoint_of_rectangle(size_t bucketsize) :
    Splitter(bucketsize)
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename Separator>
void quetzal::spatial_search::Midpoint_of_rectangle<Traits, Separator>::operator()(container_type& containerA, container_type& containerB, Separator& separator) const
{
    separator.set(containerA.bounding_box().max_extent_dimension(), containerA..bounding_box().max_extent_midpoint());
    containerA.split(containerB, separator);
    return;
}

#endif // QUETZAL_SPATIAL_SEARCH_SPLITTER_HPP
