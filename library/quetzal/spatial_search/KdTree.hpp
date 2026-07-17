#if !defined(QUETZAL_SPATIAL_SEARCH_KDTREE_HPP)
#define QUETZAL_SPATIAL_SEARCH_KDTREE_HPP
//------------------------------------------------------------------------------
// spatial_search
// KdTree.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/id.hpp"
#include "quetzal/common/string_util.hpp"
#include "quetzal/math/floating_point.hpp"
#include "SeparatorPlane.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <iostream>
#include <map>
#include <mutex>
#include <numeric>
#include <span>
#include <string>
#include <tuple>
#include <vector>
#include <cassert>

namespace quetzal::spatial_search
{

    //--------------------------------------------------------------------------
    template<typename Traits>
    struct KdTreeParams
    {
        KdTreeParams(size_t nPointsLeafMax_ = 10) : nPointsLeafMax(nPointsLeafMax_) {}

        size_t nPointsLeafMax;
    };

    //------------------------------------------------------------------------------
    template <typename Traits>
    class NearestNeighborsQuery
    {
    public:

        using value_type = Traits::value_type;
        using point_type = Traits::point_type;
        using extended_point_type = Traits::extended_point_type;
        using results_type = std::vector<extended_point_type>;

        NearestNeighborsQuery(const point_type& point, size_t count) :
            m_point(point),
            m_count(count),
            m_results()
        {
        }

        bool contains(const extended_point_type& point) const
        {
            auto d2 = (Traits::point_accessor(point) - m_point).norm_squared();
            if (m_results.size() < m_count)
            {
                m_results.insert({d2, point});
                return true;
            }

            if (d2 < m_results.rbegin()->first)
            {
                m_results.erase(std::prev(m_results.end()));
                m_results.insert({d2, point});
                return true;
            }

            return false;
        }

        results_type results() const
        {
            results_type results;
            for (const auto& result : m_results)
            {
                results.push_back(result.second);
            }

            return results;
        }

    private:

        point_type m_point;
        size_t m_count;
        mutable std::map<value_type, extended_point_type> m_results;
    };

    //------------------------------------------------------------------------------
    template <typename Traits>
    class RadiusQuery
    {
    public:

        using value_type = Traits::value_type;
        using point_type = Traits::point_type;
        using extended_point_type = Traits::extended_point_type;
        using results_type = std::vector<extended_point_type>;

        RadiusQuery(const point_type& point, value_type radius) :
            m_point(point),
            m_radius(radius),
            m_results()
        {
        }

        bool contains(const extended_point_type& point) const
        {
            auto d2 = (Traits::point_accessor(point) - m_point).norm_squared();
            if (d2 <= m_radius)
            {
                m_results.push_back(point);
                return true;
            }

            return false;
        }

        results_type results() const
        {
            return m_results;
        }

    private:

        point_type m_point;
        value_type m_radius;
        mutable results_type m_results;
    };

    //--------------------------------------------------------------------------
    template<typename Traits>
    class KdTree
    {
    public:

        // points are stored only in leaves
        // extended points might typically include an id for external reference
        // point_accessor functor is used to extract an actual point from the extended points passed in

        using value_type = Traits::value_type;
        using point_type = Traits::point_type;
        using extended_point_type = Traits::extended_point_type;

        using bounding_box_type = Traits::bounding_box_type;
        using separator_type = SeparatorPlane<value_type>;

        explicit KdTree(std::span<extended_point_type> extendedPoints = {}, const KdTreeParams<Traits>& params = KdTreeParams<Traits>());
        KdTree(const KdTree&) = delete;
        KdTree(KdTree&&) = delete;
        ~KdTree() = default;

        KdTree& operator=(const KdTree&) = delete;
        KdTree& operator=(KdTree&&) = delete;

        // Properties

        bounding_box_type bounding_box() const;
        size_t point_count() const;
        size_t size() const;
        bool built() const;
        bool empty() const;

        // Construction

        void insert(std::span<extended_point_type> points);
        void insert(const extended_point_type& point);
        void build() const; // semantically const
        void build_();
        void clear();

        // Search

        // Performs a k-nearest neighbors search around the query point
        std::vector<extended_point_type> search_knn(const point_type& point, size_t count = 1) const;

        // Returns the set of points within a radius of the query point
        std::vector<extended_point_type> search_radius(const point_type& point, value_type radius) const;

