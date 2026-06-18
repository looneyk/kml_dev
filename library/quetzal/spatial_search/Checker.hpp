#if !defined(QUETZAL_SPATIAL_SEARCH_CHECKER_HPP)
#define QUETZAL_SPATIAL_SEARCH_CHECKER_HPP
//------------------------------------------------------------------------------
// spatial_search
// Checker.hpp
//------------------------------------------------------------------------------

#include "quetzal/spatial_search/RTree.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/Ray.hpp"
#include "quetzal/geometry/random.hpp"
#include <optional>

namespace quetzal::spatial_search
{

    // This specifically does a point in polyhedron test, so no need for any additional parameters
    // Rename to reflect this ...
    // create the tree internally instead of being passed in? allow for both possibilities? ...

    //--------------------------------------------------------------------------
    template<typename Traits>
    class Checker
    {
    public:

        Checker(const RTree<Traits>& tree);
        Checker(const Checker&) = delete;
        Checker(Checker&&) = delete;
        ~Checker() = default;

        Checker& operator=(const Checker&) = delete;
        Checker& operator=(Checker&&) = delete;

        // Returns three-way comparison result: -1, 0, 1: interior, boundary, exterior
        int compare(const geometry::Point<Traits>& point);

    private:

        const RTree<Traits>& m_tree;
    };

    template<typename Traits>
    std::optional<int> query(const geometry::Point<Traits>& point, const RTree<Traits>& tree);

} // namespace quetzal::spatial_search

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::spatial_search::Checker<Traits>::Checker(const RTree<Traits>& tree) :
    m_tree(tree)
{
}

//------------------------------------------------------------------------------
template<typename Traits>
int quetzal::spatial_search::Checker<Traits>::compare(const geometry::Point<Traits>& point)
{
    if (m_tree.bounding_box().contains(point))
    {
        if (!m_tree.built())
        {
            m_tree.build();
        }

        for (;;)
        {
            geometry::Ray<Traits> ray = {point, random_vector(Traits::val(1))};
            std::optional<int> nIntersections = m_tree.interior_intersection_count(ray);
            if (nIntersections)
            {
                return math::odd(nIntersections);
            }
        }
    }

    assert(false);
    return 1;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::optional<int> quetzal::spatial_search::query(const geometry::Point<Traits>& point, const RTree<Traits>& tree)
{
    // ...
    return {};
}

#endif // QUETZAL_SPATIAL_SEARCH_CHECKER_HPP
