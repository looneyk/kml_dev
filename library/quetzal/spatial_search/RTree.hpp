#if !defined(QUETZAL_SPATIAL_SEARCH_RTREE_HPP)
#define QUETZAL_SPATIAL_SEARCH_RTREE_HPP
//------------------------------------------------------------------------------
// spatial_search
// RTree.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/id.hpp"
#include "quetzal/common/string_util.hpp"
#include "quetzal/geometry/AxisAlignedBoundingBox.hpp"
#include "quetzal/math/floating_point.hpp"
#include <algorithm>
#include <iostream>
#include <mutex>
#include <numeric>
#include <optional>
#include <span>
#include <string>
#include <vector>
#include <cassert>

#include <set>

namespace quetzal::spatial_search
{

// PolygonPointTraversalTraits
// PolygonInteriorPointTraversalTraits
// PolygonRayTraversalTraits
// PolygonInteriorRayTraversalTraits

    //------------------------------------------------------------------------------
    template <typename R, typename Query>
    struct CountTraversalTraits
    {
        using value_type = R::value_type;
        using query_type = Query;
        using result_type = size_t;
        using primitive_type = R::primitive_type;
        using bounding_box_type = R::bounding_box_type;

        // this specifically for Query point, move these traversal traits separately

        static result_type empty_result()
        {
            return 0;
        }

        static bool intersects(const query_type& query, const bounding_box_type& boundingBox)
        {
            return boundingBox.contains(query);
        }

        static void intersects(const query_type& query, const primitive_type& primitive, result_type& result)
        {
            value_type dsquared = (query - primitive).norm_squared();
            if (dsquared <= value_type(1)) // float_le? ...
            {
//                std::cout << primitiveId << std::endl;
                ++result;
            }
        }

        static bool complete(const result_type&)
        {
            return false; // No early termination
        }
    };

    //------------------------------------------------------------------------------
    template <typename R, typename Query>
    struct InteriorCountTraversalTraits
    {
        using value_type = R::value_type;
        using query_type = Query;
        using result_type = std::optional<size_t>;
        using primitive_type = R::primitive_type;
        using bounding_box_type = R::bounding_box_type;

        static result_type empty_result()
        {
            return 0;
        }

        static bool intersects(const query_type& query, const bounding_box_type& boundingBox)
        {
            return boundingBox.contains(query);
        }

        // strictly interior
        static void intersects(const query_type& query, const primitive_type& primitive, result_type& result)
        {
            value_type dsquared = (query - primitive).norm_squared();
            if (math::float_eq(dsquared, value_type(1)))
            {
                result = std::nullopt;
                return;
            }

            if (dsquared < value_type(1))
            {
                ++result.value();
            }
        }

        static bool complete(const result_type& result)
        {
            return result == std::nullopt; // Early termination on boundary intersection
        }
    };

    //--------------------------------------------------------------------------
    template<typename Traits>
    class RTree
    {
    public:

        using primitive_type = Traits::primitive_type;
        using bounding_box_type = Traits::bounding_box_type;

        explicit RTree(std::span<primitive_type> primitives = {});
        RTree(const RTree&) = delete;
        RTree(RTree&&) = delete;
        ~RTree() = default;

        RTree& operator=(const RTree&) = delete;
        RTree& operator=(RTree&&) = delete;

        // Properties

        bounding_box_type bounding_box() const;
        size_t primitive_count() const;
        size_t size() const;
        bool built() const;
        bool empty() const;

        // Construction

        void insert(std::span<primitive_type> primitives);
        void insert(const primitive_type& primitive);
        void build() const; // semantically const
        void build_();
        void clear();

        // Search

        // Returns the number of primitives intersected by the query
        // Requires intersects operations
        template<typename Query>
        size_t intersection_count(const Query& query) const;

        // Returns the number of primitives intersected by the query
        // Fails on any edge intersection
        // Requires intersects operations
        template<typename Query>
        std::optional<size_t> interior_intersection_count(const Query& query) const;

        // Traversal

        template<typename TraversalTraits, typename Query = TraversalTraits::query_type>
        TraversalTraits::result_type traverse(const Query& query) const;

        void print(std::ostream& os) const;

    private:

        struct Node
        {
            bounding_box_type boundingBox;
            size_t nLeft;
            size_t nRight;
            size_t nPrimitive;

            Node(bounding_box_type boundingBox_ = bounding_box_type(), size_t nLeft_ = nullid, size_t nRight_ = nullid, size_t nPrimitive_ = nullid) :
                boundingBox(boundingBox_),
                nLeft(nLeft_),
                nRight(nRight_),
                nPrimitive(nPrimitive_)
            {
            }

            bool leaf() const
            {
                return nLeft == nullid && nRight == nullid;
            }