        template<typename Query>
        Query::results_type traverse(const Query& query) const;

        void print(std::ostream& os) const;

    private:

        struct Node
        {
            union data
            {
                separator_type interiorSeparator;
                std::span<size_t> leafExtendedPointIds;

                data() {}
                data(const separator_type& separator) : interiorSeparator(separator) {}
                data(std::span<size_t> extendedPointIds) : leafExtendedPointIds(extendedPointIds) {}
            } data;

            size_t nLeft;
            size_t nRight;

            // Default node
            Node() :
                data(),
                nLeft(nullid),
                nRight(nullid)
            {
            }

            // Interior node
            Node(const separator_type& separator, size_t nLeft_, size_t nRight_) :
                data(separator),
                nLeft(nLeft_),
                nRight(nRight_)
            {
            }

            // Leaf node
            Node(std::span<size_t> extendedPointIds) :
                data(extendedPointIds),
                nLeft(nullid),
                nRight(nullid)
            {
            }

            bool leaf() const
            {
                return nLeft == nullid && nRight == nullid;
            }

//            size_t size() const
//            {
//                if (leaf())
//                {
//                    return 1;
//                }
//
//                return left().size() + right.size();
//            }

            void print(std::ostream& os) const
            {
                bool bFirst = true;
                if (leaf())
                {
                    os << &data.leafExtendedPointIds[0] << " ";
                    os << "[";
                    for (auto i : data.leafExtendedPointIds)
                    {
                        if (!bFirst)
                        {
                            os << ", ";
                        }
                        os << i;
                        bFirst = false;
                    }
                    os << "]\t";
                }
                else
                {
                    os << data.interiorSeparator << '\t';
                }

                os
                    << (nLeft == nullid ? "null" : quetzal::to_string(nLeft)) << '\t'
                    << (nRight == nullid ? "null" : quetzal::to_string(nRight)) << std::endl;
                return;
            }
        };

        using extended_points_type = std::vector<extended_point_type>;
        using nodes_type = std::vector<Node>;

        size_t create_node(std::span<size_t> extendedPointIds, const bounding_box_type& boundingBox);
        size_t create_internal_node(std::span<size_t> extendedPointIds, const bounding_box_type& boundingBox);
        size_t create_leaf_node(std::span<size_t> extendedPointIds);

        void print_node(std::ostream& os, size_t n) const;

        // Recursive node visitor, applies predicates supplied by Query
        template<typename Query>
        void visit(const Node& node, const Query& query) const;

        bounding_box_type bounding_box(std::span<size_t> extendedPointIds);

        separator_type split(const bounding_box_type& boundingBox);
        std::tuple<size_t, separator_type> split(std::span<size_t> extendedPointIds, const bounding_box_type& boundingBox);
        std::array<bounding_box_type, 2> split(const bounding_box_type& boundingBox, std::span<size_t> extendedPointIds, size_t middle, const separator_type& separator);

        KdTreeParams<Traits> m_params;
        extended_points_type m_extendedPoints; // could be a span or external reference if not for non-bulk insertion
        nodes_type m_nodes;
        bounding_box_type m_boundingBox;
        bool m_bBuilt;
        std::vector<size_t> m_extendedPointIds;
        mutable std::mutex m_mutex;
    };

    template<typename Traits>
    std::ostream& operator<<(std::ostream& os, const KdTree<Traits>& tree);

} // namespace quetzal::spatial_search

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::spatial_search::KdTree<Traits>::KdTree(std::span<extended_point_type> extendedPoints, const KdTreeParams<Traits>& params) :
    m_params(params),
    m_extendedPoints(extendedPoints.begin(), extendedPoints.end()),
    m_nodes(),
    m_boundingBox(),
    m_bBuilt(false),
    m_extendedPointIds(),
