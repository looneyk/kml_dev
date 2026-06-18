#if !defined(QUETZAL_SPATIAL_SEARCH_NKD_HPP)
#define QUETZAL_SPATIAL_SEARCH_NKD_HPP

#include "PooledAllocator.hpp"
#include "quetzal/geometry/AxisAlignedBoundingBox.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/math/CartesianTraits.hpp"
#include <algorithm>
#include <array>
#include <functional>
#include <limits>
#include <tuple>
#include <vector>
#include <cassert>
#include <cmath>

namespace quetzal::spatial_search
{

    //--------------------------------------------------------------------------
    template <typename T, typename I = size_t, typename S = size_t>
    class KNNResultSet
    {
    public:

        using value_type = T;
        using key_type = I;
        using size_type = S;
        using IndexedDistance = std::pair<key_type, value_type>;
        using IndexedDistances = std::vector<IndexedDistance>;

        KNNResultSet(size_type capacity, IndexedDistances& indexedDistances) :
            m_capacity(capacity),
            m_count(0),
            m_indexedDistances(indexedDistances)
        {
            if (m_capacity)
            {
                indexedDistances[m_capacity - 1].second = (std::numeric_limits<value_type>::max)();
            }
        }

        size_type size() const { return m_count; }

        bool full() const { return m_count == m_capacity; }

        // Called during search to add an element matching the criteria.
        // return true if the search should be continued, false if the results are sufficient
        bool addPoint(value_type distance, key_type key)
        {
            size_type i;
            for (i = m_count; i > 0; --i)
            {
                auto& [key1, dist1] = m_indexedDistances[i - 1];

#ifdef NANOFLANN_FIRST_MATCH // If defined and two points have the same distance, the one with the lowest-key will be returned first.
                if ((dist1 > distance) || ((distance == dist1) && (key1 > key)))
                {
#else
                if (dist1 > distance)
                {
#endif
                    if (i < m_capacity)
                    {
                        m_indexedDistances[i] = std::make_pair(key1, dist1);
                    }
                }
                else
                    break;
            }
            if (i < m_capacity)
            {
                m_indexedDistances[i] = std::make_pair(key, distance);
            }
            if (m_count < m_capacity)
                ++m_count;

            return true; // continue the search
        }

        value_type worstDist() const { return m_indexedDistances[m_capacity - 1].second; }

    private:

        size_type m_capacity;
        size_type m_count;
        IndexedDistances& m_indexedDistances;
    };

    //--------------------------------------------------------------------------
    struct IndexedDistanceLess
    {
        // PairType will be typically: std::pair<key_type, value_type>
        template <typename PairType>
        bool operator()(const PairType& p1, const PairType& p2) const
        {
            return p1.second < p2.second;
        }
    };

    template <typename T>
    auto indexed_distance_less = []<T>(const T& a, const T& b) -> bool { return a.second < b.second; };

    //--------------------------------------------------------------------------
    // A result-set class used when performing a radius based search.
    template <typename T, typename I = size_t>
    class RadiusResultSet
    {
    public:

        using value_type = T;
        using key_type = I;
        using IndexedDistance = std::pair<key_type, value_type>;
        using IndexedDistances = std::vector<IndexedDistance>;

        RadiusResultSet(value_type radius, IndexedDistances& indexedDistances) :
            m_radius(radius),
            m_indexedDistances(indexedDistances)
        {
            init();
        }

        void init() { clear(); }
        void clear() { m_indexedDistances.clear(); }

        size_t size() const { return m_indexedDistances.size(); }

        bool full() const { return true; }

        // Called during search to add an element matching the criteria.
        // return true if the search should be continued, false if the results are sufficient
        bool addPoint(value_type distance, key_type key)
        {
            if (distance < m_radius)
                m_indexedDistances.push_back(std::make_pair(key, distance));
            return true;
        }

        value_type worstDist() const { return m_radius; }

        // Find the worst result (furtherest neighbor) without copying or sorting.
        IndexedDistance worst_item() const
        {
            assert(!m_indexedDistances.empty());
            auto it = std::max_element(m_indexedDistances.begin(), m_indexedDistances.end(), IndexedDistanceLess());
            return *it;
        }

    private:

        const value_type m_radius;
        IndexedDistances& m_indexedDistances;
    };