            void print(std::ostream& os, size_t n) const
            {
                os << n << '\t' << boundingBox << '\t'
                    << (nLeft == nullid ? "null" : quetzal::to_string(nLeft)) << '\t'
                    << (nRight == nullid ? "null" : quetzal::to_string(nRight)) << '\t'
                    << (nPrimitive == nullid ? "null" : quetzal::to_string(nPrimitive)) << std::endl;
                return;
            }
        };

        using primitives_type = std::vector<primitive_type>; // Traits::primitives_type ...
        using nodes_type = std::vector<Node>;
        using reference_points_type = Traits::reference_points_type;
        using bounding_boxes_type = Traits::bounding_boxes_type;
        using primitive_id_type = size_t;

        void create_node(size_t n, std::span<primitive_id_type> primitiveIds);
        void create_internal_node(size_t n, std::span<primitive_id_type> primitiveIds, size_t nLeft, size_t nRight);
        void create_leaf_node(size_t n, primitive_id_type primitiveId);

        void print_node(std::ostream& os, size_t n) const;

        // Recursive node visitor, applies predicates supplied by TraversalTraits
        template<typename TraversalTraits, typename Query>
        void visit(const Query& query, const Node& node, TraversalTraits::result_type& result) const;

        bounding_box_type bounding_box(std::span<primitive_type> primitives);
        bounding_box_type bounding_box(std::span<primitive_id_type> primitiveIds);
        size_t split(std::span<primitive_id_type> primitiveIds);

        nodes_type m_nodes;
        primitives_type m_primitives; // could be a span or external reference if not for non-bulk insertion ...
        bounding_boxes_type m_boundingBoxes;
        reference_points_type m_referencePoints;
        bool m_bBuilt;
        mutable std::mutex m_mutex;
    };