//    m_nNodes(),
    m_mutex()
{
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::spatial_search::KdTree<Traits>::bounding_box_type quetzal::spatial_search::KdTree<Traits>::bounding_box() const
{
    assert(m_bBuilt);
    return m_boundingBox;
}

//------------------------------------------------------------------------------
template<typename Traits>
size_t quetzal::spatial_search::KdTree<Traits>::point_count() const
{
    return m_extendedPoints.size();
}

//------------------------------------------------------------------------------
template<typename Traits>
size_t quetzal::spatial_search::KdTree<Traits>::size() const
{
    return m_nodes.size();
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::spatial_search::KdTree<Traits>::built() const
{
    return m_bBuilt;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::spatial_search::KdTree<Traits>::empty() const
{
    return m_extendedPoints.empty();
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::spatial_search::KdTree<Traits>::insert(std::span<extended_point_type> extendedPoints)
{
    m_bBuilt = false;
    m_extendedPoints.append_range(extendedPoints);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::spatial_search::KdTree<Traits>::insert(const extended_point_type& extendedPoint)
{
    m_bBuilt = false;
    m_extendedPoints.push_back(extendedPoint);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::spatial_search::KdTree<Traits>::build() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_bBuilt)
    {
        const_cast<KdTree<Traits>*>(this)->build_();
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::spatial_search::KdTree<Traits>::build_()
{
    m_nodes.clear();
    m_boundingBox.clear();

    if (m_extendedPoints.size() == 0)
    {
        return;
    }

    m_extendedPointIds.resize(m_extendedPoints.size());
    std::iota(m_extendedPointIds.begin(), m_extendedPointIds.end(), 0);

    m_boundingBox = bounding_box(m_extendedPointIds);
    create_node(m_extendedPointIds, m_boundingBox);

    m_bBuilt = true;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::spatial_search::KdTree<Traits>::clear()
{
    m_bBuilt = false;
    m_extendedPoints.clear();
    m_nodes.clear();
    m_boundingBox.clear();
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::vector<typename quetzal::spatial_search::KdTree<Traits>::extended_point_type> quetzal::spatial_search::KdTree<Traits>::search_knn(const point_type& point, size_t count) const
{
    NearestNeighborsQuery<Traits> query(point, count);
    return traverse(query);
}

//------------------------------------------------------------------------------
template<typename Traits>
std::vector<typename quetzal::spatial_search::KdTree<Traits>::extended_point_type> quetzal::spatial_search::KdTree<Traits>::search_radius(const point_type& point, value_type radius) const
{
    RadiusQuery<Traits> query(point, radius);
    return traverse(query);
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename Query>
Query::results_type quetzal::spatial_search::KdTree<Traits>::traverse(const Query& query) const
{
    if (!m_bBuilt)
    {
        build();
    }

    if (m_nodes.size() > 0)
    {
        visit(m_nodes[0], query);
    }

    return query.results();
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::spatial_search::KdTree<Traits>::print(std::ostream& os) const
{
    if (m_extendedPoints.empty())
    {
        std::cout << "empty" << std::endl;
        return;
    }

    if (!m_bBuilt)
    {
        build();
    }

    print_node(os, 0);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
size_t quetzal::spatial_search::KdTree<Traits>::create_node(std::span<size_t> extendedPointIds, const bounding_box_type& boundingBox)
{
    assert(extendedPointIds.size() > 0);

    if (extendedPointIds.size() <= m_params.nPointsLeafMax)
    {
        return create_leaf_node(extendedPointIds);
    }

    return create_internal_node(extendedPointIds, boundingBox);
}

//------------------------------------------------------------------------------
template<typename Traits>
size_t quetzal::spatial_search::KdTree<Traits>::create_internal_node(std::span<size_t> extendedPointIds, const bounding_box_type& boundingBox)
{
    assert(extendedPointIds.size() > 2);

    size_t n = m_nodes.size();
    m_nodes.push_back({}); // ...

    auto [middle, separator] = split(extendedPointIds, boundingBox);
    auto [boundingBoxLeft, boundingBoxRight] = split(boundingBox, extendedPointIds, middle, separator);

    size_t nLeft = create_node(extendedPointIds.subspan(0, middle), boundingBoxLeft);
    size_t nRight = create_node(extendedPointIds.subspan(middle, extendedPointIds.size() - middle), boundingBoxRight);

    m_nodes[n] = {separator, nLeft, nRight};
    return n;
}

//------------------------------------------------------------------------------
template<typename Traits>
size_t quetzal::spatial_search::KdTree<Traits>::create_leaf_node(std::span<size_t> extendedPointIds)
{
    assert(extendedPointIds.size() > 0);

    size_t n = m_nodes.size();
    m_nodes.push_back({extendedPointIds});
    return n;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::spatial_search::KdTree<Traits>::print_node(std::ostream& os, size_t n) const
{
    const Node& node = m_nodes[n];
    os << n << '\t';
    node.print(os);

    if (!node.leaf())
    {
        print_node(os, node.nLeft);
        print_node(os, node.nRight);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename Query>
void quetzal::spatial_search::KdTree<Traits>::visit(const Node& node, const Query& query) const
{
    assert(m_bBuilt);

//std::cout << "visit node: ";
//node.print(std::cout);
    if (node.leaf())
    {
//dump<Traits>("visi", node.data.leafExtendedPointIds, m_extendedPoints);
        for (auto i : node.data.leafExtendedPointIds)
        {
            query.contains(m_extendedPoints[i]);
        }
        return;
    }

    // see if we can reject either or both of these branches ...

    visit(m_nodes[node.nLeft], query);
    visit(m_nodes[node.nRight], query);

    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::spatial_search::KdTree<Traits>::bounding_box_type quetzal::spatial_search::KdTree<Traits>::bounding_box(std::span<size_t> extendedPointIds)
{
    bounding_box_type boundingBox;
    for (size_t i : extendedPointIds)
    {
        boundingBox.insert(Traits::point_accessor(m_extendedPoints, i));
    }

    return boundingBox;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::spatial_search::KdTree<Traits>::separator_type quetzal::spatial_search::KdTree<Traits>::split(const bounding_box_type& boundingBox)
{
    size_t nComponent = boundingBox.extent().longest_component();
    value_type value = (boundingBox.lower()[nComponent] + boundingBox.upper()[nComponent]) / Traits::value_type(2);
    return {nComponent, value};
}

//------------------------------------------------------------------------------
template<typename Traits>
std::tuple<size_t, typename quetzal::spatial_search::KdTree<Traits>::separator_type> quetzal::spatial_search::KdTree<Traits>::split(std::span<size_t> extendedPointIds, const bounding_box_type& boundingBox)
{
    separator_type separator = split(boundingBox);

    auto i = std::partition(extendedPointIds.begin(), extendedPointIds.end(),
        [this, separator](size_t i) -> bool
        {
            return Traits::point_accessor(m_extendedPoints, i)[separator.component()] < separator.value();
        });

    size_t middle = i - extendedPointIds.begin();
// sliding, this seems too simplistic ...
    if (middle == 0 || middle > extendedPointIds.size() - 1)
    {
        middle = extendedPointIds.size() / 2;
    }

    return {middle, separator};
}

//------------------------------------------------------------------------------
template<typename Traits>
std::array<typename quetzal::spatial_search::KdTree<Traits>::bounding_box_type, 2> quetzal::spatial_search::KdTree<Traits>::split(const bounding_box_type& boundingBox, std::span<size_t> extendedPointIds, size_t middle, const separator_type& separator)
{
    bounding_box_type boundingBoxLower = boundingBox;
    bounding_box_type boundingBoxUpper = boundingBox;
    size_t nComponent = separator.component();
    auto iMiddle = std::next(extendedPointIds.begin(), middle);

// ...
boundingBoxLower.upper()[nComponent] = separator.value();
boundingBoxUpper.lower()[nComponent] = separator.value();
/*
    // Recalculate upper split component of lower bounding box
    value_type valueLowerLower = boundingBoxLower.lower()[nComponent];
    value_type& valueLowerUpper = boundingBoxLower.upper()[nComponent];
    valueLowerUpper = valueLowerLower;
    for (auto i = extendedPointIds.begin(); i < iMiddle; ++i)
    {
        value_type value = Traits::point_accessor(m_extendedPoints, *i)[nComponent];
//std::cout << value << '\t' << valueLowerUpper << std::endl;
        valueLowerUpper = std::max(valueLowerUpper, value);
    }

    // Recalculate lower split component of upper bounding box
    value_type valueUpperUpper = boundingBoxUpper.upper()[nComponent];
    value_type& valueUpperLower = boundingBoxUpper.lower()[nComponent];
    valueUpperLower = valueUpperUpper;
    for (auto i = iMiddle; i < extendedPointIds.end(); ++i)
    {
        value_type value = Traits::point_accessor(m_extendedPoints, *i)[nComponent];
        valueUpperLower = std::min(valueUpperLower, value);
    }
*/
    return {boundingBoxLower, boundingBoxUpper};
}

//------------------------------------------------------------------------------
template<typename Traits>
std::ostream& quetzal::spatial_search::operator<<(std::ostream& os, const KdTree<Traits>& tree)
{
    tree.print(os);
    return os;
}

#endif // QUETZAL_SPATIAL_SEARCH_KDTREE_HPP
