//------------------------------------------------------------------------------
// geometry_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/geometry/Intersection.hpp"
#include "quetzal/geometry/Line.hpp"
#include "quetzal/geometry/Plane.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/Polygon.hpp"
#include "quetzal/geometry/Ray.hpp"
#include "quetzal/geometry/Segment.hpp"
#include "quetzal/geometry/distance.hpp"
#include "quetzal/geometry/intersect.hpp"
#include "quetzal/geometry/relationships.hpp"
#include "quetzal/geometry/triangle_util.hpp"
#include "quetzal/math/CartesianTraits.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/math/Vector.hpp"
#include "quetzal/math/floating_point.hpp"
#include <iostream>
#include <random>

#include <array>
#include <vector>
#include <cassert>

using namespace std;
using namespace quetzal;

namespace
{

    //--------------------------------------------------------------------------
    template<typename Traits>
    typename Traits::value_type random_value(typename Traits::value_type dimension_extent)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<typename Traits::value_type> distribution(-dimension_extent, dimension_extent);
        return distribution(gen);
    }

    //--------------------------------------------------------------------------
    template<typename Traits>
    geometry::Vector<Traits> random_vector(typename Traits::value_type dimension_extent)
    {
        geometry::Vector<Traits> v;
        for (size_t i = 0; i < Traits::dimension; ++i)
        {
            v[i] = random_value<Traits>(Traits::val(dimension_extent));
        }
        return v;
    }

    //--------------------------------------------------------------------------
    template<typename Traits>
    geometry::Vector<Traits> random_unit_vector()
    {
        return normalize(random_vector<Traits>(Traits::val(10)));
    }

    //--------------------------------------------------------------------------
    template<typename Traits>
    geometry::Point<Traits> random_position(typename Traits::value_type dimension_extent)
    {
        return {random_vector<Traits>(dimension_extent)};
    }

    //--------------------------------------------------------------------------
    template<typename Traits>
    geometry::Plane<Traits> random_plane(typename Traits::value_type dimension_extent)
    {
        return geometry::Plane<Traits>(random_position<Traits>(dimension_extent), random_unit_vector<Traits>());
    }

    //--------------------------------------------------------------------------
    template<typename Traits>
    void test_extent()
    {
        // distance, area ...

        return;
    }

    //--------------------------------------------------------------------------
    template<typename Traits>
    void test_parallel()
    {
        geometry::Line<Traits> line;
        geometry::Ray<Traits> ray;
        geometry::Segment<Traits> segment;
        geometry::Plane<Traits> plane;
        geometry::Plane<Traits> polygon;

        assert(!geometry::parallel(line, segment));
        assert(!geometry::parallel(ray, polygon));
        assert(!geometry::parallel(plane, line));
        assert(!geometry::parallel(polygon, plane));

        return;
    }

    //--------------------------------------------------------------------------
    template<typename Traits>
    void test_perpendicular()
    {
        geometry::Line<Traits> line;
        geometry::Ray<Traits> ray;
        geometry::Segment<Traits> segment;
        geometry::Plane<Traits> plane;
        geometry::Plane<Traits> polygon;

        assert(!geometry::perpendicular(line, segment));
        assert(!geometry::perpendicular(ray, polygon));
        assert(!geometry::perpendicular(plane, line));
        assert(!geometry::perpendicular(polygon, plane));

        return;
    }

    //--------------------------------------------------------------------------
    template<typename Traits>
    void test_intersects()
    {
        // ...

        using T = Traits::value_type;

        for (size_t i = 0; i < 100; ++i)
        {
            auto a = random_plane<Traits>(T(25));
            auto b = random_plane<Traits>(T(25));
            auto ix = intersection(a, b);
            std::cout << std::format("a [{}]\tb [{}]\t ix {}", a, b, ix);
        }

        return;
    }

    //--------------------------------------------------------------------------
    template<typename Traits>
    void test_intersection()
    {
        // ...

        return;
    }

    //--------------------------------------------------------------------------
    template<typename Traits>
    void test_triangle()
    {
        // ...

        return;
    }

    //--------------------------------------------------------------------------
    template<typename Traits>
    void test2()
    {
        test_extent<Traits>();
//        test_parallel<Traits>();
//        test_perpendicular<Traits>();
//        test_intersects<Traits>();
//        test_intersection<Traits>();
        test_triangle<Traits>();
        return;
    }

    //--------------------------------------------------------------------------
    template<typename Traits>
    void test3()
    {
        test_extent<Traits>();
//        test_parallel<Traits>();
//        test_perpendicular<Traits>();
        test_intersects<Traits>();
        test_intersection<Traits>();
        test_triangle<Traits>();
        return;
    }

}

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    test2<math::CartesianTraits<double, 2>>();
    test2<math::CartesianTraits<float, 2>>();
    test2<math::HomogeneousTraits<double, 2>>();
    test2<math::HomogeneousTraits<float, 2>>();

    test3<math::CartesianTraits<double, 3>>();
    test3<math::CartesianTraits<float, 3>>();
    test3<math::HomogeneousTraits<double, 3>>();
    test3<math::HomogeneousTraits<float, 3>>();

    // original ...

    using value_type = float;
    using vector3_traits = math::HomogeneousTraits<value_type, 3>;
    using vector3_type = math::Vector<vector3_traits>;
    using point3_type = geometry::Point<vector3_traits>;
    using vector2_traits = math::HomogeneousTraits<value_type, 2>;
    using vector2_type = math::Vector<vector2_traits>;
    using point2_type = geometry::Point<vector2_traits>;
