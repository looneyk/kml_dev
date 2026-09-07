//------------------------------------------------------------------------------
// vector_point_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/geometry/DiscretePoint.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/math/CartesianTraits.hpp"
#include "quetzal/math/DiscreteVector.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/math/Vector.hpp"
#include "quetzal/math/floating_point.hpp"
#include <algorithm>
#include <array>
#include <concepts>
#include <exception>
#include <format>
#include <iostream>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

using namespace std;
using namespace quetzal;

namespace
{

    static constexpr auto escape_reset_string = "\x1b[0m";
    static constexpr auto escape_rendition_pattern = "\x1b[38;5;{}m";

    size_t g_case_count = 0;
    size_t g_failure_count = 0;

    //--------------------------------------------------------------------------
    void report_group(const string& name)
    {
        cout << name << endl;
        return;
    }

    //--------------------------------------------------------------------------
    // Every case reports its own line so that a failure is identifiable by name
    void report_case(const string& name, bool b, const string& detail = {})
    {
        ++g_case_count;

        if (b)
        {
            cout << format(escape_rendition_pattern, 34) << "    ok   " << escape_reset_string << name << endl;
            return;
        }

        ++g_failure_count;
        cout << format(escape_rendition_pattern, 196) << "    FAIL " << escape_reset_string << name;
        if (!detail.empty())
        {
            cout << ": " << detail;
        }
        cout << endl;
        return;
    }

    //--------------------------------------------------------------------------
    int report_results()
    {
        if (g_failure_count == 0)
        {
            cout << format(escape_rendition_pattern, 34) << format("OK: {} cases", g_case_count) << escape_reset_string << endl;
            return 0;
        }

        cout << format(escape_rendition_pattern, 196) << format("Error: {} of {} cases failed", g_failure_count, g_case_count) << escape_reset_string << endl;
        return 1;
    }

    //--------------------------------------------------------------------------
    using traits2_type = math::CartesianTraits<float, 2>;
    using traits3_type = math::CartesianTraits<float, 3>;
    using traits4_type = math::CartesianTraits<float, 4>;
    using traits3d_type = math::CartesianTraits<double, 3>;
    using homogeneous_traits_type = math::HomogeneousTraits<double, 3>;

    using vector2_type = math::Vector<traits2_type>;
    using vector3_type = math::Vector<traits3_type>;
    using vector4_type = math::Vector<traits4_type>;
    using vector3d_type = math::Vector<traits3d_type>;
    using homogeneous_vector_type = math::Vector<homogeneous_traits_type>;

    using point3_type = geometry::Point<traits3_type>;
    using point3d_type = geometry::Point<traits3d_type>;
    using homogeneous_point_type = geometry::Point<homogeneous_traits_type>;

    using discrete_vector_type = math::DiscreteVector<long, 3>;
    using discrete_point_type = geometry::DiscretePoint<long, 3>;

    //--------------------------------------------------------------------------
    // What the arity constraints on the constructors and the component setters admit. These
    // replace the commented "// error" lines the two suites this replaces carried: a form that
    // must not compile is a case that passes when it does not
    template<typename T, typename... Args>
    concept constructible_from = requires(Args... args) { T(args...); };

    template<typename T>
    concept has_set_z = requires(T t) { t.set_z(typename T::value_type(0)); };

    template<typename T>
    concept has_set_w = requires(T t) { t.set_w(typename T::value_type(0)); };

    template<typename T>
    concept has_weight = requires(const T& t) { t.weight(); };

    template<typename T>
    concept addable = requires(T a, T b) { a + b; };

    template<typename P, typename V>
    concept displaceable = requires(P p, V v) { p + v; };

    template<typename T>
    concept crossable = requires(const T& a, const T& b) { cross(a, b); };

    template<typename T>
    concept perpable = requires(const T& v) { perp(v); };

