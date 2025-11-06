//------------------------------------------------------------------------------
// helical_claws
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/clip.hpp"
#include "quetzal/brep/connection.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/geometry/HalfSpace.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/math/math_util.hpp"
#include "quetzal/math/parametric.hpp"
#include "quetzal/model/helix_anticylinder.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <limits>
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

    static const string dirOut = "out/";
    static const string dirReference = "reference/";

    //--------------------------------------------------------------------------
    string to_filename(const string& name)
    {
        return dirOut + name + ".obj";
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

        T q = T(1) + kEgg * (x - T(0.5));
        return T(0.5) * wEgg * ((1 - 2 * x) / q - kEgg * (x - x * x) / (q * q)) / sqrt((x - x * x) / q);
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
        geometry::HalfSpace<vector_traits> hsLower({mesh.vertex(idLower).attributes().position(), {0.0f, 0.0f, -1.0f}});
        geometry::HalfSpace<vector_traits> hsUpper({mesh.vertex(idUpper).attributes().position(), {0.0f, 0.0f, 1.0f}});
        clip(mesh, hsLower, "clip_lower");
        clip(mesh, hsUpper, "clip_upper");
        return;
    }

    //--------------------------------------------------------------------------
    template<typename M>
    void check_mesh(const M& mesh)
    {
        cout << "checking" << endl;
        
        size_t ne = 0;
        for (const auto& halfedge : mesh.halfedges())
        {
            id_type idPartner = halfedge.partner_id();
            if (idPartner != nullid)
            {
                if (mesh.halfedge(idPartner).partner_id() != halfedge.id())
                {
                    cout << "halfedge partner mismatch " << halfedge.id() << " p " << idPartner << " pp " << mesh.halfedge(idPartner).partner_id() << endl;
                    ++ne;
                }
            }
        }
        cout << ne << " partner mismatches" << endl;

        ne = 0;
        for (const auto& vertex : mesh.vertices())
        {
            auto position = vertex.attributes().position();

            for (const auto& halfedge : vertex.halfedges())
            {
                if (halfedge.vertex_attributes().position() != position)
                {
                    cout << "vertex position mismatch " << halfedge.id() << " pos " << halfedge.vertex_attributes().position() << " ref " << position << endl;
                    ++ne;
                }
            }
        }
        cout << ne << " vertex position mismatches" << endl;

        ne = 0;
        for (const auto& vertex : mesh.vertices())
        {
            auto position = vertex.attributes().position();
            if (isnan(position[0]) || isnan(position[1]) || isnan(position[2]))
            {
                cout << "vertex position nan " << vertex.id() << " pos " << vertex.attributes().position() << endl;
                ++ne;
            }

            auto normal = vertex.attributes().normal();
            if (isnan(normal[0]) || isnan(normal[1]) || isnan(normal[2]))
            {
                cout << "vertex normal nan " << vertex.id() << " pos " << vertex.attributes().normal() << endl;
                ++ne;
            }

            auto texcoord = vertex.attributes().texcoord();
            if (isnan(texcoord[0]) || isnan(texcoord[1]))
            {
                cout << "vertex texcoord nan " << vertex.id() << " pos " << vertex.attributes().texcoord() << endl;
                ++ne;
            }
        }
        cout << ne << " vertex nans" << endl;

        return;
    }

    //--------------------------------------------------------------------------
    void dump_mesh(const mesh_type& mesh, const std::string& name, const std::string& text = "")
    {
        cout << text << endl;
        cout << "mesh: " << name << endl;
        for (const auto& submesh : mesh.submeshes())
        {
            cout << "  submesh " << submesh.id() << " " << submesh.name() << " " << (submesh.deleted() ? "D" : "") << endl;
            for (const auto& surface : submesh.surfaces())
            {
                cout << "    surface " << surface.id() << " " << surface.name() << " " << (surface.deleted() ? "D" : "") << endl;
            }
        }
        cout << "  surface" << endl;
        for (const auto& surface : mesh.surfaces())
        {
            cout << "    " << surface.id() << " " << surface.name() << " " << surface.face_count() << " faces " << (surface.deleted() ? "D" : "") << endl;
        }
        cout << "  submesh index" << endl;
        for (const auto& e : mesh.submesh_index())
        {
            cout << "    " << e.first << " " << e.second << endl;
        }
        cout << "  surface index" << endl;
        for (const auto& e : mesh.surface_index())
        {
            cout << "    " << e.first << " " << e.second << endl;
        }

        return;
    }

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
check_mesh(m);
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
            vector_type right = {0.0f, 1.0f, 0.0f};
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
//                path[i] = {pos * factor, {normalize(direction), right}};
                path[i] = {scale(pos, factor), {normalize(direction), right}};
//                path[i] = {g::to_point(to_vector(pos) * factor), {normalize(direction), right}};
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

                auto& opoint = path[i];
                opoint.position() += positionSpikeRadial - positionSpikeOrig;
//const auto positionOrig = opoint.position();
                opoint.position() *= matrix;

                auto& orientation = opoint.orientation();
