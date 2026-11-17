//------------------------------------------------------------------------------
// laminaria
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/clip.hpp"
#include "quetzal/brep/connection.hpp"
#include "quetzal/geometry/CurveNURBS.hpp"
#include "quetzal/geometry/HalfSpace.hpp"
#include "quetzal/geometry/Path.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/Polygon.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/math/math_util.hpp"
#include "quetzal/math/parametric.hpp"
#include "quetzal/model/helix_anticylinder.hpp"
#include "quetzal/model/helix_pyramid.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include <algorithm>
#include <array>
#include <format>
#include <iostream>
#include <limits>
#include <ranges>
#include <string>
#include <cassert>

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_traits = brep::MeshTraits<vector_traits>;
    using mesh_type = brep::Mesh<mesh_traits>;
    using matrix_type = geometry::transformation_matrix_type<vector_traits>;
    using point_type = mesh_type::point_type;
    using vector_type = point_type::vector_type;
    using polygon_type = geometry::Polygon<vector_traits>;
    using path_type = geometry::Path<vector_traits>;

    namespace g = quetzal::geometry;

    // egg formula parameters ...
    static const value_type wEgg = 0.76f;
    static const value_type kEgg = 0.5f;

    //--------------------------------------------------------------------------
    template<typename T>
    T sphere_contour(const T& t)
    {
        return sqrt(t - t * t);
    }

    //--------------------------------------------------------------------------
    template<typename T>
    T egg_contour(const T& x)
    {
        return wEgg * sqrt((x - x * x) / (T(1) + kEgg * (x - T(0.5))));
    }

    //--------------------------------------------------------------------------
    template<typename T>
    T egg_contour_tangent(const T& x)
    {
        if (x == T(0))
        {
            return numeric_limits<T>::max(); // infinity ...
        }
        else if (x == T(1))
        {
            return numeric_limits<T>::lowest(); // -infinity ...
        }

        T u = x - x * x;
        T v = T(1) + kEgg * (x - T(0.5));
        return T(0.5) * wEgg * ((1 - 2 * x) / v - kEgg * u / (v * v)) / sqrt(u / v);
    }

    //--------------------------------------------------------------------------
    template<typename T>
    T egg_contour_inverse_tangent(const T& m)
    {
        // return x value where tangent is m
        // binary search, until the formula gets inverted, yeah right ...

        T resolution = T(0.0001);

        T m0 = egg_contour_tangent(T(0));
        T m1 = egg_contour_tangent(T(1));
        T x = T(0.5);
        for (T dx = T(0.5); dx > resolution; )
        {
            T mm = egg_contour_tangent(x);
            if ((m >= m0 && m < mm) || (m < m0 && m >= mm))
            {
                m1 = mm;
                dx /= T(2);
                x -= dx;
            }
            else
            {
                m0 = mm;
                dx /= T(2);
                x += dx;
            }

//            cout << m << "\t" << m0 << "\t" << m1 << "\t" << x << "\t" << dx << endl;
        }

        return x;
    }

    //--------------------------------------------------------------------------
    // for circular, but close enough for egg
    // approximates x where t values are evenly spaced along the arc length
    // returns x: 0..1 for t: 0..1
    template<typename T>
    T uniform_spacing(const T& t)
    {
        return (T(1) - cos(math::Pi<T> * t)) / T(2);
    }

    //--------------------------------------------------------------------------
    // for circular, but close enough for egg
    // approximates t (arc length) for x (x-axis)
    // returns t: 0..1 for x: 0..1
    template<typename T>
    T inverse_uniform_spacing(const T& x)
    {
        return acos(T(1) - T(2) * x) / math::Pi<T>; // denoise ...
    }

    //--------------------------------------------------------------------------
    template<typename T>
    T blob_contour(const T& t)
    {
        return T(1.0) * sqrt(pow(t, T(0.1)) - t * t);
    }

    //--------------------------------------------------------------------------
    // back and front should start and end at the same points, typically {0, 0}..{1, 0}
    // back is the part in the -y direction and front is the part in the +y direction if the curves are in the xy-plane
    g::Polygon<vector_traits> create_polygon(const g::Curve<point_type>& back, const g::Curve<point_type>& front, size_t resolution)
    {
        g::Polygon<vector_traits> polygon(2 * resolution);

        auto [t_back_0, t_back_1] = back.domain_interval();
        auto [t_front_0, t_front_1] = front.domain_interval();

        for (size_t i = 0; i < resolution; ++i)
        {
            auto t = vector_traits::value_type(i) / resolution;
            auto t_back = lerp(t_back_0, t_back_1, t);
            auto t_front = lerp(t_front_1, t_front_0, t);

            polygon.vertex(i) = front(t_front);
            polygon.vertex(resolution + i) = back(t_back);
        }

        return polygon;
    }

    //--------------------------------------------------------------------------
    g::Polygon<vector_traits> create_laminaria_cross_section(size_t nAzimuthSpike)
    {
        size_t degree = 3;
        vector_type right = vector_type::y_direction();

        vector<point_type> control_points_front =
        {
            {-0.5f, 0.0f, 0.0f},
            {-0.25f, 0.4f, 0.0f},
            {0.0f, 0.1f, 0.0f},
            {0.25f, 0.4f, 0.0f},
            {0.5f, 0.0f, 0.0f},
        };

        vector<point_type> control_points_back =
        {
            {-0.5f, 0.0f, 0.0f},
            {-0.25f, -0.2f, 0.0f},
            {0.0f, -0.3f, 0.0f},
            {0.25f, -0.2f, 0.0f},
            {0.5f, 0.0f, 0.0f},
        };

        g::CurveNURBS<point_type> curve_front(degree, control_points_front);
        g::CurveNURBS<point_type> curve_back(degree, control_points_back);
        return create_polygon(curve_back, curve_front, nAzimuthSpike);
    }
    
    //--------------------------------------------------------------------------
    g::Path<vector_traits> create_spike_path(value_type dzSpike, size_t ntSpike)
    {
        size_t degree = 3;
        vector_type right = vector_type::y_direction();
        vector<point_type> control_points =
        {
            {0.0f, 0.0f, 0.0f},
            {0.5f, 0.0f, 0.33f},
            {-0.1f, 0.0f, 0.67f},
            {0.25f, 0.0f, 1.0f},
        };

value_type t0 = 0.0f;
value_type t1 = 1.0f;

        g::CurveNURBS<point_type> curve_path(degree, control_points);
        g::Path<vector_traits> path(ntSpike + 1);
        for (size_t i = 0; i <= ntSpike; ++i)
        {
            value_type t = lerp(t0, t1, value_type(i) / ntSpike);
            point_type position = curve_path(t);
            vector_type tangent = curve_path.direction(t);
//vector_type tangent = vector_type::z_direction();
            mesh_type::vector_type direction = tangent;
            path[i] = {position * dzSpike, {normalize(direction), right}};
        }

        return path;
    }

    //--------------------------------------------------------------------------
    id_type create_stone(mesh_type& mesh, const string& name, value_type r, size_t resolution)
    {
        id_type submesh_id = model::create_geodesic_sphere(mesh, name, r, resolution, true, {{"smooth", "true"}});
        auto dzBase = r / 3.0f;
        vector_type v{0.0f, 0.0f, dzBase};
        model::transform(mesh.submesh(submesh_id), g::translation(v));
        geometry::HalfSpace<vector_traits> hs{{point_type::origin(), -vector_type::z_direction()}, -vector_type::z_direction()};
        clip(mesh.submesh(submesh_id), hs, "clip_lower");

        // Hollow out center to save filament
        std::string inner_name = "inner_" + name;
        id_type inner_id = mesh.copy(submesh_id, inner_name, [](const auto& vertex) { auto v = vertex; v.attributes().position() *= 0.5f; return v; });
        mesh.rename_surface(mesh.surface_id(submesh_id, "body"), "inner_body");
        invert(mesh.submesh(inner_id));
        brep::attach(mesh, brep::extended_surface_name(name, "clip_lower"), brep::extended_surface_name(inner_name, "clip_lower"));
        mesh.rename_submesh(inner_name, name);

        model::transform(mesh.submesh(submesh_id), g::translation(-v));
        return submesh_id;
    }

    //--------------------------------------------------------------------------
    id_type create_spike(mesh_type& mesh, const polygon_type& polygon, const path_type& path, size_t nSpike, size_t nSpikes, value_type r, size_t nAzimuth, value_type revs, value_type scale_spike_width, bool tip = true)
    {
        string spike_name = std::format("spike{}", nSpike);

        value_type t = value_type(nSpike) / nSpikes;
        value_type spike_scale = 1.0f - t * 0.75f;

        size_t nUndulations = static_cast<size_t>(15.0f * spike_scale) * 2 + 1;

        auto width_scaler = [scale_spike_width, nUndulations](const g::Polygon<vector_traits>& polygon, value_type t) -> g::Polygon<vector_traits>
        {
            value_type s0 = value_type(1);
            value_type s1 = value_type(0.01); // 0 doesn't work with swept volume, does not support apex_cusp ...
            value_type factor = lerp(s0, s1, t);
            factor = sqrt(cos(t * math::PiHalf<value_type> * 0.98f));
            factor += 0.075f * sin(t * math::Pi<value_type> * nUndulations);
            factor *= scale_spike_width;
            return scale(polygon, factor);
        };

        auto scaler = [scale_spike_width, nUndulations](value_type t) -> value_type
        {
            value_type s0 = value_type(1);
            value_type s1 = value_type(0.01); // 0 doesn't work with swept volume, does not support apex_cusp ...
            value_type factor = lerp(s0, s1, t);
            factor = sqrt(cos(t * math::PiHalf<value_type> * 0.98f));
            factor += 0.075f * sin(t * math::Pi<value_type> * nUndulations);
            factor *= scale_spike_width;
            return factor;
        };

        auto rotator = [](value_type) -> value_type { return value_type(0); };

// problem with op.position() ...
        g::Path<vector_traits> path_spike = path;
        std::ranges::transform(path_spike, path_spike.begin(), [spike_scale](auto& op) -> g::OrientedPoint<vector_traits> { return {op.position() * spike_scale, op.orientation()}; });

        // if end1 scale is 0, disable tip, is this the only reason that connect_end was used? ...
        // with the old single callback version, it could return a tuple of polygon, degenerate flag (since it knows the scaling internally) ...
        // in general, we still need to account for degenerate polygons at path points other than at the ends ...
        // *** test this with s1 = 0 now ...

        mesh_type mSpike;
//        model::create_swept_volume(mSpike, nameSpike, polygon, path_spike, width_scaler);
        id_type submesh_id = model::create_swept_volume(mSpike, spike_name, polygon, path_spike, scaler, rotator);
        if (tip)
        {
            model::connect_end_round(mSpike, spike_name, "end1", "top");
        }

        auto rInner = 0.8f * r;
        value_type theta = t * math::PiFourth<value_type>;
        point_type position = {rInner * cos(theta), 0.0f, rInner * sin(theta)};

        value_type phi = t * revs * math::PiTwo<value_type>;
        phi *= value_type(2 * nAzimuth - 1) / (2 * nAzimuth);

        auto matrix = g::translation(to_vector(position)) * g::rotation_z(phi);
        model::transform(mSpike, matrix);

        mesh.insert(mSpike);
        return submesh_id;
    }

    //--------------------------------------------------------------------------
    id_type create_laminaria_spike(mesh_type& mesh, const string& name, value_type r, value_type dzSpike, size_t nAzimuthSpike, size_t ntSpike)
    {
        name; // ...

        auto polygon = create_laminaria_cross_section(nAzimuthSpike);
        auto path = create_spike_path(dzSpike, ntSpike);

        value_type scale_spike_width = 10.0f;
        return create_spike(mesh, polygon, path, 0, 1, r, 1, 0.0f, scale_spike_width, false);
    }

    //--------------------------------------------------------------------------
    id_type create_laminaria(mesh_type& mesh, const string& name, value_type r, value_type dzSpike, size_t nAzimuth, value_type revs, size_t resolutionBase, size_t nAzimuthSpike, size_t ntSpike)
    {
        auto polygon = create_laminaria_cross_section(nAzimuthSpike);
        auto path = create_spike_path(dzSpike, ntSpike);
        id_type submesh_id = create_stone(mesh, name, r, resolutionBase);

        value_type scale_spike_width = 10.0f;
        size_t nSpikes = static_cast<size_t>(nAzimuth * revs);
        for (size_t nSpike = 0; nSpike < nSpikes; ++nSpike)
        {
            create_spike(mesh, polygon, path, nSpike, nSpikes, r, nAzimuth, revs, scale_spike_width, true);
            // in this case, there is no connect, spikes are separate submeshes ...
        }

        mesh.pack(); // submesh id's may change after pack ...
        return submesh_id;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    filesystem::path dir_out = argc > 1 ? argv[1] : "out/";

    auto to_filename = [&dir_out](const string& name)
    {
        return dir_out / (name + ".obj");
    };

    string name;
    mesh_type mesh;

    vector<point_type> points =
    {
        {1.0f, 0.0f, 0.0f},
        {0.5f, 0.5f, 0.0f},
        {0.0f, 0.25f, 0.0f},
        {-0.5f, 0.5f, 0.0f},
        {-1.0f, 0.0f, 0.0f},
    };

//    vector<value_type> knots1 = {0, 0, 0.25f, 0.5f, 0.75f, 1.0f, 1.0f};
    g::CurveNURBS<point_type> c1(1, points);

//    for (size_t i = 0; i < c1.knot_count(); ++i)
//    {
//        cout << "knot " << i << "\t" << c1.knot(i) << endl;
//    }

//    for (size_t i = 0; i <= 10; ++i)
//    {
//        cout << "curve1 " << i << "\t" << c1(i / 10.0f) << endl;
//    }

c1.direction(0.0f); // ...

    g::Polygon<vector_traits> polygon1(20);
    polygon1.vertex(0) = c1(0.0f);
    polygon1.vertex(10) = c1(1.0f);
    for (size_t i = 1; i < 10; ++i)
    {
        auto point = c1(0.1f * i);
//        cout << i << "\t" << polygon1.vertex(i) << endl;
        polygon1.vertex(i) = point;
        point.y() = -point.y();
        polygon1.vertex(20 - i) = point;
    }

//    for (size_t i = 0; i < polygon1.vertex_count(); ++i)
//    {
//        cout << "polygon1 " << i << "\t" << polygon1.vertex(i) << endl;
//    }

// normal degradation between 1 and 1b, different centroid implies different polygons ...
// curve calculations are not as symmetrical ...
// not much to be done there ...

// use this to test extrusion while we're here ...

    vector<point_type> points1f =
    {
        {-1.0f, 0.0f, 0.0f},
        {-0.5f, 0.5f, 0.0f},
        {0.0f, 0.25f, 0.0f},
        {0.5f, 0.5f, 0.0f},
        {1.0f, 0.0f, 0.0f},
    };
    g::CurveNURBS<point_type> c1f(1, points1f);

    vector<point_type> points1b =
    {
        {-1.0f, 0.0f, 0.0f},
        {-0.5f, -0.5f, 0.0f},
        {0.0f, -0.25f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {1.0f, 0.0f, 0.0f},
    };
    g::CurveNURBS<point_type> c1b(1, points1b);

//    for (size_t i = 0; i <= 10; ++i)
//    {
//        cout << "curve1b " << i << "\t" << c1b(i / 10.0f) << endl;
//    }

//    for (size_t i = 0; i <= 10; ++i)
//    {
//        cout << "curve1f " << i << "\t" << c1f(i / 10.0f) << endl;
//    }

    auto polygon1b = create_polygon(c1b, c1f, 10);

//    for (size_t i = 0; i < polygon1b.vertex_count(); ++i)
//    {
//        cout << "polygon1b " << i << "\t" << polygon1b.vertex(i) << endl;
//    }

    name = "laminaria_swept_1";
    cout << name << endl;
    mesh.clear();
//    model::create_extrusion(mesh, name, polygon, {0.0f, 0.0f, 50.0f});
    polygon1.transform(g::scaling(30.0f));
    model::create_cylinder(mesh, name, polygon1, 1, 0.0f, 50.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "laminaria_swept_1b";
    cout << name << endl;
    mesh.clear();
//    model::create_extrusion(mesh, name, polygon, {0.0f, 0.0f, 50.0f});
    polygon1b.transform(g::scaling(30.0f));
    model::create_cylinder(mesh, name, polygon1b, 1, 0.0f, 50.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

//    vector<value_type> knots2 = {0, 0, 0, 0.333f, 0.666f, 1.0f, 1.0f, 1.0f};
    g::CurveNURBS<point_type> c2(2, points);

//    for (size_t i = 0; i < c2.knot_count(); ++i)
//    {
//        cout << "knot " << i << "\t" << c2.knot(i) << endl;
//    }

//    for (size_t i = 0; i <= 10; ++i)
//    {
//        cout << "curve " << i << "\t" << c2(i / 10.0f) << endl;
//    }

    g::Polygon<vector_traits> polygon2(20);
    polygon2.vertex(0) = c2(0.0f);
    polygon2.vertex(10) = c2(1.0f);
    for (size_t i = 1; i < 10; ++i)
    {
        auto point = c2(0.1f * i);
//        cout << i << "\t" << polygon2.vertex(i) << endl;
        polygon2.vertex(i) = point;
        point.y() = -point.y();
        polygon2.vertex(20 - i) = point;
    }

//    for (size_t i = 0; i < polygon2.vertex_count(); ++i)
//    {
//        cout << "polygon " << i << "\t" << polygon2.vertex(i) << endl;
//    }

    name = "laminaria_swept_2";
    cout << name << endl;
    mesh.clear();
//    model::create_extrusion(mesh, name, polygon, {0.0f, 0.0f, 100.0f});
    polygon2.transform(g::scaling(30.0f));
    model::create_cylinder(mesh, name, polygon2, 1, 0.0f, 50.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "laminaria_10_spike_1";
    cout << name << endl;
    mesh.clear();
    create_laminaria_spike(mesh, name, 20.0f, 150.0f, 8, 10);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "laminaria_20";
    cout << name << endl;
    mesh.clear();
    create_laminaria(mesh, name, 20.0f, 150.0f, 8, 2.0f, 3, 8, 20);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "laminaria_100";
    cout << name << endl;
    mesh.clear();
    create_laminaria(mesh, name, 20.0f, 150.0f, 8, 2.0f, 3, 8, 100);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
/*
    name = "laminaria_300";
    cout << name << endl;
    mesh.clear();
    create_laminaria(mesh, name, 20.0f, 150.0f, 8, 2.0f, 24, 30, 300);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
*/
    return 0;
}