    //--------------------------------------------------------------------------
    void test_construction()
    {
        report_group("construction");

        report_case("a two dimensional vector takes two components", constructible_from<vector2_type, float, float>);
        report_case("and not three", !constructible_from<vector2_type, float, float, float>);
        report_case("and not four", !constructible_from<vector2_type, float, float, float, float>);

        report_case("a three dimensional vector takes three components", constructible_from<vector3_type, float, float, float>);
        report_case("and not two", !constructible_from<vector3_type, float, float>);
        report_case("and not four", !constructible_from<vector3_type, float, float, float, float>);

        report_case("a four dimensional vector takes four components", constructible_from<vector4_type, float, float, float, float>);
        report_case("and not three", !constructible_from<vector4_type, float, float, float>);

        // Each component converts on its own, so the arguments need not share a type
        report_case("components may be given as mixed literals", constructible_from<vector3_type, float, int, double>);
        report_case("a point takes them too", constructible_from<point3_type, float, int, double>);
        report_case("with the values converted", vector3_type(1.0f, 2, 3.0) == vector3_type(1.0f, 2.0f, 3.0f));

        {
            vector3_type v;
            report_case("a default vector is zero", v.zero());
        }

        {
            vector3_type v(1.0f, 2.0f, 3.0f);
            vector3_type copy = v;
            report_case("a copy holds the same components", copy == v);
        }

        {
            array<float, 3> a {1.0f, 2.0f, 3.0f};
            vector3_type v(a);
            report_case("a vector reads its rep", v == vector3_type(1.0f, 2.0f, 3.0f));
        }

        {
            array<float, 3> a {4.0f, 5.0f, 6.0f};
            span<float, 3> s(a);
            vector3_type v(s);
            report_case("a vector reads a span", v == vector3_type(4.0f, 5.0f, 6.0f));
        }

        {
            vector<vector3_type> vs {{1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}};
            report_case("a container of vectors takes braced components", vs.size() == 2 && vs[1] == vector3_type(4.0f, 5.0f, 6.0f));
        }

        {
            point3_type p(1.0f, 2.0f, 3.0f);
            report_case("a point takes its components", p.x() == 1.0f && p.y() == 2.0f && p.z() == 3.0f);
        }

        {
            vector3_type v(1.0f, 2.0f, 3.0f);
            point3_type p(v);
            report_case("a point is built from a vector", p.x() == 1.0f && p.z() == 3.0f);
        }

        report_case("the origin is zero", point3_type::origin().zero());

        return;
    }

    //--------------------------------------------------------------------------
    void test_component_access()
    {
        report_group("component access");

        {
            vector3_type v(1.0f, 2.0f, 3.0f);
            report_case("indexing reads every component", v[0] == 1.0f && v[1] == 2.0f && v[2] == 3.0f);
            report_case("the named accessors agree with indexing", v.x() == v[0] && v.y() == v[1] && v.z() == v[2]);
            report_case("size reports the component count", v.size() == 3);
            report_case("data addresses the first component", *v.data() == 1.0f);
        }

        {
            vector3_type v(1.0f, 2.0f, 3.0f);
            v[1] = 20.0f;
            v.set_z(30.0f);
            report_case("indexing and the setters write", v == vector3_type(1.0f, 20.0f, 30.0f));
        }

        {
            vector3_type v(1.0f, 2.0f, 3.0f);
            v.set(4.0f, 5.0f, 6.0f);
            report_case("set writes every component at once", v == vector3_type(4.0f, 5.0f, 6.0f));
        }

        {
            vector3_type v(1.0f, 2.0f, 3.0f);
            report_case("get reads by compile time index", v.get<2>() == 3.0f);
        }

        report_case("a two dimensional vector has no z", !has_set_z<vector2_type>);
        report_case("a three dimensional vector has a z", has_set_z<vector3_type>);
        report_case("and no w", !has_set_w<vector3_type>);
        report_case("a four dimensional vector has a w", has_set_w<vector4_type>);

        {
            vector3_type v;
            v.clear();
            report_case("clear zeroes every component", v.zero());
        }

        return;
    }