    //--------------------------------------------------------------------------
    struct Metric {};

    // Squared Euclidean (L2) distance functor (suitable for low-dimensionality datasets, like 2D or 3D point clouds), traits: Metric_L2_Simple
    // T Type of the distance, components (e.g. float, double)
    // DataSource Source of the data, i.e. where the vectors are stored, provides DatasetAdapter interface
    // AccessorType Type of the arguments with which the data can be accessed (e.g. int64_t, T*)
    template <typename DataSource, typename AccessorType = size_t>
    class L2_Simple_Adapter
    {
    public:

        using value_type = DataSource::value_type;
        using point_type = DataSource::point_type;

        L2_Simple_Adapter(const DataSource& datasource) :
            m_datasource(datasource)
        {}

        value_type eval(const point_type& a, const AccessorType b_idx, size_t size) const
        {
            value_type result = value_type(0);
            for (size_t i = 0; i < size; ++i)
            {
                value_type diff = a[i] - m_datasource.point_component(b_idx, i);
                result += diff * diff;
            }
            return result;
        }

        template <typename U, typename V>
        value_type accum_dist(const U a, const V b, size_t) const
        {
            return (a - b) * (a - b);
        }

    private:

        const DataSource& m_datasource;
    };

    // Metaprogramming helper traits class for the L2_simple (Euclidean) metric
    struct Metric_L2_Simple : public Metric
    {
        template <typename DataSource, typename AccessorType = size_t>
        struct traits
        {
            using distance_t = L2_Simple_Adapter<DataSource, AccessorType>;
        };
    };

    //--------------------------------------------------------------------------
    struct KDTreeSingleIndexAdapterParams
    {
        KDTreeSingleIndexAdapterParams(size_t _leaf_size_max = 10) :
            leaf_size_max(_leaf_size_max)
        {}

        size_t leaf_size_max;
    };

    //--------------------------------------------------------------------------
    struct SearchParams
    {
        SearchParams(float eps_ = 0, bool sorted_ = true) :
            eps(eps_),
            sorted(sorted_)
        {}

        float eps;   // search for eps-approximate neighbours (default: 0)
        bool sorted; // only for radius search, require neighbours sorted by distance (default: true)
    };

    //--------------------------------------------------------------------------
    // Dataset interface
    template <typename P, typename N, typename BBOX>
    struct Dataset
    {
        using value_type = P::value_type;
        using point_type = P;
        using size_type = size_t;
        static constexpr size_type dimension = N; // dimension_type vs size_type? ...

        virtual ~Dataset() = default;

        // The number of data points
        virtual size_t point_count() const = 0;

        // The i'th point in the set
        virtual point_type point(size_t i) const = 0;

        // The n'th component of the i'th point in the set
        virtual value_type point_component(size_t i, size_t n) const = 0;

        // Optional bounding-box computation
        // Return false to default to standard bbox computation
        // Return true if the BBOX was already computed and returned in "bb" so recalculation can be avoided
//        template <typename BBOX>
        virtual bool get_bbox(BBOX& bbox) const = 0;
    };

    //--------------------------------------------------------------------------
    // KDTreeBase
    // Derived The class which inherits this class.
    // DatasetAdapter Interface to the dataset that implements DatasetAdapter functionality
    // DistanceMetric The distance metric to use, derived from Metric
    // AccessorType Will be typically size_t or int
    template <typename Derived, typename DatasetAdapter, typename DistanceMetric, typename AccessorType = size_t>
    class KDTreeBase
    {
    public:

        // Frees the previously-built index. Automatically called within build_index().
        void free_index(Derived& obj)
        {
            obj.pool.free_all();
            obj.root_node = nullptr;
            obj.m_size_at_index_build = 0;
        }

        using value_type = DatasetAdapter::value_type;
        using point_type = DatasetAdapter::point_type;
        using size_type = size_t;

        struct Node
        {
            // Union used because a node can be either a leaf or an internal node, so both data fields are never used simultaneously
            union
            {
                struct
                {
                    // Indices of points in leaf node
                    size_type left;
                    size_type right;
                } leaf;
                struct
                {
                    // Dimension and values used for subdivision.
                    size_type component;
                    value_type lower;
                    value_type upper;
                } internal;
            } data;

            // Both children null indicates a leaf node
            Node* child1;
            Node* child2;

