//------------------------------------------------------------------------------
// spatial_search_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/common/Timestamp.hpp"
#include "quetzal/math/CartesianTraits.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/geometry/AxisAlignedBoundingBox.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/spatial_search/nkd.hpp"
#include "quetzal/spatial_search/KdTree.hpp"
#include "quetzal/spatial_search/RTree.hpp"
#include <iostream>
#include <unordered_map>
#include <ctime>
#include <cstdlib>
#include <conio.h>

using namespace std;
using namespace quetzal;

namespace
{

    //--------------------------------------------------------------------------
    size_t tree_nodes(size_t breadth)
    {
        if (breadth == 0)
        {
            return 0;
        }

        size_t n = 0;
        for (size_t full = breadth * 2 - 1; full > 0; full /= 2)
        {
            ++n;
        }

        return n;
    }

/*
    //--------------------------------------------------------------------------
    bool intersection(ray r, box b)
    {
        double t1 = (b.min[0] - r.origin[0]) * r.dir_inv[0];
        double t2 = (b.max[0] - r.origin[0]) * r.dir_inv[0];

        double tmin = min(t1, t2);
        double tmax = max(t1, t2);

        for (int i = 1; i < 3; ++i)
        {
            t1 = (b.min[i] - r.origin[i]) * r.dir_inv[i];
            t2 = (b.max[i] - r.origin[i]) * r.dir_inv[i];

            tmin = max(tmin, min(min(t1, t2), tmax));
            tmax = min(tmax, max(max(t1, t2), tmin));
        }

        return tmax > max(tmin, 0.0);
    }
*/

} // namespace
/*
//------------------------------------------------------------------------------
template<typename Traits>
struct PrimitiveTraits
{
    using point_type = ...;
    using primitive_type = ...;
    using bounding_box_type = ...;

    point_type reference_point(const primitive_type& primitive);
    bounding_box_type bounding_box(const primitive_type& primitive);
};
*/
//------------------------------------------------------------------------------
template<typename BoundingBox, typename Primitives>
class BoundingBoxes
{
public:
    using bounding_box_type = BoundingBox;
    using value_type = bounding_box_type::value_type;
    using primitives_type = Primitives;
    using primitive_type = primitives_type::value_type; // sphere center ...

    BoundingBoxes(const primitives_type& primitives) :
        m_primitives(primitives),
        m_boundingBoxes()
    {
    }

    bounding_box_type& operator[](size_t primitiveId)
    {
        typename bounding_box_type::vector_type offset{value_type(1), value_type(1), value_type(1)};
        if (!m_boundingBoxes.contains(primitiveId))
        {
            auto& primitive = m_primitives[primitiveId];
            bounding_box_type boundingBox(primitive - offset, primitive + offset);
            m_boundingBoxes.insert(make_pair(primitiveId, boundingBox));
        }
        return m_boundingBoxes[primitiveId];
    }

private:
    const primitives_type& m_primitives;
    unordered_map<size_t, bounding_box_type> m_boundingBoxes;
};

//------------------------------------------------------------------------------
template<typename Point, typename Primitives>
class ReferencePoints
{
public:
    using point_type = Point;
    using primitives_type = Primitives;
    using primitive_type = primitives_type::value_type; // sphere center ...

    ReferencePoints(const primitives_type& primitives) :
        m_primitives(primitives),
        m_points()
    {
    }

    point_type& operator[](size_t primitiveId)
    {
        if (!m_points.contains(primitiveId))
        {
            const primitive_type& primitive = m_primitives[primitiveId];
            const point_type& point = primitive;
            m_points.insert(make_pair(primitiveId, point));
        }
        return m_points[primitiveId];
    }

private:
    const primitives_type& m_primitives;
    unordered_map<size_t, point_type> m_points;
};

//------------------------------------------------------------------------------
template<typename Traits>
struct RTreeTraits
{
    using value_type = Traits::value_type;
    using point_type = geometry::Point<Traits>;
    using bounding_box_type = geometry::AxisAlignedBoundingBox<Traits>;
    using primitive_type = point_type; // for initial testing, an implied unit sphere centered at this point ...
    using primitives_type = vector<primitive_type>;

    using bounding_boxes_type = BoundingBoxes<bounding_box_type, primitives_type>;
    using reference_points_type = ReferencePoints<point_type, primitives_type>;
};

//------------------------------------------------------------------------------
template<typename Traits>
struct KdTreeTraits
{
    using value_type = Traits::value_type;
    using point_type = geometry::Point<Traits>;
    using bounding_box_type = geometry::AxisAlignedBoundingBox<Traits>;

    struct extended_point_type : public point_type
    {
        size_t id;
    };

    using extended_points_type = vector<extended_point_type>;

    static point_type point_accessor(const extended_point_type& point)
    {
        return static_cast<point_type>(point);
    }

    static point_type point_accessor(const extended_points_type& points, size_t i)
    {
        return point_accessor(points[i]);
    }
};