    //--------------------------------------------------------------------------
    void test_vector_arithmetic()
    {
        report_group("vector arithmetic");

        const vector3_type a(1.0f, 2.0f, 3.0f);
        const vector3_type b(4.0f, 5.0f, 6.0f);

        report_case("addition", a + b == vector3_type(5.0f, 7.0f, 9.0f));
        report_case("subtraction", b - a == vector3_type(3.0f, 3.0f, 3.0f));
        report_case("scaling on the right", a * 2.0f == vector3_type(2.0f, 4.0f, 6.0f));
        report_case("scaling on the left", 2.0f * a == vector3_type(2.0f, 4.0f, 6.0f));
        report_case("division", b / 2.0f == vector3_type(2.0f, 2.5f, 3.0f));
        report_case("componentwise multiplication", a * b == vector3_type(4.0f, 10.0f, 18.0f));
        report_case("unary minus", -a == vector3_type(-1.0f, -2.0f, -3.0f));
        report_case("unary plus leaves it alone", +a == a);

        {
            vector3_type v = a;
            v += b;
            report_case("compound addition", v == a + b);
        }

        {
            vector3_type v = b;
            v -= a;
            report_case("compound subtraction", v == b - a);
        }

        {
            vector3_type v = a;
            v *= 3.0f;
            report_case("compound scaling", v == vector3_type(3.0f, 6.0f, 9.0f));
        }

        {
            vector3_type v = a;
            v /= 2.0f;
            report_case("compound division", v == vector3_type(0.5f, 1.0f, 1.5f));
        }

        report_case("equal vectors compare equal", a == vector3_type(1.0f, 2.0f, 3.0f));
        report_case("different vectors do not", !(a == b));

        return;
    }

    //--------------------------------------------------------------------------
    void test_vector_algebra()
    {
        report_group("vector algebra");

        const vector3_type v(3.0f, 4.0f, 0.0f);

        report_case("norm", v.norm() == 5.0f);
        report_case("norm squared avoids the root", v.norm_squared() == 25.0f);
        report_case("a normalized vector is a unit vector", normalize(v).unit());
        report_case("direction is the normalized vector", v.direction() == normalize(v));
        report_case("a zero vector is zero", vector3_type().zero());
        report_case("and a nonzero one is not", !v.zero());
        report_case("the longest component is named", v.longest_component() == 1);

        {
            vector3_type u = v;
            u.normalize();
            report_case("normalize in place", u.unit());
        }

        report_case("dot of perpendicular axes is zero", dot(vector3_type::x_direction(), vector3_type::y_direction()) == 0.0f);
        report_case("dot of a unit axis with itself is one", dot(vector3_type::x_direction(), vector3_type::x_direction()) == 1.0f);
        report_case("cross of x and y is z", cross(vector3_type::x_direction(), vector3_type::y_direction()) == vector3_type::z_direction());
        report_case("cross is anticommutative",
            cross(vector3_type::y_direction(), vector3_type::x_direction()) == -vector3_type::z_direction());

        report_case("the axis directions are unit vectors",
            vector3_type::x_direction().unit() && vector3_type::y_direction().unit() && vector3_type::z_direction().unit());
        report_case("direction by index agrees with the named form", vector3_type::direction(1) == vector3_type::y_direction());

        report_case("perpendicular axes report perpendicular", perpendicular(vector3_type::x_direction(), vector3_type::y_direction()));
        report_case("parallel vectors report parallel", parallel(v, v * 2.0f));
        report_case("perpendicular vectors are not parallel", !parallel(vector3_type::x_direction(), vector3_type::y_direction()));

        // perp and cross are each defined at one dimension only
        report_case("cross is three dimensional", crossable<vector3_type> && !crossable<vector2_type>);
        report_case("perp is two dimensional", perpable<vector2_type> && !perpable<vector3_type>);

        {
            vector2_type p = perp(vector2_type(1.0f, 0.0f));
            report_case("perp turns a two dimensional vector a quarter turn", p == vector2_type(0.0f, 1.0f));
            report_case("dot_perp of a vector with itself is zero", dot_perp(vector2_type(1.0f, 2.0f), vector2_type(1.0f, 2.0f)) == 0.0f);
        }

        return;
    }

