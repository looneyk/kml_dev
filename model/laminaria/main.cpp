//------------------------------------------------------------------------------
// laminaria
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/clip.hpp"
#include "quetzal/brep/connection.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/geometry/CurveNURBS.hpp"
#include "quetzal/geometry/HalfSpace.hpp"
#include "quetzal/geometry/Point.hpp"
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
    using vector_type = mesh_type::vector_type;
    using point_type = mesh_type::point_type;

    namespace g = quetzal::geometry;

    // egg formula parameters ...
    static const value_type wEgg = 0.76f;
    static const value_type kEgg = 0.5f;

    static const filesystem::path dir_out = "out/";
    static const filesystem::path dir_ref = "reference/";
    static const string app_name = "laminaria";

    //--------------------------------------------------------------------------
    filesystem::path to_filename(const string& name)
    {
        return dir_out / (name + ".obj");
    }

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
            return -numeric_limits<T>::max(); // -infinity ...
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
        return acos(T(1) - T(2) * x) / math::Pi<T>;
    }

    //--------------------------------------------------------------------------
    template<typename T>
    T blob_contour(const T& t)
    {
        return T(1.0) * sqrt(pow(t, T(0.1)) - t * t);
    }

    //--------------------------------------------------------------------------
    template<typename M>
    void truncate(M& mesh, size_t nAzimuth)
    {
        id_type idLower = 2;
        id_type idUpper = mesh.halfedge_count() - 6 * (nAzimuth + 1) - 2;
        geometry::HalfSpace<vector_traits> hsLower({mesh.vertex(idLower).attributes().position(), -vector_type::z_direction()});
        geometry::HalfSpace<vector_traits> hsUpper({mesh.vertex(idUpper).attributes().position(), vector_type::z_direction()});
        clip(mesh, hsLower, "clip_lower");
        clip(mesh, hsUpper, "clip_upper");
        return;
    }
