//------------------------------------------------------------------------------
// geometry_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/common/string_util.hpp"
#include "quetzal/geometry/AxisAlignedBoundingBox.hpp"
#include "quetzal/geometry/BoundingSphere.hpp"
#include "quetzal/geometry/Box.hpp"
#include "quetzal/geometry/Circle.hpp"
#include "quetzal/geometry/HalfPlane.hpp"
#include "quetzal/geometry/HalfSpace.hpp"
#include "quetzal/geometry/Intersection.hpp"
#include "quetzal/geometry/Line.hpp"
#include "quetzal/geometry/Orientation.hpp"
#include "quetzal/geometry/OrientedPoint.hpp"
#include "quetzal/geometry/Plane.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/Polygon.hpp"
#include "quetzal/geometry/Ray.hpp"
#include "quetzal/geometry/Segment.hpp"
#include "quetzal/geometry/Sphere.hpp"
#include "quetzal/geometry/distance.hpp"
#include "quetzal/geometry/intersect.hpp"
#include "quetzal/geometry/relationships.hpp"
#include "quetzal/geometry/triangle_util.hpp"
#include "quetzal/math/CartesianTraits.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/math/Vector.hpp"
#include "quetzal/math/floating_point.hpp"
#include "quetzal/physics/gjk.hpp"
#include <iomanip>
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
    math::Vector<Traits> random_vector(typename Traits::value_type dimension_extent)
    {
        math::Vector<Traits> v;
        for (size_t i = 0; i < Traits::dimension; ++i)
        {
            v[i] = random_value<Traits>(Traits::val(dimension_extent));
        }
        return v;
    }

    //--------------------------------------------------------------------------
    template<typename Traits>
    math::Vector<Traits> random_unit_vector()
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

    size_t n_cases = 0;
    size_t n_failures = 0;

    //--------------------------------------------------------------------------
    void check(const std::string& name, bool b)
    {
        ++n_cases;
        if (!b)
        {
            ++n_failures;
        }

        std::cout << (b ? "pass  " : "FAIL  ") << name << std::endl;
        return;
    }

    //--------------------------------------------------------------------------
    // The third component is dropped in 2D, so a caller can name all three and let the dimension decide
    template<typename Traits>
    geometry::Point<Traits> make_point(typename Traits::value_type x, typename Traits::value_type y, typename Traits::value_type z)
    {
        if constexpr (Traits::dimension == 2)
        {
            return {x, y};
        }
        else
        {
            return {x, y, z};
        }
    }

    //--------------------------------------------------------------------------
    template<typename Traits>
    std::string traits_name()
    {
        return std::format("dim {} {}", Traits::dimension, sizeof(typename Traits::value_type) == 4 ? "float " : "double");
    }

    //--------------------------------------------------------------------------
    template<typename Traits>
    geometry::Point<Traits> point_on_line(typename Traits::value_type t, typename Traits::value_type scale)
    {
        using T = Traits::value_type;

        // Deliberately not representable in binary, so that the products forming the predicate do not
        // cancel exactly and the test sees real rounding rather than an artificially clean zero
        return make_point<Traits>((T(0.1) + t * T(1)) * scale, (T(0.2) + t * T(3)) * scale, (T(0.3) + t * T(2)) * scale);
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
        using T = Traits::value_type;
        using vector_type = geometry::Point<Traits>::vector_type;

        auto origin = make_point<Traits>(T(0), T(0), T(0));
        std::string d = traits_name<Traits>();

        geometry::Line<Traits> line_x(origin, vector_type::x_direction());
        geometry::Line<Traits> line_y(origin, vector_type::y_direction());
        geometry::Segment<Traits> segment_x(make_point<Traits>(T(0), T(1), T(0)), make_point<Traits>(T(1), T(1), T(0)));

        check(d + " parallel: line and segment along x", geometry::parallel(line_x, segment_x));
        check(d + " parallel: lines along x and y are not", !geometry::parallel(line_x, line_y));

        // The plane overloads are only reachable where normal() is, which Plane declares for 3D and up
        if constexpr (Traits::dimension >= 3)
        {
            geometry::Plane<Traits> plane_xy(origin, vector_type::z_direction());
            geometry::Plane<Traits> plane_yz(origin, vector_type::x_direction());

            check(d + " parallel: line lies parallel to plane", geometry::parallel(line_x, plane_xy));
            check(d + " parallel: line crossing plane is not", !geometry::parallel(line_x, plane_yz));
            check(d + " parallel: plane with itself", geometry::parallel(plane_xy, plane_xy));
            check(d + " parallel: perpendicular planes are not", !geometry::parallel(plane_xy, plane_yz));
        }

        return;
    }

    //--------------------------------------------------------------------------
    template<typename Traits>
    void test_perpendicular()
    {
        using T = Traits::value_type;
        using vector_type = geometry::Point<Traits>::vector_type;

        auto origin = make_point<Traits>(T(0), T(0), T(0));
        std::string d = traits_name<Traits>();

        geometry::Line<Traits> line_x(origin, vector_type::x_direction());
        geometry::Line<Traits> line_y(origin, vector_type::y_direction());

        // The directional pair also requires that they meet, so this is perpendicular rather than skew
        check(d + " perpendicular: lines along x and y meeting at the origin", geometry::perpendicular(line_x, line_y));
        check(d + " perpendicular: line with itself is not", !geometry::perpendicular(line_x, line_x));

        if constexpr (Traits::dimension >= 3)
        {
            geometry::Line<Traits> line_z(origin, vector_type::z_direction());
            geometry::Plane<Traits> plane_xy(origin, vector_type::z_direction());
            geometry::Plane<Traits> plane_yz(origin, vector_type::x_direction());

            check(d + " perpendicular: line along the plane normal", geometry::perpendicular(line_z, plane_xy));
            check(d + " perpendicular: line lying in the plane is not", !geometry::perpendicular(line_x, plane_xy));
            check(d + " perpendicular: planes with orthogonal normals", geometry::perpendicular(plane_xy, plane_yz));
            check(d + " perpendicular: plane with itself is not", !geometry::perpendicular(plane_xy, plane_xy));
        }

        return;
    }

    //--------------------------------------------------------------------------
    // Whether three points are colinear cannot depend on how far from the origin they sit. The
    // predicate compares a product of two edge vectors, so its rounding error grows with the square of
    // the coordinates while a fixed tolerance does not, and past some scale genuinely colinear points
    // start reading as not colinear
    template<typename Traits>
    void test_colinear()
    {
        using T = Traits::value_type;

        for (T scale : {T(1), T(10), T(100), T(1000), T(10000)})
        {
            auto a = point_on_line<Traits>(T(0.3), scale);
            auto b = point_on_line<Traits>(T(1.1), scale);
            auto c = point_on_line<Traits>(T(2.7), scale);

            bool exact = geometry::colinear(a, b, c);

            // Displaced perpendicular to the line by a thousandth of the scale, well clear of any
            // rounding, so this has to read as not colinear at every scale
            auto d = c;
            d.set_y(d.y() + scale / T(1000));
            bool displaced = geometry::colinear(a, b, d);

            check(std::format("{} colinear: colinear at scale {}", traits_name<Traits>(), scale), exact);
            check(std::format("{} colinear: displaced at scale {}", traits_name<Traits>(), scale), !displaced);
        }

        return;
    }

    //--------------------------------------------------------------------------
    template<typename Traits>
    void test_intersects()
    {
        // ...

        using T = Traits::value_type;

        // Plane intersection needs normal(), which is 3D and up, and Intersection holds a Polygon whose
        // containment test needs Projection, which exists only for homogeneous traits
        if constexpr (math::Homogeneous<Traits> && Traits::dimension >= 3)
        {
            for (size_t i = 0; i < 100; ++i)
            {
                auto a = random_plane<Traits>(T(25));
                auto b = random_plane<Traits>(T(25));
                auto ix = intersection(a, b);
                std::cout << std::format("a [{}]\tb [{}]\t ix {}", a, b, ix);
            }
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
        test_parallel<Traits>();
        test_perpendicular<Traits>();
        test_intersects<Traits>();
        test_intersection<Traits>();
        test_colinear<Traits>();
        test_triangle<Traits>();
        return;
    }

    //--------------------------------------------------------------------------
    template<typename Traits>
    void test3()
    {
        test_extent<Traits>();
        test_parallel<Traits>();
        test_perpendicular<Traits>();
        if constexpr (math::Homogeneous<Traits>)
        {
            // Intersection<Traits> holds a Polygon<Traits> member, and Polygon's containment
            // test relies on Projection<Traits>, which only exists for homogeneous traits
            test_intersects<Traits>();
            test_intersection<Traits>();
        }
        test_colinear<Traits>();
        test_triangle<Traits>();
        return;
    }

}

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    math::Vector<math::CartesianTraits<double, 3>> smoke_v(1.5, 2.25, 3.0);
    std::cout << std::format("default [{}]", smoke_v) << std::endl;
    std::cout << std::format("brackets [{:[}]", smoke_v) << std::endl;
    std::cout << std::format("parens [{:(}]", smoke_v) << std::endl;
    std::cout << std::format("comma-space [{:[,_}]", smoke_v) << std::endl;
    std::cout << std::format("space-colon-space [{:_:_}]", smoke_v) << std::endl;
    std::cout << std::format("semicolon [{:;}]", smoke_v) << std::endl;
    std::cout << std::format("combo [{:(,_.2f}]", smoke_v) << std::endl;

    math::Vector<math::HomogeneousTraits<float, 3>> smoke_h(4.5f, 5.25f, 6.0f);
    std::cout << std::format("homogeneous combo [{:[,_.1f}]", smoke_h) << std::endl;

    std::cout << "to_string " << to_string(smoke_v) << std::endl;
    std::cout << "to_array_string " << to_array_string(smoke_v) << std::endl;

    geometry::Point<math::CartesianTraits<double, 3>> smoke_p(1.5, 2.25, 3.0);
    std::cout << std::format("point default [{}]", smoke_p) << std::endl;
    std::cout << std::format("point combo [{:(,_.2f}]", smoke_p) << std::endl;
    std::cout << "point to_string " << to_string(smoke_p) << std::endl;
    std::cout << "point to_array_string " << to_array_string(smoke_p) << std::endl;

    std::cout << "point operator<< default [" << smoke_p << "]" << std::endl;
    std::cout << "point operator<< fixed(2) [" << std::fixed << std::setprecision(2) << smoke_p << "]" << std::endl;
    std::cout << "point operator<< scientific(3) [" << std::scientific << std::setprecision(3) << smoke_p << "]" << std::endl;
    std::cout.unsetf(std::ios::fixed | std::ios::scientific);
    std::cout << std::setprecision(6);

    geometry::Segment<math::CartesianTraits<double, 3>> smoke_s(smoke_p, geometry::Point<math::CartesianTraits<double, 3>>(4.0, 5.0, 6.0));
    std::cout << std::format("segment default [{}]", smoke_s) << std::endl;
    std::cout << std::format("segment combo [{:[,_.2f}]", smoke_s) << std::endl;

    geometry::Line<math::CartesianTraits<double, 3>> smoke_l(smoke_p, math::Vector<math::CartesianTraits<double, 3>>::z_direction());
    std::cout << std::format("line default [{}]", smoke_l) << std::endl;
    std::cout << std::format("line combo [{:[,_.2f}]", smoke_l) << std::endl;

    geometry::Plane<math::CartesianTraits<double, 3>> smoke_pl(smoke_p, math::Vector<math::CartesianTraits<double, 3>>::z_direction());
    std::cout << std::format("plane default [{}]", smoke_pl) << std::endl;
    std::cout << std::format("plane combo [{:[,_.2f}]", smoke_pl) << std::endl;

    // Polygon (and anything holding one, like Intersection) needs Homogeneous traits -- see the
    // Cartesian/Projection incompatibility discussed earlier.
    geometry::Polygon<math::HomogeneousTraits<double, 3>> smoke_poly
    {
        geometry::Point<math::HomogeneousTraits<double, 3>>(0.0, 0.0, 0.0),
        geometry::Point<math::HomogeneousTraits<double, 3>>(1.0, 0.0, 0.0),
        geometry::Point<math::HomogeneousTraits<double, 3>>(1.0, 1.0, 0.0),
    };
    std::cout << std::format("polygon default [{}]", smoke_poly) << std::endl;
    std::cout << std::format("polygon combo [{:[,_.1f}]", smoke_poly) << std::endl;

    geometry::Intersection<math::HomogeneousTraits<double, 3>> smoke_ix(geometry::Point<math::HomogeneousTraits<double, 3>>(1.5, 2.25, 3.0));
    std::cout << std::format("intersection default [{}]", smoke_ix) << std::endl;
    std::cout << std::format("intersection combo [{:[,_.2f}]", smoke_ix) << std::endl;

    geometry::AxisAlignedBoundingBox<math::CartesianTraits<double, 3>> smoke_aabb(smoke_p, geometry::Point<math::CartesianTraits<double, 3>>(4.0, 5.0, 6.0));
    std::cout << std::format("aabb default [{}]", smoke_aabb) << std::endl;
    std::cout << std::format("aabb combo [{:[,_.2f}]", smoke_aabb) << std::endl;

    geometry::BoundingSphere<math::CartesianTraits<double, 3>> smoke_bs(smoke_p, 2.5);
    std::cout << std::format("bounding sphere default [{}]", smoke_bs) << std::endl;
    std::cout << std::format("bounding sphere combo [{:[,_.2f}]", smoke_bs) << std::endl;

    geometry::Orientation<math::CartesianTraits<double, 3>> smoke_or;
    std::cout << std::format("orientation default [{}]", smoke_or) << std::endl;
    std::cout << std::format("orientation combo [{:[,_.2f}]", smoke_or) << std::endl;

    geometry::OrientedPoint<math::CartesianTraits<double, 3>> smoke_op(smoke_p, smoke_or);
    std::cout << std::format("oriented point default [{}]", smoke_op) << std::endl;
    std::cout << std::format("oriented point combo [{:[,_.2f}]", smoke_op) << std::endl;

    geometry::Box<math::CartesianTraits<double, 3>> smoke_box(smoke_p, geometry::Point<math::CartesianTraits<double, 3>>(4.0, 5.0, 6.0));
    std::cout << std::format("box default [{}]", smoke_box) << std::endl;
    std::cout << std::format("box combo [{:[,_.2f}]", smoke_box) << std::endl;

    geometry::Circle<math::CartesianTraits<double, 3>> smoke_circle(smoke_p, 2.5);
    std::cout << std::format("circle default [{}]", smoke_circle) << std::endl;
    std::cout << std::format("circle combo [{:[,_.2f}]", smoke_circle) << std::endl;

    geometry::Sphere<math::CartesianTraits<double, 3>> smoke_sphere(smoke_p, 2.5);
    std::cout << std::format("sphere default [{}]", smoke_sphere) << std::endl;
    std::cout << std::format("sphere combo [{:[,_.2f}]", smoke_sphere) << std::endl;

    geometry::HalfSpace<math::CartesianTraits<double, 3>> smoke_hs(smoke_pl, math::Vector<math::CartesianTraits<double, 3>>::z_direction());
    std::cout << std::format("half space default [{}]", smoke_hs) << std::endl;
    std::cout << std::format("half space combo [{:[,_.2f}]", smoke_hs) << std::endl;

    geometry::HalfPlane<math::CartesianTraits<double, 3>> smoke_hp(smoke_l, math::Vector<math::CartesianTraits<double, 3>>::x_direction());
    std::cout << std::format("half plane default [{}]", smoke_hp) << std::endl;
    std::cout << std::format("half plane combo [{:[,_.2f}]", smoke_hp) << std::endl;

    // GJK cross-validation against known closed-form results
    {
        using traits3 = math::CartesianTraits<double, 3>;
        using point3 = geometry::Point<traits3>;

        geometry::AxisAlignedBoundingBox<traits3> aabb_a(point3(0.0, 0.0, 0.0), point3(2.0, 2.0, 2.0));
        geometry::AxisAlignedBoundingBox<traits3> aabb_b_overlap(point3(1.0, 1.0, 1.0), point3(3.0, 3.0, 3.0));
        geometry::AxisAlignedBoundingBox<traits3> aabb_c_disjoint(point3(5.0, 5.0, 5.0), point3(6.0, 6.0, 6.0));

        bool aabb_overlap_gjk = physics::intersects_gjk(aabb_a, aabb_b_overlap);
        bool aabb_overlap_expected = !geometry::disjoint(aabb_a, aabb_b_overlap);
        std::cout << "gjk aabb-aabb overlap: gjk=" << aabb_overlap_gjk << " expected=" << aabb_overlap_expected << (aabb_overlap_gjk == aabb_overlap_expected ? " OK" : " MISMATCH") << std::endl;

        bool aabb_disjoint_gjk = physics::intersects_gjk(aabb_a, aabb_c_disjoint);
        bool aabb_disjoint_expected = !geometry::disjoint(aabb_a, aabb_c_disjoint);
        std::cout << "gjk aabb-aabb disjoint: gjk=" << aabb_disjoint_gjk << " expected=" << aabb_disjoint_expected << (aabb_disjoint_gjk == aabb_disjoint_expected ? " OK" : " MISMATCH") << std::endl;

        geometry::BoundingSphere<traits3> sphere_a(point3(0.0, 0.0, 0.0), 2.0);
        geometry::BoundingSphere<traits3> sphere_b_overlap(point3(3.0, 0.0, 0.0), 2.0);
        geometry::BoundingSphere<traits3> sphere_c_disjoint(point3(10.0, 0.0, 0.0), 1.0);

        bool sphere_overlap_gjk = physics::intersects_gjk(sphere_a, sphere_b_overlap);
        std::cout << "gjk sphere-sphere overlap: gjk=" << sphere_overlap_gjk << " expected=1" << (sphere_overlap_gjk == true ? " OK" : " MISMATCH") << std::endl;

        bool sphere_disjoint_gjk = physics::intersects_gjk(sphere_a, sphere_c_disjoint);
        std::cout << "gjk sphere-sphere disjoint: gjk=" << sphere_disjoint_gjk << " expected=0" << (sphere_disjoint_gjk == false ? " OK" : " MISMATCH") << std::endl;

        geometry::BoundingSphere<traits3> sphere_unit(point3(0.0, 0.0, 0.0), 1.0);
        geometry::AxisAlignedBoundingBox<traits3> aabb_overlap_sphere(point3(0.5, 0.5, 0.5), point3(2.0, 2.0, 2.0));
        geometry::AxisAlignedBoundingBox<traits3> aabb_disjoint_sphere(point3(5.0, 5.0, 5.0), point3(6.0, 6.0, 6.0));

        bool sphere_aabb_overlap_gjk = physics::intersects_gjk(sphere_unit, aabb_overlap_sphere);
        std::cout << "gjk sphere-aabb overlap: gjk=" << sphere_aabb_overlap_gjk << " expected=1" << (sphere_aabb_overlap_gjk == true ? " OK" : " MISMATCH") << std::endl;

        bool sphere_aabb_disjoint_gjk = physics::intersects_gjk(sphere_unit, aabb_disjoint_sphere);
        std::cout << "gjk sphere-aabb disjoint: gjk=" << sphere_aabb_disjoint_gjk << " expected=0" << (sphere_aabb_disjoint_gjk == false ? " OK" : " MISMATCH") << std::endl;

        // argument-order symmetry
        bool sphere_aabb_overlap_swapped = physics::intersects_gjk(aabb_overlap_sphere, sphere_unit);
        std::cout << "gjk aabb-sphere overlap (swapped): gjk=" << sphere_aabb_overlap_swapped << " expected=1" << (sphere_aabb_overlap_swapped == true ? " OK" : " MISMATCH") << std::endl;
    }

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

    std::cout << (n_failures == 0 ? "OK: " : "Error: ") << (n_cases - n_failures) << " of " << n_cases << " cases passed" << std::endl;

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