            bool is_leaf() const { return child1 == nullptr && child2 == nullptr; };

            void set_leaf(size_type left, size_type right)
            {
                data.leaf.left = left;
                data.leaf.right = right;
                child1 = nullptr;
                child2 = nullptr;
            }

            void set_internal(size_type component, value_type lower, value_type upper)
            {
                data.internal.component = component;
                data.internal.lower = lower;
                data.internal.upper = upper;
            }
        };

        using NodePtr = Node*;

        NodePtr root_node;
        size_type m_leaf_size_max;
        size_type m_size;                // Number of current points in the dataset
        size_type m_size_at_index_build; // Number of points in the dataset when the index was built

        using BoundingBox = quetzal::geometry::AxisAlignedBoundingBox<quetzal::math::CartesianTraits<value_type, DatasetAdapter::dimension>>;

        BoundingBox root_bbox;

        //  Permutation of dataset vector keys
        std::vector<AccessorType> accessors;

        // Using a pooled memory allocator is more efficient than allocating memory directly when there is a large number small of memory allocations.
        PooledAllocator pool;

        // Returns number of points in dataset
        size_type size(const Derived& obj) const { return obj.m_size; }

        // Dataset access helpers
        value_type get_indirect(const Derived& obj, AccessorType accessor, size_type component) const
        {
            return obj.dataset.point_component(accessor, component);
        }

        point_type get_indirect(const Derived& obj, AccessorType accessor) const
        {
            return obj.dataset.point(accessor);
        }

point_type& get_indirect(Derived& obj, AccessorType accessor)
{
    return const_cast<DatasetAdapter&>(obj.dataset).point(accessor);
}

point_type& get_indirect(const DatasetAdapter& dataset, AccessorType accessor)
{
    return const_cast<DatasetAdapter&>(dataset).point(accessor);
}

        std::array<value_type, 2> computeMinMax(const Derived& obj, size_type start, size_type count, size_type component)
        {
            value_type valueMin = get_indirect(obj, accessors[start], component);
            value_type valueMax = valueMin;
            for (size_type i = 1; i < count; ++i)
            {
                value_type value = get_indirect(obj, accessors[start + i], component);
                if (value < valueMin)
                {
                    valueMin = value;
                }
                if (value > valueMax)
                {
                    valueMax = value;
                }
            }
            return {valueMin, valueMax};
        }

        // Create a tree node that recursively subdivides the list of points from accessors[left] to accessors[right]
        NodePtr divideTree(Derived& obj, const size_type left, const size_type right, BoundingBox& bbox)
        {
            NodePtr node = obj.pool.template allocate<Node>();
            // If too few exemplars remain, then make this a leaf node.
            if ((right - left) <= obj.m_leaf_size_max)
            {
                node->set_leaf(left, right);

                // compute bounding-box of leaf points
                bbox.clear();
//                bbox.insert({&get_indirect(obj, obj.accessors[left]), right - left}); // ...
                for (size_type k = left + 1; k < right; ++k)
                {
                    bbox.insert(get_indirect(obj, obj.accessors[k]));
                }
            }
            else
            {
                auto [component, value, idx] = middleSplit_(obj, left, right - left, bbox);

                BoundingBox left_bbox(bbox);
                left_bbox.upper()[component] = value;
                node->child1 = divideTree(obj, left, left + idx, left_bbox);

                BoundingBox right_bbox(bbox);
                right_bbox.lower()[component] = value;
                node->child2 = divideTree(obj, left + idx, right, right_bbox);

                node->set_internal(component, left_bbox.upper()[component], right_bbox.lower()[component]);

                bbox = op_union(left_bbox, right_bbox);
            }

            return node;
        }