/*
    //--------------------------------------------------------------------------
    void create_helical_claws(mesh_type& mesh, const string& name, size_t nAzimuth, value_type r, value_type z1, value_type revs1, size_t nAzimuthSpike, size_t ntSpike, value_type revsSpike)
    {
        value_type z0 = 0.0f;
        value_type revs0 = 0.0f;

        std::function<value_type(const value_type&)> body_contour = blob_contour<value_type>;
//        std::function<value_type(const value_type&)> body_contour = egg_contour<value_type>;
//        std::function<value_type(const value_type&)> body_contour = sphere_contour<value_type>;
// need different x range for blob ...

        std::function<value_type(const value_type&)> body_contour_slope = egg_contour_tangent<value_type>;

        value_type tz0 = 0.0625f;
        value_type tz1 = 0.9375f;

// not z1 ...
        mesh_type m;
        model::create_helix_anticylinder(m, name, nAzimuth, math::scale(r, math::restrict<value_type>(tz0, tz1, body_contour)), math::restrict<value_type>(tz0, tz1, body_contour_slope), z0, z1, revs0, revs1, true);
        truncate(m, nAzimuth);

        // transform so that base is at z = 0
        value_type z00 = m.surface(0, "clip_lower").faces().front().halfedge().vertex_attributes().position().z();
        model::transform(m, geometry::translation(0.0f, 0.0f, -z00));

        model::connect_end_round_regular(m, name, "clip_upper", "top");

        // Hollow out center to save filament
        auto mm = m;
        for (auto& submesh : mm.submeshes())
        {
            submesh.set_name("inner_" + submesh.name());
        }
        for (auto& surface : mm.surfaces())
        {
            surface.set_name("inner_" + surface.name());
        }
        model::transform(mm, geometry::scaling(0.75f));
        invert(mm);
        m.insert(mm);
        brep::attach(m, name + "/clip_lower", "inner_" + name + "/inner_clip_lower");

        size_t nSpikes = static_cast<size_t>(nAzimuth * (revs1 - revs0 - 2.0f));
        for (size_t nSpike = 0; nSpike < nSpikes; ++nSpike)
        {
            size_t nFace = 2 * (nAzimuth + nSpike);

            auto face = m.face(nFace);
            assert(face.halfedge_count() == 3);
            auto normal = face.attributes().normal();
            auto polygonFace = to_polygon(face);
            auto [position, radius] = triangle_incenter(polygonFace.vertex(0), polygonFace.vertex(1), polygonFace.vertex(2));
            radius *= 0.9f;

            string nameSpike = "spike" + to_string(nSpike);
            string nameSpikeBase = nameSpike + "/end0";

            // Create path
            value_type slope0 = sqrt(normal.x() * normal.x() + normal.y() * normal.y()) / normal.z();
            value_type x0 = egg_contour_inverse_tangent(slope0);
            value_type t0 = inverse_uniform_spacing(x0);
            value_type t1 = 0.95f;
            vector_type right = vector_type::y_direction();
            value_type factor = r + (3 * r * (nSpikes - nSpike - 1)) / nSpikes; // scale factor 1..4 * r ...

            g::Path<vector_traits> path(ntSpike + 1);
            for (size_t i = 0; i <= ntSpike; ++i)
            {
                value_type t = lerp(t0, t1, value_type(i) / ntSpike);
                value_type x = uniform_spacing(t);
                value_type y = egg_contour(x);
                point_type pos = {y, 0.0f, x};
                value_type tangent = egg_contour_tangent(x);
                mesh_type::vector_type direction = {tangent, 0.0f, 1.0f};
                path[i] = {scale(pos, factor), {normalize(direction), right}};
//path[i] = {geometry::to_point(to_vector(pos) * factor), {normalize(direction), right}};
            }

            point_type positionSpikeOrig = path[0].position();
            point_type positionSpikeRadial = {math::azimuth_radius(position), 0.0f, 0.0f};

            // Apply incremental rotation
            value_type phiMax = math::PiTwo<value_type> * revsSpike;
            auto phiSpikeMax = phiMax * factor / (4 * r);
            for (size_t i = 0; i <= ntSpike; ++i)
            {
                auto phi = phiSpikeMax * i / ntSpike;
                auto matrix = g::rotation_z(phi);

                auto& op = path[i];
                op.position() += positionSpikeRadial - positionSpikeOrig;
const auto positionOrig = op.position();
                op.position() *= matrix;

                auto& orientation = op.orientation();
// not taking dphi (but not that simple) into account ...
//                orientation.direction() = normalize(orientation.direction() * matrix);
auto md = g::rotation_z(phi * 0.5f);
orientation.direction() = normalize(orientation.direction() * md);

//vector_type axis = {op.position().x(), op.position().y(), 0.0f};
//orientation.direction() = normalize(orientation.direction() * g::rotation_axis_unit(normalize(axis), -0.5f));

// this the same as simple matrix multiplication
//orientation.direction() = normalize((positionOrig + orientation.direction()) * matrix - op.position());

//R' * p + R * p'
//orientation.direction() = normalize(g::rotation_dz(phi) * to_vector(op.position()) + matrix * orientation.direction());

                auto f = cross(orientation.direction(), orientation.right());
                orientation.right() = normalize(cross(f, orientation.direction()));
            }

            // Polygonal cross section
            g::OrientedPolygon<vector_traits> polygon(nAzimuthSpike);
            for (size_t i = 0; i < nAzimuthSpike; ++i)
            {
                value_type phi = math::PiTwo<value_type> * i / value_type(nAzimuthSpike);
                polygon.vertex(i) = {radius * cos(phi), radius * sin(phi), value_type(0)};
            }

            // Create spike
            mesh_type mSpike;
            auto scaler = [](g::OrientedPolygon<vector_traits> polygon, value_type t) -> g::OrientedPolygon<vector_traits>
            {
                value_type s0 = value_type(1);
                value_type s1 = value_type(0.1);
                value_type s = lerp(s0, s1, t);
                for (auto& vertex : polygon.vertices())
                {
                    vertex *= s;
                }
                return polygon;
            };

            model::create_swept_volume(mSpike, nameSpike, polygon, path, scaler);
            // set end0 normal to perpendicular instead of following curve ...
            // but it really shouldn't be off that far ...
            // that's probably why the end caps on helical_claws are not aligned properly ...
            model::connect_end_round(mSpike, nameSpike, "end1", "top");

            // Rotate spike to align with face normal and translate spike to face position
            vector_type directionFace = normal;
            vector_type rightFace = normalize(face.halfedge().vertex_attributes().position() - position);
            vector_type directionSpike = path[0].orientation().direction();
            vector_type rightSpike = normalize(cross(path[0].orientation().right(), path[0].orientation().direction()));
            point_type positionSpike = path[0].position();

//            matrix *= alignment2(directionSpike, rightSpike, directionFace, rightFace);
            auto matrix = g::translation(-g::to_vector(positionSpike)) * g::alignment(directionSpike, rightSpike, directionFace, rightFace) * g::translation(to_vector(position));
            model::transform(mSpike, matrix);

            // Connect spike to body
            m.insert(mSpike);
            brep::attach(m, face.id(), m.surface(nameSpikeBase).faces().front().id());
        }

        m.pack();
        mesh.insert(m);
        return;
    }
*/
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
    void create_laminaria(mesh_type& mesh, const string& name, value_type r, value_type dzSpike, size_t nAzimuth, value_type revs, size_t resolutionBase, size_t nAzimuthSpike, size_t ntSpike)
    {
        value_type scale_spike_width = 10.0f;
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

        vector<point_type> control_points_path =
        {
            {0.0f, 0.0f, 0.0f},
            {0.5f, 0.0f, 0.33f},
            {-0.1f, 0.0f, 0.67f},
            {0.25f, 0.0f, 1.0f},
        };

value_type t0 = 0.0f;
value_type t1 = 1.0f;

        g::CurveNURBS<point_type> curve_front(degree, control_points_front);
        g::CurveNURBS<point_type> curve_back(degree, control_points_back);
        auto polygon = create_polygon(curve_back, curve_front, nAzimuthSpike);
        g::OrientedPolygon<vector_traits> opolygon(polygon, {vector_type::z_direction(), vector_type::x_direction()});

        g::CurveNURBS<point_type> curve_path(degree, control_points_path);
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

        mesh_type m;

        model::create_geodesic_sphere(m, name, r, resolutionBase, true, true);
        auto dzBase = r / 3.0f;
        vector_type v {0.0f, 0.0f, dzBase};
        model::transform(m, g::translation(v));
        geometry::HalfSpace<vector_traits> hs({point_type::origin(), -vector_type::z_direction()});
        clip(m, hs, "clip_lower");

        // Hollow out center to save filament
        auto mm = m;
        for (auto& submesh : mm.submeshes())
        {
            submesh.set_name("inner_" + submesh.name());
        }
        for (auto& surface : mm.surfaces())
        {
            surface.set_name("inner_" + surface.name());
        }
        model::transform(mm, geometry::scaling(0.5f));
        invert(mm);
        m.insert(mm);
        brep::attach(m, name + "/clip_lower", "inner_" + name + "/inner_clip_lower");

        model::transform(m, g::translation(-v));

        size_t nSpikes = static_cast<size_t>(nAzimuth * revs);
        for (size_t nSpike = 0; nSpike < nSpikes; ++nSpike)
        {
            string nameSpike = "spike" + to_string(nSpike);
            string nameSpikeBase = nameSpike + "/end0";

            value_type t = value_type(nSpike) / nSpikes;
            value_type scale_spike = 1.0f - t * 0.75f;

            size_t nUndulations = static_cast<size_t>(15.0f * scale_spike) * 2 + 1;

            auto scaler_width = [scale_spike_width, nUndulations](const g::OrientedPolygon<vector_traits>& opolygon, value_type t) -> g::OrientedPolygon<vector_traits>
            {
                value_type s0 = value_type(1);
                value_type s1 = value_type(0.01); // 0 doesn't work with swept volume ...
                value_type factor = lerp(s0, s1, t);
                factor = sqrt(cos(t * math::PiHalf<value_type> * 0.98f));
                factor += 0.075f * sin(t * math::Pi<value_type> * nUndulations);
                factor *= scale_spike_width;
                g::OrientedPolygon<vector_traits> poly({}, opolygon.orientation());
                std::ranges::transform(opolygon.vertices(), std::back_inserter(poly.vertices()), [factor](auto point) { return point * factor; });
                return poly;
            };

// problem with op.position() ...
            g::Path<vector_traits> path_spike = path;
            std::ranges::transform(path_spike, path_spike.begin(), [scale_spike](auto& op) -> g::OrientedPoint<vector_traits> { return {op.position() * scale_spike, op.orientation()}; });

            mesh_type mSpike;
            model::create_swept_volume(mSpike, nameSpike, opolygon, path_spike, scaler_width);
            model::connect_end_round(mSpike, nameSpike, "end1", "top");

            auto rInner = 0.8f * r;
            value_type theta = t * math::PiFourth<value_type>;
            point_type position = {rInner * cos(theta), 0.0f, rInner * sin(theta)};

            value_type phi = t * revs * math::PiTwo<value_type>;
            phi *= value_type(2 * nAzimuth - 1) / (2 * nAzimuth);

            auto matrix = g::translation(to_vector(position)) * g::rotation_z(phi);
            model::transform(mSpike, matrix);

            m.insert(mSpike);
        }

        m.pack();
        mesh.insert(m);
        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    FileValidator validator(dir_out, dir_ref, app_name);

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
//    model::create_extrusion(mesh, name, polygon, vector_type::z_direction(), {0.0f, 0.0f, 50.0f});
    polygon1.transform(g::scaling(30.0f));
    model::create_cylinder(mesh, name, polygon1, 1, 0.0f, 50.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "laminaria_swept_1b";
    cout << name << endl;
    mesh.clear();
//    model::create_extrusion(mesh, name, polygon, vector_type::z_direction(), {0.0f, 0.0f, 50.0f});
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
//    model::create_extrusion(mesh, name, polygon, vector_type::z_direction(), {0.0f, 0.0f, 100.0f});
    polygon2.transform(g::scaling(30.0f));
    model::create_cylinder(mesh, name, polygon2, 1, 0.0f, 50.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "laminaria_20";
    cout << name << endl;
    mesh.clear();
    create_laminaria(mesh, name, 20.0f, 150.0f, 8, 2.0f, 3, 8, 20);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
/*
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
    validator.report_results();
    return 0;
}