//    using matrix_type = math::Matrix<value_type>;

    std::array<point3_type, 3> t3s
    {
        point3_type{-0.5f, -0.5f, 0.5f},
        {1.5f, -0.5f, 0.5f},
        {-0.5f, 1.5f, 0.5f},
    };

    std::array<point2_type, 3> t2s =
    {
        point2_type{-0.5f, -0.5f},
        {{1.5f, -0.5f}},
        {{-0.5f, 1.5f}},
    };

    std::array<point3_type, 6> v3s =
    {
        point3_type{{-0.5f, -0.5f, 0.5f}},
        {{1.5f, -0.5f, 0.5f}},
        {{1.5f, 0.5f, 0.5f}},
        {{0.5f, 0.5f, 0.5f}},
        {{0.5f, 1.5f, 0.5f}},
        {{-0.5f, 1.5f, 0.5f}},
    };

    std::array<point2_type, 6> v2s =
    {
        point2_type{{-0.5f, -0.5f}},
        {{1.5f, -0.5f}},
        {{1.5f, 0.5f}},
        {{0.5f, 0.5f}},
        {{0.5f, 1.5f}},
        {{-0.5f, 1.5f}},
    };

    bool b;
    value_type a;

    b = geometry::triangle_ccw(t2s[0], t2s[1], t2s[2]);
    assert(b);

    b = geometry::triangle_contains(t3s[0], t3s[1], t3s[2], {0.0f, 0.0f, 0.5f});
    assert(b);

    b = geometry::triangle_contains(t3s[0], t3s[1], t3s[2], {1.0f, 1.0f, 0.5f});
    assert(!b);

    auto normal_z = vector3_type::z_direction();

    a = geometry::triangle_area(t3s[0], t3s[1], t3s[2]);
    assert(math::float_eq(a, 2.0f));
    a = geometry::triangle_area(t3s[0], t3s[1], t3s[2]);
    assert(math::float_eq(a, 2.0f));

    geometry::Polygon<vector3_traits> p3(v3s);
    geometry::Polygon<vector2_traits> p2(v2s);

    std::reverse(v3s.begin(), v3s.end());
    std::reverse(v2s.begin(), v2s.end());

    geometry::Polygon<vector3_traits> p3r(v3s);
    geometry::Polygon<vector2_traits> p2r(v2s);

    a = p3.area(normal_z);
    assert(math::float_eq(a, 3.0f));
    a = p3.signed_area(normal_z);
    assert(math::float_eq(a, 3.0f));
    a = p2.area();
    assert(math::float_eq(a, 3.0f));
    a = p2.signed_area();
    assert(math::float_eq(a, 3.0f));

    a = p3r.area(normal_z);
    assert(math::float_eq(a, 3.0f));
    a = p3r.signed_area(normal_z);
    assert(math::float_eq(a, -3.0f));
    a = p2r.area();
    assert(math::float_eq(a, 3.0f));
    a = p2r.signed_area();
    assert(math::float_eq(a, -3.0f));

    return 0;
}