//------------------------------------------------------------------------------
template<typename P>
struct PointCloud
{
    // no easy way to handle both std::array and Point (or array/pointer at all)
    // pointer would require adding value type to template parameters

    using point_type = P;
    using value_type = P::value_type;
    using size_type = size_t;
    static constexpr size_t dimension = P::dimension;

    std::vector<point_type> m_points;

    // The number of data points
    size_type point_count() const { return m_points.size(); }

    // The i'th point in the set
    point_type point(size_t i) const
    {
        return m_points[i];
    }

// The i'th point in the set
point_type& point(size_t i)
{
    return m_points[i];
}

    // The n'th component of the i'th point in the set
    value_type point_component(size_t i, size_t n) const
    {
        return m_points[i][n];
    }

    // Optional bounding-box computation
    // Return false to default to standard bbox computation
    // Return true if the BBOX was already computed and returned in "bb" so recalculation can be avoided
    template<class BBOX>
    bool get_bbox(BBOX&) const { return false; }
};

//------------------------------------------------------------------------------
template<typename P>
void generateRandomPointCloud(PointCloud<P>& points, size_t n, typename P::value_type max_range = 10)
{
    points.m_points = random_points<P>(n, max_range);
}

//------------------------------------------------------------------------------
template<typename P>
vector<P> random_points(size_t n, typename P::value_type max_range = 10)
{
    using T = typename P::value_type;

    vector<P> points(n);
    for (size_t i = 0; i < n; ++i)
    {
        for (size_t j = 0; j < P::dimension; ++j)
        {
            points[i][j] = max_range * (rand() % 1000) / T(1000);
        }
    }

    return points;
}

//------------------------------------------------------------------------------
template<typename P>
vector<P> random_extended_points(size_t n, typename P::value_type max_range = 10)
{
    using T = typename P::value_type;

    vector<P> points(n);
    for (size_t i = 0; i < n; ++i)
    {
        auto& point = points[i];

        for (size_t j = 0; j < P::dimension; ++j)
        {
            point[j] = max_range * (rand() % 1000) / T(1000);
        }

        point.id = i;
    }

    return points;
}

//------------------------------------------------------------------------------
template<typename T>
void nkdtree_test(size_t n)
{
//    using point_type = array<T, 3>;
    using point_type = quetzal::geometry::Point<quetzal::math::CartesianTraits<T, 3>>;
    using points_type = PointCloud<point_type>;
    points_type cloud;
    generateRandomPointCloud(cloud, n);

    typename points_type::point_type query_pt = { 0.5, 0.5, 0.5 };

    Timestamp t0;
    using kdtree_type = quetzal::spatial_search::KDTreeSingleIndexAdapter<points_type, quetzal::spatial_search::L2_Simple_Adapter<points_type>>;
    kdtree_type index(cloud, quetzal::spatial_search::KDTreeSingleIndexAdapterParams(10));
//    kdtree_type index(cloud);
    index.build_index();
    Timestamp t1;
    cout << "nkdtree" << endl;
    cout << "build\t" << (t1 - t0) << endl;

    {
        // knn search
        Timestamp t2;
        const size_t nResults = 5;
        vector<pair<size_t, T>> indexedDistances(nResults);
        quetzal::spatial_search::KNNResultSet<T> resultSet(nResults, indexedDistances);
        index.findNeighbors(resultSet, query_pt, quetzal::spatial_search::SearchParams(10));
        Timestamp t3;

        cout << "knnSearch " << nResults << " results: \n";
        cout << (t3 - t2) << " seconds" << endl;
//        for (size_t i = 0; i < nResults; ++i)
//        {
//            cout << i << ": key=" << indexedDistances[i].first << " distance_squared=" << indexedDistances[i].second << endl;
//        }
    }
    {
        // Unsorted radius search:
        Timestamp t4;
        const T radius = 1;
        vector<pair<size_t, T>> indexedDistances;
        quetzal::spatial_search::RadiusResultSet<T, size_t> resultSet(radius, indexedDistances);
        index.findNeighbors(resultSet, query_pt, quetzal::spatial_search::SearchParams());
        Timestamp t5;

        // Get worst (furthest) point, without sorting:
        pair<size_t, T> worst_pair = resultSet.worst_item();
        cout << "unsorted radius search" << endl;
        cout << (t5 - t4) << " seconds" << endl;
        cout << "Worst pair: idx=" << worst_pair.first << " dist=" << worst_pair.second << endl;
    }
}