        // Returns [component, value, key] at split
        std::tuple<size_type, value_type, size_type> middleSplit_(Derived& obj, size_type start, size_type count, const BoundingBox& bbox)
        {
//            auto extent = bbox.extent();
//            size_t nComponent = extent.longest_component();
//            value_type extentMax = extent[nComponent];

            //const auto EPS = static_cast<value_type>(0.00001);
            value_type spreadMax = -1;
            size_type component = 0;
            for (size_type i = 0; i < DatasetAdapter::dimension; ++i)
            {
//std::cout << "\t" << i << "\textent " << extent[i];
//                if (extent[i] > (1 - EPS) * extentMax)
                {
                    auto [valueMin, valueMax] = computeMinMax(obj, start, count, i);
                    value_type spread = valueMax - valueMin;
//std::cout << "\t" << valueMin << ".." << valueMax << "\tspread " << spread;
                    if (spread > spreadMax)
                    {
                        component = i;
                        spreadMax = spread;
                    }
                }
//std::cout << std::endl;
            }
//assert(component == nComponent);
//std::cout << "spreadMax " << spreadMax << " at " << component << "\t" << c << "\t" << (component == c ? "-" : "*") << std::endl;

            // Split on the middle value of component
            auto [valueMin, valueMax] = computeMinMax(obj, start, count, component);
            value_type value = (bbox.lower()[component] + bbox.upper()[component]) / 2;
            if (value < valueMin)
                value = valueMin;
            else if (value > valueMax)
                value = valueMax;

            auto [lim1, lim2] = planeSplit(obj, start, count, component, value);
            size_type key = count / 2;
            if (lim1 > count / 2)
                key = lim1;
            else if (lim2 < count / 2)
                key = lim2;
//std::cout << "key: " << key << "\t" << lim1 << "\t" << lim2 << "\t" << (count / 2) << std::endl;

size_t middle = start + count / 2;
point_type* first = &get_indirect(obj, start);
point_type* last = &get_indirect(obj, start) + count;
point_type* mid = &get_indirect(obj, middle);

std::nth_element(first, mid, last,
    [this, component](const point_type& lhs, const point_type& rhs) -> bool
    {
        return lhs[component] < rhs[component];
    });
//std::cout << "nth: " << start << ".." << middle << ".." << (start + count) << "\t" << obj.dataset.point_component(middle, component) << "\t" << value << "\t" << accessors[middle] << std::endl;

            return {component, value, key};
        }

        //  Subdivide the list of points by a plane perpendicular to axis corresponding to the 'component' dimension at 'value' position.
        //  Returns [lim1, lim2]
        //  dataset[ind[0..lim1-1]][component]<value
        //  dataset[ind[lim1..lim2-1]][component]==value
        //  dataset[ind[lim2..count]][component]>value
        std::array<size_type, 2> planeSplit(Derived& obj, size_type start, size_type count, size_type component, value_type value)
        {
            auto& dataset = obj.dataset;

            point_type* first = &get_indirect(dataset, start);
            point_type* last = &get_indirect(dataset, start) + count;
            auto m1 = std::partition(first, last,
                [component, value](const point_type& point) -> bool
                {
                    return point[component] < value;
                });
            size_type lim1 = m1 - first;

            auto m2 = std::partition(m1, last,
                [component, value](const point_type& point) -> bool
                {
                    return point[component] <= value;
                });
            size_type lim2 = m2 - first;

            return {lim1, lim2};
        }

        value_type computeInitialDistances(const Derived& obj, const point_type& point, point_type& dists) const
        {
            value_type distsq = value_type(0);

            for (size_type i = 0; i < DatasetAdapter::dimension; ++i)
            {
                if (point[i] < obj.root_bbox.lower()[i])
                {
                    dists[i] = obj.metric.accum_dist(point[i], obj.root_bbox.lower()[i], i);
                }
                else if (point[i] > obj.root_bbox.upper()[i])
                {
                    dists[i] = obj.metric.accum_dist(point[i], obj.root_bbox.upper()[i], i);
                }
                distsq += dists[i];
            }
            return distsq;
        }
    };

    //--------------------------------------------------------------------------
    // kd-tree static index
    // Contains the k-d trees and other information for indexing a set of points for nearest-neighbor matching.
    // DatasetAdapter Interface to the dataset that implements DatasetAdapter functionality
    // DistanceMetric The distance metric to use: Metric_L1, Metric_L2, Metric_L2_Simple, etc.
    // key_type Will be typically size_t or int
    template <typename DatasetAdapter, typename DistanceMetric, typename AccessorType = size_t>
    class KDTreeSingleIndexAdapter : public KDTreeBase<KDTreeSingleIndexAdapter<DatasetAdapter, DistanceMetric, AccessorType>, DatasetAdapter, DistanceMetric, AccessorType>
    {
    public:

        const DatasetAdapter& dataset; // Source dataset
        const KDTreeSingleIndexAdapterParams index_params;
        DistanceMetric metric;