    //--------------------------------------------------------------------------
    // A point is a position and a vector is a displacement, and the operations between them
    // are what keeps the two apart
    void test_point_and_vector()
    {
        report_group("point and vector");

        const point3_type p(1.0f, 2.0f, 3.0f);
        const point3_type q(4.0f, 6.0f, 8.0f);
        const vector3_type v(1.0f, 1.0f, 1.0f);

        report_case("a point displaced by a vector is a point", p + v == point3_type(2.0f, 3.0f, 4.0f));
        report_case("the displacement may come first", v + p == p + v);
        report_case("a point less a vector is a point", p - v == point3_type(0.0f, 1.0f, 2.0f));
        report_case("a point less a point is a vector", q - p == vector3_type(3.0f, 4.0f, 5.0f));
        report_case("and its type is the vector type", is_same_v<decltype(q - p), vector3_type>);

        report_case("two points do not add", !addable<point3_type>);
        report_case("two vectors do", addable<vector3_type>);
        report_case("a point takes a displacement", displaceable<point3_type, vector3_type>);

        {
            point3_type moved = p;
            moved += v;
            report_case("compound displacement", moved == p + v);
        }

        {
            point3_type moved = p + v;
            moved -= v;
            report_case("and its inverse returns the point", moved == p);
        }

        report_case("scaling a position is scaling its coordinates", p * 2.0f == point3_type(2.0f, 4.0f, 6.0f));
        report_case("to_vector is the displacement from the origin", to_vector(p) == vector3_type(1.0f, 2.0f, 3.0f));
        // to_vector is reachable by lookup on a Point, to_point is not: its argument is a
        // Vector, whose namespace is math rather than geometry
        report_case("to_point is its inverse", geometry::to_point<traits3_type>(vector3_type(1.0f, 2.0f, 3.0f)) == p);
        report_case("a point knows the vector to itself", p.vector() == to_vector(p));

        report_case("lerp at 0 is the first point", lerp(p, q, 0.0f) == p);
        report_case("lerp at 1 is the second", lerp(p, q, 1.0f) == q);
        report_case("and halfway is the midpoint", lerp(p, q, 0.5f) == point3_type(2.5f, 4.0f, 5.5f));

        report_case("min is componentwise", min(p, q) == p);
        report_case("max is componentwise", max(p, q) == q);

        report_case("equal points compare equal", p == point3_type(1.0f, 2.0f, 3.0f));
        report_case("ordering runs by the highest component first", p < q);

        report_case("all_less holds where every component is", all_less(p, q));
        report_case("any_less holds where one is", any_less(p, point3_type(0.0f, 6.0f, 0.0f)));

        return;
    }

    //--------------------------------------------------------------------------
    // Homogeneous coordinates carry one component more than the dimension, and the weight is
    // what tells a position from a direction
    void test_homogeneous()
    {
        report_group("homogeneous coordinates");

        report_case("the stored count exceeds the dimension",
            homogeneous_traits_type::size == homogeneous_traits_type::dimension + 1);
        report_case("a cartesian type stores its dimension", traits3_type::size == traits3_type::dimension);

        report_case("a homogeneous type has a weight", has_weight<homogeneous_vector_type>);
        report_case("a cartesian type does not", !has_weight<vector3d_type>);

        report_case("a homogeneous vector weighs 0", homogeneous_vector_type(1.0, 2.0, 3.0).weight() == 0.0);
        report_case("a homogeneous point weighs 1", homogeneous_point_type(1.0, 2.0, 3.0).weight() == 1.0);

        {
            homogeneous_point_type p(6.0, -10.0, 14.0);
            p.rep()[3] = 2.0;
            p.projective_divide();
            report_case("projective division reduces to a weight of 1", p.weight() == 1.0);
            report_case("scaling the coordinates with it", p.x() == 3.0 && p.y() == -5.0 && p.z() == 7.0);
        }

        {
            homogeneous_point_type p(1.0, 2.0, 3.0);
            homogeneous_point_type q(4.0, 6.0, 8.0);
            auto d = q - p;
            report_case("a difference of homogeneous points weighs 0", d.weight() == 0.0);
            report_case("and holds the displacement", d.x() == 3.0 && d.y() == 4.0 && d.z() == 5.0);
        }

        return;
    }