// not taking dphi (but not that simple) into account ...
//                orientation.direction() = normalize(orientation.direction() * matrix);
auto md = g::rotation_z(phi * 0.5f);
orientation.direction() = normalize(orientation.direction() * md); // md, not matrix? ...

//vector_type axis = {opoint.position().x(), opoint.position().y(), 0.0f};
//orientation.direction() = normalize(orientation.direction() * g::rotation_axis_unit(normalize(axis), -0.5f));

// this the same as simple matrix multiplication
//orientation.direction() = normalize((positionOrig + orientation.direction()) * matrix - opoint.position());

//R' * p + R * p'
//orientation.direction() = normalize(g::rotation_dz(phi) * to_vector(opoint.position()) + matrix * orientation.direction());

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
            model::connect_end_round(mSpike, nameSpike, "end1", "top");

            // Rotate spike to align with face normal and translate spike to face position
            vector_type directionFace = normal;
            vector_type rightFace = normalize(face.halfedge().vertex_attributes().position() - position);
            vector_type directionSpike = path[0].orientation().direction();
            vector_type rightSpike = normalize(cross(path[0].orientation().right(), path[0].orientation().direction()));
            point_type positionSpike = path[0].position();

//            matrix *= alignment2(directionSpike, rightSpike, directionFace, rightFace);
            auto matrix = g::translation(-to_vector(positionSpike)) * g::alignment(directionSpike, rightSpike, directionFace, rightFace) * g::translation(to_vector(position));
            model::transform(mSpike, matrix);

            // Connect spike to body
            m.insert(mSpike);
            brep::attach(m, face.id(), m.surface(nameSpikeBase).faces().front().id());
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
/*
for (value_type x = 0.0f; x <= 1.0f; x += 0.01f)
{
    value_type estimated = numeric_limits<value_type>::infinity();
    if (x > 0.0f && x < 1.0f)
    {
        estimated = (egg_contour(x + 0.01f) - egg_contour(x - 0.01f)) / 0.02f;
    }
    cout << x << "\t" << egg_contour(x) << "\t" << estimated << "\t" << egg_contour_tangent(x) << endl;
}

for (value_type m = 5.0f; m >= -5.0f; m -= 0.1f)
{
    value_type x = egg_contour_inverse_tangent(m);
    value_type mm = egg_contour_tangent(x);
    cout << x << "\t" << mm << "\t" << m << endl;
}
*/
    FileValidator validator(dirOut, dirReference);

    string name;
    mesh_type mesh;
//    matrix_type matrix;
/*
    size_t nAzimuth = 8;

    name = "helix_anticylinder";
    cout << name << endl;
    mesh.clear();
    model::create_helix_anticylinder(mesh, name, nAzimuth, 40.0f, 30.0f, 0.0f, 100.0f, 0.0f, 4.0f, true);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "helix_anticylinder_clipped";
    cout << name << endl;
    truncate(mesh, nAzimuth);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
*/

    // Want better row distinction/separation
    // see if increasing height of base helps, else consider helix_anticylinder with multi-helix support

    name = "helical_claws_4_10_0250";
    cout << name << endl;
    mesh.clear();
    create_helical_claws(mesh, name, 8, 50.0f, 100.0f, 4.25f, 6, 10, 0.250f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "helical_claws_4_10_0375";
    cout << name << endl;
    mesh.clear();
    create_helical_claws(mesh, name, 8, 50.0f, 100.0f, 4.25f, 6, 10, 0.375f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "helical_claws_4_10_0500";
    cout << name << endl;
    mesh.clear();
    create_helical_claws(mesh, name, 8, 50.0f, 100.0f, 4.25f, 6, 10, 0.500f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
/*
    name = "helical_claws_4_100_0250";
    cout << name << endl;
    mesh.clear();
    create_helical_claws(mesh, name, 8, 50.0f, 100.0f, 4.25f, 40, 100, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "helical_claws_4_100_0375";
    cout << name << endl;
    mesh.clear();
    create_helical_claws(mesh, name, 8, 50.0f, 100.0f, 4.25f, 40, 100, 0.375f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "helical_claws_6_10_0250";
    cout << name << endl;
    mesh.clear();
    create_helical_claws(mesh, name, 8, 50.0f, 100.0f, 6.25f, 6, 10, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "helical_claws_8_10_0250";
    cout << name << endl;
    mesh.clear();
    create_helical_claws(mesh, name, 8, 80.0f, 100.0f, 8.25f, 6, 10, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
*/
    name = "azimuth_test";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 4, 4, 20.0f, 20.0f, 0.0f, 40.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    geometry::Polygon<vector_traits> polygon =
    {
        {20.0f, 0.0f, 0.0f},
        {0.0f, 20.0f, 0.0f},
        {-20.0f, 0.0f, 0.0f},
        {0.0f, -20.0f, 0.0f},
    };

    name = "revs_test";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, polygon, 4, 0.0f, 40.0f, 1.0f, 1.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    validator.report_results();
    return 0;
}