        using Base = KDTreeBase<KDTreeSingleIndexAdapter<DatasetAdapter, DistanceMetric, AccessorType>, DatasetAdapter, DistanceMetric, AccessorType>;

        using value_type = Base::value_type;
        using point_type = Base::point_type;
        using size_type = Base::size_type;
        using BoundingBox = Base::BoundingBox;
        using Base::Node;
        using NodePtr = Base::NodePtr;

        using IndexedDistances = RadiusResultSet<value_type>::IndexedDistances;

        // inputData Dataset with the input features
        // params Basically, the maximum leaf node size
        KDTreeSingleIndexAdapter(const DatasetAdapter& inputData, const KDTreeSingleIndexAdapterParams& params = KDTreeSingleIndexAdapterParams()) :
            dataset(inputData),
            index_params(params),
            metric(inputData)
        {
            Base::root_node = nullptr;
            Base::m_size = dataset.point_count();
            Base::m_size_at_index_build = Base::m_size;
            Base::m_leaf_size_max = params.leaf_size_max;

            build_index();

            std::cout << "m_leaf_size_max\t" << Base::m_leaf_size_max << std::endl;
            std::cout << "size\t" << Base::m_size << std::endl;
            auto size = tree_size(Base::root_node);
            std::cout << "tree size\t" << size[0] << "\t" << size[1] << "\t" << size[2] << std::endl;
        }

        KDTreeSingleIndexAdapter(const KDTreeSingleIndexAdapter&) = delete;

        void build_index()
        {
//            Base::m_size = dataset.point_count();
//            Base::m_size_at_index_build = Base::m_size;
            init_accessors();
            this->free_index(*this);
            Base::m_size_at_index_build = Base::m_size;
            if (Base::m_size == 0)
                return;
            computeBoundingBox(Base::root_bbox);
            Base::root_node = this->divideTree(*this, 0, Base::m_size, Base::root_bbox); // construct the tree
        }

        // nodes, leaves, points
        std::array<size_t, 3> tree_size(const NodePtr pnode) const
        {
            assert(pnode != nullptr);
            if (pnode->is_leaf())
            {
                return {1, 1, pnode->data.leaf.right - pnode->data.leaf.left};
            }

            auto sizeLower = tree_size(pnode->child1);
            auto sizeUpper = tree_size(pnode->child2);
            return {1 + sizeLower[0] + sizeUpper[0], sizeLower[1] + sizeUpper[1], sizeLower[2] + sizeUpper[2]};
        }

        // Find set of nearest neighbors to point[0:dim-1]. Their indices are stored inside the result object.
        // result: the result object in which the indices of the nearest-neighbors are stored, RESULTSET should be any ResultSet<value_type>
        // point: the vector for which to search the nearest neighbors
        // return True if the requested neighbors could be found.
        template <typename RESULTSET>
        bool findNeighbors(RESULTSET& result, const point_type& point, const SearchParams& searchParams) const
        {
            assert(Base::root_node);

            if (this->size(*this) == 0)
                return false;

            float epsError = 1 + searchParams.eps;

            point_type dists = {0, 0, 0};
            value_type distsq = this->computeInitialDistances(*this, point, dists);
            searchLevel(result, point, Base::root_node, distsq, dists, epsError);
            return result.full();
        }

        // Find the "num_closest" nearest neighbors to the query_point[0:dim-1].
        // Their indices are stored inside the result object.
        // Return Count of valid points in the result set. Only the first count entries in `out_indices` and `out_distances_sq` will be valid.
        // May be less than `num_closest` only if the number of elements in the tree is less than `num_closest`.
        size_type knnSearch(const point_type& query_point, const size_type num_closest, AccessorType* out_indices, value_type* out_distances_sq) const
        {
            KNNResultSet<value_type, AccessorType> resultSet(num_closest);
            resultSet.init(out_indices, out_distances_sq);
            this->findNeighbors(resultSet, query_point, SearchParams());
            return resultSet.size();
        }