    //--------------------------------------------------------------------------
    void test_structured_bindings()
    {
        report_group("structured bindings");

        report_case("a cartesian vector binds its dimension", tuple_size_v<vector3d_type> == 3);
        report_case("a homogeneous vector binds its weight as well", tuple_size_v<homogeneous_vector_type> == 4);
        report_case("a cartesian point binds its dimension", tuple_size_v<point3d_type> == 3);
        report_case("a homogeneous point binds its weight as well", tuple_size_v<homogeneous_point_type> == 4);
        report_case("a discrete vector binds its dimension", tuple_size_v<discrete_vector_type> == 3);
        report_case("a discrete point binds its dimension", tuple_size_v<discrete_point_type> == 3);

        {
            vector3d_type v(1.0, 2.0, 3.0);
            auto [x, y, z] = v;
            report_case("a copy binds every component", x == 1.0 && y == 2.0 && z == 3.0);
        }

        {
            vector3d_type v(1.0, 2.0, 3.0);
            auto& [x, y, z] = v;
            y = 20.0;
            report_case("a reference binds the components themselves", v.y() == 20.0);
        }

        {
            const point3d_type p(4.0, 5.0, 6.0);
            const auto& [x, y, z] = p;
            report_case("a const reference reads every component", x == 4.0 && y == 5.0 && z == 6.0);
        }

        {
            homogeneous_vector_type v(7.0, 8.0, 9.0);
            auto [x, y, z, w] = v;
            report_case("a homogeneous vector binds a weight of 0", x == 7.0 && z == 9.0 && w == 0.0);
        }

        {
            homogeneous_point_type p(7.0, 8.0, 9.0);
            auto [x, y, z, w] = p;
            report_case("a homogeneous point binds a weight of 1", x == 7.0 && z == 9.0 && w == 1.0);
        }

        {
            discrete_vector_type v(1L, 2L, 3L);
            auto [i, j, k] = v;
            report_case("a discrete vector binds every component", i == 1L && j == 2L && k == 3L);
        }

        return;
    }