    template<typename Traits>
    std::ostream& operator<<(std::ostream& os, const RTree<Traits>& tree);

} // namespace quetzal::spatial_search

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::spatial_search::RTree<Traits>::RTree(std::span<primitive_type> primitives) :
    m_nodes(),
    m_primitives(primitives.begin(), primitives.end()),
    m_boundingBoxes(m_primitives),
    m_referencePoints(m_primitives),
    m_bBuilt(false)
{
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::spatial_search::RTree<Traits>::bounding_box_type quetzal::spatial_search::RTree<Traits>::bounding_box() const
{
    if (m_primitives.empty())
    {
        return bounding_box_type();
    }

    if (!m_bBuilt)
    {
        return bounding_box(m_primitives);
    }

    return m_nodes[0].boundingBox;
}

//------------------------------------------------------------------------------
template<typename Traits>
size_t quetzal::spatial_search::RTree<Traits>::primitive_count() const
{
    return m_primitives.size();
}

//------------------------------------------------------------------------------
template<typename Traits>
size_t quetzal::spatial_search::RTree<Traits>::size() const
{
    return m_nodes.size();
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::spatial_search::RTree<Traits>::built() const
{
    return m_bBuilt;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::spatial_search::RTree<Traits>::empty() const
{
    return m_primitives.empty();
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::spatial_search::RTree<Traits>::insert(std::span<primitive_type> primitives)
{
    m_bBuilt = false;
    m_primitives.append_range(primitives);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::spatial_search::RTree<Traits>::insert(const primitive_type& primitive)
{
    m_bBuilt = false;
    m_primitives.push_back(primitive);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::spatial_search::RTree<Traits>::build() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_bBuilt)
    {
        const_cast<RTree<Traits>*>(this)->build_();
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::spatial_search::RTree<Traits>::build_()
{
    m_nodes.clear();

    if (m_primitives.size() == 0)
    {
        return;
    }

    m_nodes.resize(m_primitives.size() * 2 - 1);

    std::vector<size_t> primitiveIds(m_primitives.size());
    std::iota(primitiveIds.begin(), primitiveIds.end(), 0);

    create_node(0, primitiveIds);
    assert(m_nodes.size() == m_primitives.size() * 2 - 1);

    m_bBuilt = true;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::spatial_search::RTree<Traits>::clear()
{
    m_bBuilt = false;
    m_nodes.clear();
    m_primitives.clear();
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename Query>
size_t quetzal::spatial_search::RTree<Traits>::intersection_count(const Query& query) const
{
    using count_traversal_traits = CountTraversalTraits<Traits, Query>;
    return traverse<count_traversal_traits, Query>(query);
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename Query>
std::optional<size_t> quetzal::spatial_search::RTree<Traits>::interior_intersection_count(const Query& query) const
{
    using interior_count_traversal_traits = InteriorCountTraversalTraits<Traits, Query>;
    return traverse<interior_count_traversal_traits, Query>(query);
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename TraversalTraits, typename Query>
TraversalTraits::result_type quetzal::spatial_search::RTree<Traits>::traverse(const Query& query) const
{
    if (!m_bBuilt)
    {
        build();
    }

    auto result = TraversalTraits::empty_result();
    if (m_nodes.size() == 0)
    {
        return result;
    }

    visit<TraversalTraits, Query>(query, m_nodes[0], result);
    return result;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::spatial_search::RTree<Traits>::print(std::ostream& os) const
{
    if (m_primitives.empty())
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
void quetzal::spatial_search::RTree<Traits>::create_node(size_t n, std::span<primitive_id_type> primitiveIds)
{
    assert(primitiveIds.size() > 0);

    switch (primitiveIds.size())
    {
        case 1:
            create_leaf_node(n, primitiveIds[0]);
            break;

        case 2:
            create_internal_node(n, primitiveIds, n + 1, n + 2);
            create_leaf_node(n + 1, primitiveIds[0]);
            create_leaf_node(n + 2, primitiveIds[1]);
            break;
            
        case 3:
            create_internal_node(n, primitiveIds, n + 1, n + 2);
            create_leaf_node(n + 1, primitiveIds[0]);
            create_node(n + 2, primitiveIds.subspan(1, 2));
            break;

        default:
            size_t middle = split(primitiveIds);
            size_t n1 = n + 1;
            size_t n2 = n + middle * 2; // offset beyond left subtree nodes
            create_internal_node(n, primitiveIds, n1, n2);
            create_node(n1, primitiveIds.subspan(0, middle));
            create_node(n2, primitiveIds.subspan(middle, primitiveIds.size() - middle));
            break;
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::spatial_search::RTree<Traits>::create_internal_node(size_t n, std::span<primitive_id_type> primitiveIds, size_t nLeft, size_t nRight)
{
    m_nodes[n] = {bounding_box(primitiveIds), nLeft, nRight, nullid};
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::spatial_search::RTree<Traits>::create_leaf_node(size_t n, primitive_id_type primitiveId)
{
    m_nodes[n] = {m_boundingBoxes[primitiveId], nullid, nullid, primitiveId};
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::spatial_search::RTree<Traits>::print_node(std::ostream& os, size_t n) const
{
    const Node& node = m_nodes[n];
    node.print(os, n);

    if (!node.leaf())
    {
        print_node(os, node.nLeft);
        print_node(os, node.nRight);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename TraversalTraits, typename Query>
void quetzal::spatial_search::RTree<Traits>::visit(const Query& query, const Node& node, TraversalTraits::result_type& result) const
{
    assert(m_bBuilt);

    if (!TraversalTraits::intersects(query, node.boundingBox))
    {
        return;
    }

    if (node.leaf())
    {
        TraversalTraits::intersects(query, m_primitives[node.nPrimitive], result);
        return;
    }

    if (node.nLeft != nullid)
    {
        visit<TraversalTraits, Query>(query, m_nodes[node.nLeft], result);
    }

    if (!TraversalTraits::complete(result) && node.nRight != nullid)
    {
        visit<TraversalTraits, Query>(query, m_nodes[node.nRight], result);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
Traits::bounding_box_type quetzal::spatial_search::RTree<Traits>::bounding_box(std::span<primitive_type> primitives)
{
    bounding_box_type box;
    for (const auto& primitive : primitives)
    {
        box = op_union(box, m_boundingBoxes[primitive.id()]);
    }

    return box;
}

//------------------------------------------------------------------------------
template<typename Traits>
Traits::bounding_box_type quetzal::spatial_search::RTree<Traits>::bounding_box(std::span<primitive_id_type> primitiveIds)
{
    bounding_box_type box;
    for (const auto& id : primitiveIds)
    {
        box = op_union(box, m_boundingBoxes[id]);
    }

    return box;
}

//------------------------------------------------------------------------------
template<typename Traits>
size_t quetzal::spatial_search::RTree<Traits>::split(std::span<primitive_id_type> primitiveIds)
{
    size_t middle = primitiveIds.size() / 2;
    size_t nComponent = m_boundingBoxes[primitiveIds[middle]].extent().longest_component();

    std::nth_element(primitiveIds.begin(), std::next(primitiveIds.begin(), middle), primitiveIds.end(),
        [this, nComponent](primitive_id_type lhs, primitive_id_type rhs) -> bool
        {
            return m_referencePoints[lhs][nComponent] < m_referencePoints[rhs][nComponent];
        });

    return middle;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::ostream& quetzal::spatial_search::operator<<(std::ostream& os, const RTree<Traits>& tree)
{
    tree.print(os);
    return os;
}

#endif // QUETZAL_SPATIAL_SEARCH_RTREE_HPP