        // Find all the neighbors to query_point[0:dim-1] within a maximum radius.
        // The output is given as a vector of pairs, of which the first element is a
        // point index and the second the corresponding distance. Previous contents of indexedDistances are cleared.
        // If searchParams.sorted==true, the output list is sorted by ascending distances.
        // For a better performance, it is advisable to do a .reserve() on the vector if you have any wild guess about the number of expected matches.
        // return The number of points within the given radius (i.e. indices.size() or dists.size() )
        size_type radiusSearch(const point_type& query_point, const value_type& radius, IndexedDistances& indexedDistances, const SearchParams& searchParams) const
        {
            RadiusResultSet<value_type, AccessorType> resultSet(radius, indexedDistances);
            size_type nFound = radiusSearchCustomCallback(query_point, resultSet, searchParams);
            if (searchParams.sorted)
                std::sort(indexedDistances.begin(), indexedDistances.end(), IndexedDistanceLess());
            return nFound;
        }

        // Just like radiusSearch() but with a custom callback class for each point found in the radius of the query.
        // See the source of RadiusResultSet<> as a start point for your own classes.
        template <typename SEARCH_CALLBACK>
        size_type radiusSearchCustomCallback(const point_type& query_point, SEARCH_CALLBACK& resultSet, const SearchParams& searchParams = SearchParams()) const
        {
            this->findNeighbors(resultSet, query_point, searchParams);
            return resultSet.size();
        }

    public:

        // Set the permutation index to the identity permutation on the dataset
        void init_accessors()
        {
            Base::m_size = dataset.point_count();
            if (Base::accessors.size() != Base::m_size)
                Base::accessors.resize(Base::m_size);
            for (size_type i = 0; i < Base::m_size; ++i)
                Base::accessors[i] = i;
        }

        void computeBoundingBox(BoundingBox& bbox)
        {
            if (dataset.get_bbox(bbox))
            {
                return; // Done! It was implemented in derived class
            }

            size_type n = dataset.point_count();
            assert(n > 0);

            bbox.clear();
            for (size_type k = 0; k < n; ++k)
            {
                bbox.insert(this->get_indirect(*this, Base::accessors[k]));
            }
        }

        // Performs an exact search in the tree starting from a node.
        // RESULTSET Should be any ResultSet<value_type>
        // return true if the search should be continued, false if the results are sufficient
        template <typename RESULTSET>
        bool searchLevel(RESULTSET& result_set, const point_type& point, const NodePtr node, value_type mindistsq, point_type& dists, const float epsError) const
        {
            // If this is a leaf node, check and return.
            if (node->is_leaf())
            {
                value_type worst_dist = result_set.worstDist();
                for (size_type i = node->data.leaf.left; i < node->data.leaf.right; ++i)
                {
                    const AccessorType accessor = Base::accessors[i]; // reorder... : i;
                    value_type dist = metric.eval(point, accessor, DatasetAdapter::dimension);
                    if (dist < worst_dist)
                    {
                        if (!result_set.addPoint(dist, Base::accessors[i]))
                        {
                            // the resultset doesn't want to receive any more points, we're done searching!
                            return false;
                        }
                    }
                }
                return true;
            }

            // Which child branch should be taken first?
            size_type component = node->data.internal.component;
            value_type value = point[component];
            value_type diff1 = value - node->data.internal.lower;
            value_type diff2 = value - node->data.internal.upper;

            NodePtr bestChild;
            NodePtr otherChild;
            value_type cut_dist;
            if ((diff1 + diff2) < 0)
            {
                bestChild = node->child1;
                otherChild = node->child2;
                cut_dist = metric.accum_dist(value, node->data.internal.upper, component);
            }
            else
            {
                bestChild = node->child2;
                otherChild = node->child1;
                cut_dist = metric.accum_dist(value, node->data.internal.lower, component);
            }

            // Call recursively to search next level down.
            if (!searchLevel(result_set, point, bestChild, mindistsq, dists, epsError))
            {
                // the resultset doesn't want to receive any more points, we're done searching!
                return false;
            }

            value_type dst = dists[component];
            mindistsq = mindistsq + cut_dist - dst;
            dists[component] = cut_dist;
            if (mindistsq * epsError <= result_set.worstDist())
            {
                if (!searchLevel(result_set, point, otherChild, mindistsq, dists, epsError))
                {
                    // the resultset doesn't want to receive any more points, we're done searching!
                    return false;
                }
            }
            dists[component] = dst;
            return true;
        }
    };

} // namespace quetzal::spatial_search

#endif // QUETZAL_SPATIAL_SEARCH_NKD_HPP