    //--------------------------------------------------------------------------
    // The order a discrete vector sorts in, which decides what a container of them holds
    void test_discrete_vector_order()
    {
        report_group("discrete vector ordering");

        using discrete_vector2_type = math::DiscreteVector<long, 2>;
        using discrete_vector4_type = math::DiscreteVector<long, 4>;

        report_case("y outranks x", discrete_vector2_type(5L, 0L) < discrete_vector2_type(0L, 1L));
        report_case("x decides where y is equal", discrete_vector2_type(1L, 7L) < discrete_vector2_type(2L, 7L));
        report_case("z outranks y and x", discrete_vector_type(9L, 9L, 0L) < discrete_vector_type(0L, 0L, 1L));
        report_case("y decides where z is equal", discrete_vector_type(9L, 5L, 3L) < discrete_vector_type(0L, 9L, 3L));
        report_case("x decides where z and y are equal", discrete_vector_type(1L, 2L, 3L) < discrete_vector_type(2L, 2L, 3L));
        report_case("w outranks z", discrete_vector4_type(9L, 9L, 9L, 0L) < discrete_vector4_type(0L, 0L, 0L, 1L));

        report_case("greater is the mirror of less", discrete_vector_type(0L, 0L, 1L) > discrete_vector_type(9L, 9L, 0L));
        report_case("equal is neither less nor greater",
            !(discrete_vector_type(1L, 2L, 3L) < discrete_vector_type(1L, 2L, 3L))
            && !(discrete_vector_type(1L, 2L, 3L) > discrete_vector_type(1L, 2L, 3L)));
        report_case("equal satisfies both inclusive forms",
            discrete_vector_type(1L, 2L, 3L) <= discrete_vector_type(1L, 2L, 3L)
            && discrete_vector_type(1L, 2L, 3L) >= discrete_vector_type(1L, 2L, 3L));
        report_case("negatives order below positives", discrete_vector_type(0L, 0L, -1L) < discrete_vector_type(0L, 0L, 0L));

        report_case("equality holds", discrete_vector_type(1L, 2L, 3L) == discrete_vector_type(1L, 2L, 3L));
        report_case("inequality holds", discrete_vector_type(1L, 2L, 3L) != discrete_vector_type(1L, 2L, 4L));
        report_case("the comparison reports equivalence",
            (discrete_vector_type(1L, 2L, 3L) <=> discrete_vector_type(1L, 2L, 3L)) == strong_ordering::equal);

        {
            vector<discrete_vector_type> vs {
                discrete_vector_type(2L, 0L, 1L),
                discrete_vector_type(0L, 1L, 0L),
                discrete_vector_type(1L, 0L, 0L),
                discrete_vector_type(0L, 0L, 1L)};
            sort(vs.begin(), vs.end());
            report_case("a sort runs by z, then y, then x",
                vs[0] == discrete_vector_type(1L, 0L, 0L)
                && vs[1] == discrete_vector_type(0L, 1L, 0L)
                && vs[2] == discrete_vector_type(0L, 0L, 1L)
                && vs[3] == discrete_vector_type(2L, 0L, 1L));
        }

        return;
    }

    //--------------------------------------------------------------------------
    // The discrete pair keeps the same separation the floating point pair does, over a lattice
    void test_discrete_point_and_vector()
    {
        report_group("discrete point and vector");

        const discrete_point_type p(1L, 2L, 3L);
        const discrete_vector_type v(1L, 1L, 1L);

        report_case("a discrete point displaced by a discrete vector is a point",
            is_same_v<decltype(p + v), discrete_point_type>);

        {
            discrete_point_type moved = p + v;
            report_case("with the components summed", moved.x() == 2L && moved.y() == 3L && moved.z() == 4L);
        }

        {
            discrete_point_type moved = v + p;
            report_case("the displacement may come first", moved.x() == 2L && moved.z() == 4L);
        }

        {
            discrete_point_type moved = p;
            moved += v;
            report_case("compound displacement", moved.x() == 2L && moved.z() == 4L);
            moved -= v;
            report_case("and its inverse returns the point", moved.x() == 1L && moved.z() == 3L);
        }

        report_case("two discrete points do not add", !addable<discrete_point_type>);
        report_case("two discrete vectors do", addable<discrete_vector_type>);

        report_case("a discrete vector reduces to its longest axis",
            discrete_vector_type(1L, 5L, 2L).direction() == discrete_vector_type(0L, 1L, 0L));
        report_case("keeping the sign", discrete_vector_type(0L, 0L, -9L).direction() == discrete_vector_type(0L, 0L, -1L));
        report_case("a null vector has no direction", discrete_vector_type().direction().zero());

        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main()
{
    try
    {
        test_construction();
        test_component_access();
        test_vector_arithmetic();
        test_vector_algebra();
        test_point_and_vector();
        test_homogeneous();
        test_structured_bindings();
        test_discrete_vector_order();
        test_discrete_point_and_vector();
    }
    catch (const exception& e)
    {
        cout << format(escape_rendition_pattern, 196) << "Error: " << escape_reset_string << e.what() << endl;
        return 1;
    }

    return report_results();
}
