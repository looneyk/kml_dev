//------------------------------------------------------------------------------
// acicula
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshBuilder.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/clip.hpp"
#include "quetzal/brep/connection.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/geometry/HalfSpace.hpp"
#include "quetzal/geometry/intersection.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/model/SurfaceName.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/mesh_attributes.hpp"
#include "quetzal/model/primitives.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <cassert>

#include "quetzal/brep/validation.hpp"

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = double;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_traits = brep::MeshTraits<vector_traits>;
    using mesh_type = brep::Mesh<mesh_traits>;
    using matrix_type = geometry::transformation_matrix_type<vector_traits>;
    using vector_type = mesh_type::vector_type;
    using point_type = mesh_type::point_type;

    namespace g = quetzal::geometry;

    static const filesystem::path dir_out = "out/";
    static const filesystem::path dir_ref = "reference/";
    static const string app_name = "acicula";

    //--------------------------------------------------------------------------
    filesystem::path to_filename(const string& name)
    {
        return dir_out / (name + ".obj");
    }

    //--------------------------------------------------------------------------
    std::string surface_name(const std::string& base, size_t n)
    {
        return std::format("{}{}", base, n);
    }

    //--------------------------------------------------------------------------
    std::string next_surface_name(mesh_type& mesh, const std::string& base_name = model::SurfaceName::BodySection)
    {
        return surface_name(base_name, mesh.surface_store_count());
    }

    //--------------------------------------------------------------------------
    template<typename M>
    void truncate(M& mesh, size_t nAzimuth)
    {
        id_type idLower = 2;
        id_type idUpper = mesh.halfedge_count() - 6 * (nAzimuth + 1) - 2;
        geometry::HalfSpace<vector_traits> hsLower({mesh.vertex(idLower).attributes().position(), {0.0, 0.0, -1.0}});
        geometry::HalfSpace<vector_traits> hsUpper({mesh.vertex(idUpper).attributes().position(), {0.0, 0.0, 1.0}});
        clip(mesh, hsLower, "clip_lower");
        clip(mesh, hsUpper, "clip_upper");
        return;
    }

    //--------------------------------------------------------------------------
    void append_facet(mesh_type& mesh, id_type submesh_id, const std::string& surface_base_name, span<point_type> points)
    {
        auto normal = geometry::normal(points);

        size_t ns = mesh.surface_store_count();
        id_type surface_id = mesh.create_surface(submesh_id, surface_name(surface_base_name, ns), {normal});

        size_t nh = mesh.halfedge_store_count();
        id_type face_id = mesh.create_face(surface_id, nh, {normal});

        size_t n = points.size();
        for (size_t i = 0; i < n; ++i)
        {
            mesh_type::vertex_attributes_type av{*std::next(points.begin(), i), normal, {}};
            id_type partner_id = nullid;
            id_type halfedge_id_next = nh + (i + 1) % n;
            id_type halfedge_id_prev = nh + (n + i - 1) % n;
            mesh.create_halfedge_vertex(partner_id, halfedge_id_next, halfedge_id_prev, face_id, av);
        }

        calculate_face_texcoords(mesh, face_id);
        return;
    }

    //--------------------------------------------------------------------------
    void append_facet(mesh_type& mesh, id_type submesh_id, const std::string& surface_base_name, initializer_list<point_type> points)
    {
        vector<point_type> ps = points;
        append_facet(mesh, submesh_id, surface_base_name, ps);
        return;
    }

    //--------------------------------------------------------------------------
    // returns halfedge ids of unmatched halfedges
    template<typename M>
    vector<id_type> connect_partners(M& mesh)
    {
        using segment_type = std::array<typename M::point_type, 2>;
        std::map<segment_type, id_type> segments;

        for (auto& face : mesh.faces())
        {
            for (auto& halfedge : face.halfedges())
            {
                id_type idHalfedge = halfedge.id();
                const typename M::point_type& position = halfedge.vertex_attributes().position();
                const typename M::point_type& positionNext = halfedge.next().vertex_attributes().position();
                segment_type segment = {positionNext, position};
                auto i = segments.find(segment);
                if (i == segments.end())
                {
                    segments[{position, positionNext}] = idHalfedge;
                }
                else
                {
                    halfedge.set_partner_id(i->second);
                    mesh.halfedge(i->second).set_partner_id(idHalfedge);
                    segments.erase(i);
                }
            }
        }

        vector<id_type> ids;
        for (const auto& segment : segments)
        {
            ids.push_back(segment.second);
        }

        return ids;
    }

    //--------------------------------------------------------------------------
    // r is radius in xy-plane
    // z is height above and below xy-plane, total height is 2 * z
    void create_acicula(mesh_type& mesh, const string& name, size_t nAzimuth, value_type r, value_type z, bool clipped)
    {
        name; // ...

        value_type zLower = -z;
        value_type zUpper = z;

        value_type zLower2 = zLower / 2.0;
        value_type zUpper2 = zUpper / 2.0;

        point_type base{0.0, 0.0, zLower};
        point_type apex{0.0, 0.0, zUpper};

        auto r2 = r / 2.0; // incircle radius
        r2 /= cos(math::Pi<value_type> / nAzimuth); // circumcircle radius
        g::Polygon<vector_traits> polygon1(nAzimuth);
        for (size_t i = 0; i < nAzimuth; ++i)
        {
            auto phi = math::PiTwo<value_type> * (2 * i + 1) / (2 * nAzimuth);
            polygon1.vertex(i) = {r2 * cos(phi), r2 * sin(phi), zLower2};
        }

        g::Polygon<vector_traits> polygon2(nAzimuth);
        for (size_t i = 0; i < nAzimuth; ++i)
        {
            auto phi = math::PiTwo<value_type> * i / nAzimuth;
            polygon2.vertex(i) = {r * cos(phi), r * sin(phi), 0.0};
        }

        g::Polygon<vector_traits> polygon3 = polygon1;
        for (auto& point : polygon3.vertices())
        {
            point.set_z(zUpper2);
        }

        id_type submesh_id = mesh.create_submesh("head");
        std::string surface_base_name = model::SurfaceName::BodySection;

        for (size_t i = 0; i < nAzimuth; ++i)
        {
            append_facet(mesh, submesh_id, surface_base_name, {base, polygon1.vertex(i), polygon2.vertex(i), polygon1.vertex((i + nAzimuth - 1) % nAzimuth)});
        }

        for (size_t i = 0; i < nAzimuth; ++i)
        {
            append_facet(mesh, submesh_id, surface_base_name, {polygon1.vertex(i), polygon3.vertex(i), polygon2.vertex(i)});
            append_facet(mesh, submesh_id, surface_base_name, {polygon1.vertex(i), polygon2.vertex((i + 1) % nAzimuth), polygon3.vertex(i)});
        }

        for (size_t i = 0; i < nAzimuth; ++i)
        {
            append_facet(mesh, submesh_id, surface_base_name, {polygon2.vertex(i), polygon3.vertex(i), apex, polygon3.vertex((i + nAzimuth - 1) % nAzimuth)});
        }

        connect_partners(mesh);

        if (clipped)
        {
            for (size_t i = 0; i < nAzimuth; ++i)
            {
                auto phi = math::PiTwo<value_type> * i / nAzimuth;
                vector_type normal = {cos(phi), sin(phi), 0.0};
                geometry::HalfSpace<vector_traits> hs({normal * (r * 0.75), normal});
                clip(mesh, hs, "clip" + to_string(i));
            }
        }

        geometry::HalfSpace<vector_traits> hsLower({{0.0, 0.0, 0.75 * zLower}, {0.0, 0.0, -1.0}});
        clip(mesh, hsLower, "end0");

        model::create_cylinder_edges(mesh, "stem", 6, 1, 0.5 * r2, 0.5 * r2, 1.25 * zLower, 0.75 * zLower, 0.5 / nAzimuth, 1.0 + 0.5 / nAzimuth);
        brep::weld(mesh, "head/end0", "stem/end1");

        model::create_cylinder_edges(mesh, "base", 6, 1, 1.0 * r2, 0.5 * r2, 1.5 * zLower, 1.25 * zLower, 0.5 / nAzimuth, 1.0 + 0.5 / nAzimuth);
        brep::weld(mesh, "stem/end0", "base/end1");

        mesh.pack();
        return;
    }

    //--------------------------------------------------------------------------
    // r is radius in xy-plane
    // z is height above and below xy-plane, total height is 2 * z
    void build_acicula(mesh_type& mesh, const string& name, size_t nAzimuth, value_type r, value_type z, bool clipped)
    {
        name; // ...

        value_type zLower = -z;
        value_type zUpper = z;

        value_type zLower2 = zLower / 2.0;
        value_type zUpper2 = zUpper / 2.0;

        point_type base{0.0, 0.0, zLower};
        point_type apex{0.0, 0.0, zUpper};

        auto r2 = r / 2.0; // incircle radius
        r2 /= cos(math::Pi<value_type> / nAzimuth); // circumcircle radius
        g::Polygon<vector_traits> polygon1(nAzimuth);
        for (size_t i = 0; i < nAzimuth; ++i)
        {
            auto phi = math::PiTwo<value_type> * (2 * i + 1) / (2 * nAzimuth);
            polygon1.vertex(i) = {r2 * cos(phi), r2 * sin(phi), zLower2};
        }

        g::Polygon<vector_traits> polygon2(nAzimuth);
        for (size_t i = 0; i < nAzimuth; ++i)
        {
            auto phi = math::PiTwo<value_type> * i / nAzimuth;
            polygon2.vertex(i) = {r * cos(phi), r * sin(phi), 0.0};
        }

        g::Polygon<vector_traits> polygon3 = polygon1;
        for (auto& point : polygon3.vertices())
        {
            point.set_z(zUpper2);
        }

        brep::MeshBuilder<mesh_type> builder;

        builder.set_mesh(mesh);
        id_type submesh_id = builder.submesh("head");
        std::vector<point_type> points;

        for (size_t i = 0; i < nAzimuth; ++i)
        {
            points = {base, polygon1.vertex(i), polygon2.vertex(i), polygon1.vertex((i + nAzimuth - 1) % nAzimuth)};
            builder.surface_face(submesh_id, next_surface_name(mesh), geometry::normal(points), points);
        }

        for (size_t i = 0; i < nAzimuth; ++i)
        {
            points = {polygon1.vertex(i), polygon3.vertex(i), polygon2.vertex(i)};
            builder.surface_face(submesh_id, next_surface_name(mesh), geometry::normal(points), points);

            points = {polygon1.vertex(i), polygon2.vertex((i + 1) % nAzimuth), polygon3.vertex(i)};
            builder.surface_face(submesh_id, next_surface_name(mesh), geometry::normal(points), points);
        }

        for (size_t i = 0; i < nAzimuth; ++i)
        {
            points = {polygon2.vertex(i), polygon3.vertex(i), apex, polygon3.vertex((i + nAzimuth - 1) % nAzimuth)};
            builder.surface_face(submesh_id, next_surface_name(mesh), geometry::normal(points), points);
        }

        if (clipped)
        {
            for (size_t i = 0; i < nAzimuth; ++i)
            {
                auto phi = math::PiTwo<value_type> * i / nAzimuth;
                vector_type normal = {cos(phi), sin(phi), 0.0};
                geometry::HalfSpace<vector_traits> hs({normal * (r * 0.75), normal});
                clip(mesh, hs, "clip" + to_string(i));
            }
        }

        geometry::HalfSpace<vector_traits> hsLower({{0.0, 0.0, 0.75 * zLower}, {0.0, 0.0, -1.0}});
        clip(mesh, hsLower, "end0");

        model::create_cylinder_edges(mesh, "stem", 6, 1, 0.5 * r2, 0.5 * r2, 1.25 * zLower, 0.75 * zLower, 0.5 / nAzimuth, 1.0 + 0.5 / nAzimuth);
        brep::weld(mesh, "head/end0", "stem/end1");

        model::create_cylinder_edges(mesh, "base", 6, 1, 1.0 * r2, 0.5 * r2, 1.5 * zLower, 1.25 * zLower, 0.5 / nAzimuth, 1.0 + 0.5 / nAzimuth);
        brep::weld(mesh, "stem/end0", "base/end1");

        mesh.pack();
        return;
    }

    //--------------------------------------------------------------------------
    void create_acicula2(mesh_type& mesh, const string& name, size_t nAzimuth, value_type r, value_type z, bool clipped)
    {
        name; // ...

        auto z0 = 0.0;
        auto z1 = 0.3 * z;
        auto z2 = 0.4 * z;
        auto z3 = 0.6 * z;
        auto z4 = z;

// spike vanes
//        z3 = z;
//        z4 = 0.6 * z;

//        auto r1 = 0.25 * r;
        auto r1 = 0.33 * r;
//        auto r2 = r1 / cos(math::Pi<value_type> / nAzimuth); // circumcircle radius
//        auto r2 = r1;
        auto r2 = 0.66 * r;
//        auto r2 = 0.6 * r1;
        auto r3 = r;

        point_type apex{0.0, 0.0, z4};

        g::Polygon<vector_traits> polygon1(nAzimuth);
        for (size_t i = 0; i < nAzimuth; ++i)
        {
            auto phi = math::PiTwo<value_type> * i / nAzimuth;
            polygon1.vertex(i) = {r1 * cos(phi), r1 * sin(phi), z1};
        }

        g::Polygon<vector_traits> polygon2(nAzimuth);
        for (size_t i = 0; i < nAzimuth; ++i)
        {
            auto phi = math::PiTwo<value_type> * (2 * i + 1) / (2 * nAzimuth);
            polygon2.vertex(i) = {r2 * cos(phi), r2 * sin(phi), z2};
        }

        g::Polygon<vector_traits> polygon3(nAzimuth);
        for (size_t i = 0; i < nAzimuth; ++i)
        {
            auto phi = math::PiTwo<value_type> * i / nAzimuth;
            polygon3.vertex(i) = {r3 * cos(phi), r3 * sin(phi), z3};
//            polygon3.vertex(i) = {r3 * cos(phi), r3 * sin(phi), z3 * (0.75 + 0.25 * i / (nAzimuth - 1))}; // helix height ...

            // adjust polygon3 point so that the points defining the face are coplanar
            std::vector<point_type> plane_points = {polygon1.vertex(i), polygon2.vertex(i), polygon2.vertex((i + nAzimuth - 1) % nAzimuth)};
            g::Plane<vector_traits> plane(plane_points[0], g::normal(plane_points));
            std::vector<point_type> line_points = {apex, polygon3.vertex(i)};
            g::Line<vector_traits> line(line_points[0], normalize(line_points[1] - line_points[0]));
            auto intersection_point = geometry::intersection(plane, line);

            // interesting with two half facets and polygon3 vertices much further out
//            auto t = line.projection_parameter(intersection_point.point());
//            polygon3.vertex(i) = line.point() + line.direction() * t * 1.3;

            assert(intersection_point.locus() == g::Locus::Point);
            polygon3.vertex(i) = intersection_point.point();
        }

        auto polygon01 = polygon1;
        for (size_t i = 0; i < nAzimuth; ++i)
        {
            polygon01.vertex(i).set_z(z0);
        }

        id_type submesh_id = mesh.create_submesh("head");
        std::string surface_base_name = model::SurfaceName::BodySection;

        for (size_t i = 0; i < nAzimuth; ++i)
        {
            append_facet(mesh, submesh_id, surface_base_name, {apex, polygon2.vertex((i + nAzimuth - 1) % nAzimuth), polygon3.vertex(i)});
//            append_facet(mesh, submesh_id, surface_base_name, {polygon2.vertex((i + nAzimuth - 1) % nAzimuth), polygon1.vertex(i), polygon3.vertex(i)});

            append_facet(mesh, submesh_id, surface_base_name, {apex, polygon3.vertex(i), polygon2.vertex(i)});
//            append_facet(mesh, submesh_id, surface_base_name, {polygon3.vertex(i), polygon1.vertex(i), polygon2.vertex(i)});

            // single face replacing the two above
            // use the above without the polygon3 adjustment for the interesting difference
            append_facet(mesh, submesh_id, surface_base_name, {polygon1.vertex(i), polygon2.vertex(i), polygon3.vertex(i), polygon2.vertex((i + nAzimuth - 1) % nAzimuth)});

            append_facet(mesh, submesh_id, surface_base_name, {polygon1.vertex(i), polygon1.vertex((i + 1) % nAzimuth), polygon2.vertex(i)});
        }

        auto ids = connect_partners(mesh);
        id_type surface_id_base = mesh.create_surface(submesh_id, "end0");
        create_border_face(mesh, ids[0], {0.0, 0.0, -1.0}, surface_id_base);

        for (size_t i = 0; i < nAzimuth; ++i)
        {
            if (clipped)
            {
                auto phi = math::PiTwo<value_type> * i / nAzimuth;
                vector_type normal = {cos(phi), sin(phi), 0.0};
                normal.normalize();
                geometry::HalfSpace<vector_traits> hs({normal * (r3 * 0.75), normal});
                clip(mesh, hs, "side" + to_string(i));
            }
/*
            normal.set_z(2.0);
            normal.normalize();
            hs.set_plane({normal * (2.3 * r3), normal});
            clip(mesh, hs, "peak" + to_string(i));
*/
        }

        model::create_cylinder(mesh, "stem", 6, 1, r1, r1, 0.1 * z, 0.3 * z);
        brep::weld(mesh, "head/end0", "stem/end1");

        model::create_cylinder(mesh, "base", 6, 1, 1.5 * r1, r1, 0.0, 0.1 * z);
        brep::weld(mesh, "stem/end0", "base/end1");

        mesh.pack();
        return;
    }

    //--------------------------------------------------------------------------
    void create_pyramid_arrangement(mesh_type& mesh, const string& name, size_t nAzimuthOuter, value_type rOuter0, value_type rOuter1, size_t nAzimuthInner, value_type rInner, value_type z1)
    {
        mesh_type m;
        id_type submesh_id0 = model::create_cylinder(m, "proto", nAzimuthInner, 1, rInner, 0.0, 0.0, z1);

        for (size_t i = 0; i < nAzimuthOuter; ++i)
        {
            id_type submesh_id = mesh.insert(m, submesh_id0, std::format("{}_{}", name, i));
            auto submesh = mesh.submesh(submesh_id);

            auto phi = math::PiTwo<value_type> * (value_type(i) / nAzimuthOuter);
            model::transform(submesh, [rOuter0, rOuter1, z1, phi](typename mesh_type::vertex_type& vertex) {
                if (vertex.marked())
                {
                    return;
                }
                auto position = vertex.attributes().position();
                auto t = position.z() / z1;
                auto r = lerp(rOuter0, rOuter1, t);
                auto matrix = g::rotation_z(phi) * g::translation(r * cos(phi), r * sin(phi), 0.0);
                position *= matrix;
                for (auto& halfedge : vertex.halfedges())
                {
                    halfedge.vertex_attributes().set_position(position);
                    halfedge.vertex().set_marked();
                }
                return;
            });

            // union? ...
        }

        return;
    }

    //--------------------------------------------------------------------------
    void create_pyramid_arrangement2(mesh_type& mesh, const string& name, size_t nAzimuthOuter, value_type rOuter0, value_type rOuter1, value_type rPlacement0, value_type rPlacement1, size_t nAzimuthInner, value_type rInner0, value_type rInner1, value_type z1, bool bRevs = false)
    {
        model::create_cylinder(mesh, name, nAzimuthOuter, 1, rOuter0, rOuter1, 0.0, z1);

        auto revs = bRevs ? value_type(1) / (2 * nAzimuthInner) : value_type(0);
        mesh_type m;
        id_type submesh_id0 = model::create_cylinder(m, "proto", nAzimuthInner, 1, rInner0, rInner1, 0.0, z1, revs, revs);

        for (size_t i = 0; i < nAzimuthOuter; ++i)
        {
            id_type submesh_id = mesh.insert(m, submesh_id0, std::format("{}_{}", name, i));
            auto submesh = mesh.submesh(submesh_id);

            auto phi = math::PiTwo<value_type> * (value_type(2 * i + 1) / (2 * nAzimuthOuter));
            model::transform(submesh, [rPlacement0, rPlacement1, z1, phi](typename mesh_type::vertex_type& vertex) {
                if (vertex.marked())
                {
                    return;
                }
                auto position = vertex.attributes().position();
                auto t = position.z() / z1;
                auto r = lerp(rPlacement0, rPlacement1, t);
                auto matrix = g::rotation_z(math::Pi<value_type>) * g::translation(r, 0.0, 0.0) * g::rotation_z(phi);
                position *= matrix;
                for (auto& halfedge : vertex.halfedges())
                {
                    halfedge.vertex_attributes().set_position(position);
                    halfedge.vertex().set_marked();
                }
                return;
            });

            // union? ...
        }

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

    name = "acicula_30_60_sharp";
    cout << name << endl;
    mesh.clear();
    create_acicula(mesh, name, 6, 30.0, 60.0, false);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "acicula_30_60_flat";
    cout << name << endl;
    mesh.clear();
    create_acicula(mesh, name, 6, 30.0, 60.0, true);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "acicula_45_45_sharp";
    cout << name << endl;
    mesh.clear();
    create_acicula(mesh, name, 6, 45.0, 45.0, false);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "acicula_45_45_flat";
    cout << name << endl;
    mesh.clear();
    create_acicula(mesh, name, 6, 45.0, 45.0, true);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "acicula_60_30_sharp";
    cout << name << endl;
    mesh.clear();
    create_acicula(mesh, name, 6, 60.0, 30.0, false);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "acicula_60_30_flat";
    cout << name << endl;
    mesh.clear();
    create_acicula(mesh, name, 6, 60.0, 30.0, true);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "acicula2_sharp";
    cout << name << endl;
    mesh.clear();
    create_acicula2(mesh, name, 6, 33.0, 100.0, false);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "acicula2_flat";
    cout << name << endl;
    mesh.clear();
    create_acicula2(mesh, name, 6, 33.0, 100.0, true);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "acicula_30_60_sharp_b";
    cout << name << endl;
    mesh.clear();
    build_acicula(mesh, name, 6, 30.0, 60.0, false);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "acicula_30_60_flat_b";
    cout << name << endl;
    mesh.clear();
    build_acicula(mesh, name, 6, 30.0, 60.0, true);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "acicula_45_45_sharp_b";
    cout << name << endl;
    mesh.clear();
    build_acicula(mesh, name, 6, 45.0, 45.0, false);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "acicula_45_45_flat_b";
    cout << name << endl;
    mesh.clear();
    build_acicula(mesh, name, 6, 45.0, 45.0, true);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "acicula_60_30_sharp_b";
    cout << name << endl;
    mesh.clear();
    build_acicula(mesh, name, 6, 60.0, 30.0, false);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "acicula_60_30_flat_b";
    cout << name << endl;
    mesh.clear();
    build_acicula(mesh, name, 6, 60.0, 30.0, true);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
/*
    name = "acicula2_sharp_b";
    cout << name << endl;
    mesh.clear();
    build_acicula2(mesh, name, 6, 33.0, 100.0, false);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "acicula2_flat_b";
    cout << name << endl;
    mesh.clear();
    build_acicula2(mesh, name, 6, 33.0, 100.0, true);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
*/
    name = "pyramids_6_3_0";
    cout << name << endl;
    mesh.clear();
    create_pyramid_arrangement(mesh, name, 6, 30.0, 0.0, 3, 30.0, 100.0);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "pyramids_6_3_1";
    cout << name << endl;
    mesh.clear();
    create_pyramid_arrangement(mesh, name, 6, 30.0, 30.0, 3, 30.0, 100.0);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "pyramids_6_3_2";
    cout << name << endl;
    mesh.clear();
    create_pyramid_arrangement2(mesh, name, 6, 30.0, 0.0, 30.0, 0.0, 3, 15.0, 0.0, 100.0);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "pyramids_4_3_2";
    cout << name << endl;
    mesh.clear();
    create_pyramid_arrangement2(mesh, name, 4, 30.0, 0.0, 30.0, 0.0, 3, 15.0, 0.0, 100.0);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "pyramids_6_6_2a";
    cout << name << endl;
    mesh.clear();
    create_pyramid_arrangement2(mesh, name, 6, 40.0, 0.0, 35.0, 0.0, 6, 15.0, 0.0, 100.0);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "pyramids_6_6_2b";
    cout << name << endl;
    mesh.clear();
    create_pyramid_arrangement2(mesh, name, 6, 40.0, 0.0, 35.0, 0.0, 6, 15.0, 0.0, 100.0, true);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    return validator.report_results();
}