//------------------------------------------------------------------------------
template<typename T>
void rtree_test(size_t nPrimitives)
{
//    using point_type = array<T, 3>;
//    using traits_type = quetzal::math::CartesianTraits<T, 3>;
    using traits_type = quetzal::math::HomogeneousTraits<T, 3>;
    using point_type = quetzal::geometry::Point<traits_type>;
    using points_type = vector<point_type>;
    points_type primitives = random_points<point_type>(nPrimitives);

    using rtree_traits_type = RTreeTraits<traits_type>;
    using rtree_type = quetzal::spatial_search::RTree<rtree_traits_type>;

    Timestamp t0;
    rtree_type rtree(primitives);
    rtree.build();
//    cout << rtree;
    Timestamp t1;
    cout << "RTree" << endl;
    cout << "build\t" << (t1 - t0) << " seconds" << endl;

    point_type query = {0.5, 0.5, 0.5};

    cout << "point-sphere intersection" << endl;
    cout << "count [" << query << "]" << endl;
    Timestamp t2;
    size_t nIntersections = rtree.intersection_count(query);
    Timestamp t3;
    cout << nIntersections << " intersections" << endl;
    cout << (t3 - t2) << " seconds" << endl;

    Timestamp t4;
    size_t nBrute = 0;
    for (size_t i = 0; i < nPrimitives; ++i)
    {
        T dsquared = (query - primitives[i]).norm_squared();
        if (dsquared <= T(1)) // float_le? ...
        {
//            cout << i << endl;
            ++nBrute;
        }
    }
    Timestamp t5;
    cout << nBrute << " brute intersections" << endl;
    cout << (t5 - t4) << " seconds" << endl;

    Timestamp t6;
    optional<size_t> nIntersectionsStrict = rtree.interior_intersection_count(query);
    Timestamp t7;
    cout << "interior count [" << query << "]" << endl;
    cout << (t7 - t6) << " seconds" << endl;
    if (nIntersectionsStrict)
    {
        cout << nIntersectionsStrict.value() << " intersections" << endl;
    }
    else
    {
        cout << "edge intersection" << endl;
    }

    return;
}

//------------------------------------------------------------------------------
template<typename T>
void kdtree_test(size_t n)
{
//    using traits_type = math::CartesianTraits<T, 3>;
    using traits_type = quetzal::math::HomogeneousTraits<T, 3>;
    using point_type = geometry::Point<traits_type>;
    using kdtree_traits_type = KdTreeTraits<traits_type>;
    using kdtree_type = quetzal::spatial_search::KdTree<kdtree_traits_type>;
    using extended_point_type = kdtree_type::extended_point_type;

    auto points = random_extended_points<extended_point_type>(n);

    Timestamp t0;
    kdtree_type kdtree(points);
    kdtree.build();
//cout << kdtree;
    Timestamp t1;
    cout << "KDTree" << endl;
    cout << "build\t" << (t1 - t0) << " seconds" << endl;

    point_type query = {0.5, 0.5, 0.5};

    // k-nearest neighbors search

    Timestamp t2;
    // knn KdTree
    auto results = kdtree.search_knn(query, 10);
    Timestamp t3;
    cout << "search_knn" << endl;
    cout << (t3 - t2) << " seconds" << endl;
//    for (const auto& result : results)
//    {
//        cout << result.id << "\t" << static_cast<point_type>(result) << endl;
//    }

    // knn brute force
    Timestamp t4;
    map<T, extended_point_type> m;
    for (const auto& point : points)
    {
        T d2 = (point - query).norm_squared();
        if (m.size() < 10)
        {
            m.insert({d2, point});
            continue;
        }

        if (d2 < m.rbegin()->first)
        {
            m.erase(prev(m.end()));
            m.insert({d2, point});
        }
    }
    Timestamp t5;

    cout << "brute knn" << endl;
    cout << (t5 - t4) << " seconds" << endl;
//    for (const auto& result : m)
//    {
//        cout << result.second.id << "\t" << result.first << "\t" << static_cast<point_type>(result.second) << endl;
//    }

    // Radius search

    Timestamp t6;
    // radius KDTree
    results = kdtree.search_radius(query, T(1));
    Timestamp t7;
    cout << "search_radius" << endl;
    cout << (t7 - t6) << " seconds" << endl;
    m.clear();
    for (const auto& result : results)
    {
        m.insert({(result - query).norm_squared(), result});
    }
//    for (const auto& result : m)
//    {
//        cout << result.second.id << "\t" << result.first << "\t" << static_cast<point_type>(result.second) << endl;
//    }

    // radius brute force
    Timestamp t8;
    m.clear();
    for (const auto& point : points)
    {
        T d2 = (point - query).norm_squared();
        if (d2 <= T(1))
        {
            m.insert({d2, point});
        }
    }
    Timestamp t9;

    cout << "brute radius" << endl;
    cout << (t9 - t8) << " seconds" << endl;
//    for (const auto& result : m)
//    {
//        cout << result.second.id << "\t" << result.first << "\t" << static_cast<point_type>(result.second) << endl;
//    }

    return;
}

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    srand(static_cast<unsigned int>(time(nullptr)));

//    nkdtree_test<float>(1000000);
//    nkdtree_test<double>(1000000);
    nkdtree_test<double>(100000);

    kdtree_test<float>(100000);

//    rtree_test<float>(1000000);
//    rtree_test<double>(1000000);
    rtree_test<float>(100000);

    (void)_getch();
    return 0;
}
