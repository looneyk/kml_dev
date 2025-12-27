//------------------------------------------------------------------------------
// spoctahedron
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/connection.hpp"
#include "quetzal/brep/triangulation.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/geometry/points_util.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/model/mesh_attributes.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include <filesystem>
#include <iostream>
#include <string>
#include <cassert>

#include "quetzal/brep/validation.hpp"

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;
    using vector_type = mesh_type::vector_type;
    using point_type = mesh_type::point_type;

    static const filesystem::path dir_out = "out/";
    static const filesystem::path dir_ref = "reference/";
    static const string app_name = "spoctahedron";

    //--------------------------------------------------------------------------
    filesystem::path to_filename(const string& name)
    {
        return dir_out / (name + ".obj");
    }

    //--------------------------------------------------------------------------
    // Be sure that flags are reset before the first iteration of this recursive procedure
    void subdivide_spike(mesh_type& mesh, id_type idFace, float scale, size_t nLevels)
    {
        if (nLevels == 0)
        {
            return;
        }

        vector<id_type> split_ids;

        id_type idHalfedgeOrig = mesh.face(idFace).halfedge_id();
        id_type idHalfedge = idHalfedgeOrig;
        do
        {
            if (mesh.halfedge(idHalfedge).marked())
            {
                split_ids.push_back(idHalfedge);
            }
            else if (!mesh.halfedge(idHalfedge).checked())
            {
                split_edge(mesh, idHalfedge);
                mesh.halfedge(idHalfedge).partner().set_marked();
                mesh.halfedge(idHalfedge).partner().set_checked();
                mesh.halfedge(idHalfedge).next().partner().set_checked();
                idHalfedge = mesh.halfedge(idHalfedge).next_id();
                split_ids.push_back(idHalfedge);
            }

            idHalfedge = mesh.halfedge(idHalfedge).next_id();
        } while (idHalfedge != idHalfedgeOrig);


        // This ordering of halfedges in split_edge should ensure that the original face is in the center
        size_t n = split_ids.size();
        for (size_t i = 0; i < n; ++i)
        {
            if (i == n - 1)
            {
                split_face(mesh, mesh.halfedge(split_ids[0]).prev().partner_id(), split_ids[i]);
            }
            else
            {
                split_face(mesh, split_ids[i + 1], split_ids[i]);
            }
        }

        float a = length(mesh.face(idFace).halfedge());
        float h = a * sqrt(6.0f) / 3.0f;

        size_t nFacesOrig = mesh.face_store_count();
        auto position = centroid(mesh.face(idFace)) + mesh.face(idFace).attributes().normal() * scale * h;
        triangulate_central_vertex(mesh, idFace, position, false);

        for (size_t i = 0; i < n; ++i)
        {
            subdivide_spike(mesh, nFacesOrig + i, scale, nLevels - 1);
        }

        return;
    }

    //--------------------------------------------------------------------------
    void create_spoctahedron(mesh_type& mesh, float radius, float scale, size_t nLevels)
    {
        id_type body_id = model::create_octahedron(mesh, "body", radius, false);
        auto& body = mesh.submesh(body_id);

        float zBottom = body.surface("body0").faces().front().halfedge().vertex_attributes().position().z();
        float zTop = body.surface("body6").faces().front().halfedge().vertex_attributes().position().z();
        model::transform(body, geometry::translation(0.0f, 0.0f, zTop - 2.0f * zBottom));

        auto face_ids = body.face_ids();
        for (id_type face_id : face_ids | std::views::drop(1))
        {
            subdivide_spike(mesh, face_id, scale, nLevels);
        }

        // just a test, but better not to use an octahedron for base ...
        id_type base_id = model::create_octahedron(mesh, "base", radius, false);
        auto& base = mesh.submesh(base_id);

        model::transform_positions(base, [](typename mesh_type::traits_type::point_type position) -> typename mesh_type::traits_type::point_type
        {
            if (position.z() > 0.0f)
            {
                position.set_x(position.x() * 0.49f);
                position.set_y(position.y() * 0.49f);
            }
            return position;
        });

        auto zz = zTop - zBottom;
        zBottom = base.surface("body0").faces().front().halfedge().vertex_attributes().position().z();
        assert(zz == base.surface("body6").faces().front().halfedge().vertex_attributes().position().z() - zBottom);
        model::transform(base, geometry::translation(0.0f, 0.0f, -zBottom));
        assert(zz == base.surface("body6").faces().front().halfedge().vertex_attributes().position().z());

        // fix attach so that I don't need to do the scaling above (edge intersections?) ...
        attach(mesh, "body/body0", "base/body6");

        mesh.pack();
        mesh.check();
        return;
    }

    //--------------------------------------------------------------------------
    void create_spectrahedron(mesh_type& mesh, float radius, float scale, size_t nLevels)
    {
        id_type body_id = model::create_tetrahedron(mesh, "body", radius);
        auto& body = mesh.submesh(body_id);

        float z_body_bottom = body.surface(0).faces().front().halfedge().vertex_attributes().position().z();

        auto face_ids = body.face_ids();
        for (id_type face_id : face_ids | std::views::drop(1))
        {
            subdivide_spike(mesh, face_id, scale, nLevels);
        }

        value_type z_base_top = radius * 0.75f;
        model::transform(body, geometry::translation(0.0f, 0.0f, z_base_top - z_body_bottom));

        model::create_anticylinder(mesh, "base", 3, 1, radius * 0.5f, radius * 0.25f, 0.0f, z_base_top);
        attach(mesh, "body/body0", "base/end1");

        mesh.pack();
        mesh.check();
        return;
    }

    //--------------------------------------------------------------------------
    void create_octahedron_cube(mesh_type& mesh, float radius, float scale)
    {
        id_type body_id = model::create_octahedron(mesh, "body", radius, true);

        auto face_ids = mesh.submesh(body_id).face_ids();
        for (id_type face_id : face_ids)
        {
            subdivide_spike(mesh, face_id, scale, 1);
        }

        mesh.check();
        return;
    }

    //--------------------------------------------------------------------------
    void create_stellated_octahedron(mesh_type& mesh, float radius, bool bVertex = true)
    {
        id_type body_id = model::create_octahedron(mesh, "body", radius, !bVertex);

        auto face_ids = mesh.submesh(body_id).face_ids();
        for (id_type face_id : face_ids)
        {
            const auto& face = mesh.face(face_id);

            point_type position;
            for (const auto& halfedge : face.halfedges())
            {
                position += to_vector(halfedge.vertex_attributes().position());
            }

            triangulate_central_vertex(mesh, face.id(), position, false);
        }

        mesh.check();
        return;
    }

    //--------------------------------------------------------------------------
    void create_stellated_octahedron(mesh_type& mesh, float radius, float d, bool bVertex = true)
    {
        id_type body_id = model::create_octahedron(mesh, "body", radius, !bVertex);

        auto face_ids = mesh.submesh(body_id).face_ids();
        for (id_type face_id : face_ids)
        {
            const auto& face = mesh.face(face_id);
            point_type position = centroid(face) + d * face.attributes().normal();
            triangulate_central_vertex(mesh, face.id(), position, false);
        }

        mesh.check();
        return;
    }

    //--------------------------------------------------------------------------
    void create_spiketahedron(mesh_type& mesh, float radius)
    {
        size_t nf_octahedron = mesh.face_count();
        size_t face_count = 8;
        model::create_octahedron(mesh, "body", radius, false);

        float s3 = sqrt(3.0f);
        float a = radius * sqrt(2.0f);
        float ri = radius / s3;
        point_type bottom;

        for (size_t i = 0; i < face_count; ++i) // octahedron.face_ids(), need descriptive octahedron surface names first ...
        {
            id_type face_id = nf_octahedron + i;
            const auto& face = mesh.face(face_id);

            if (i == 0) // base (octahedron surface names?) ...
            {
                bottom = face.halfedges().front().vertex_attributes().position();
                continue;
            }

            point_type position;
            if (i == 6) // top (octahedron surface names?) ...
            {
                position = centroid(face) + face.attributes().normal() * a * s3;
            }
            else
            {
                for (const auto& halfedge : face.halfedges())
                {
                    position += to_vector(halfedge.vertex_attributes().position());
                }
            }

            triangulate_central_vertex(mesh, face_id, position, false);
        }

        float h = ri * 2.0f;
        float z0 = bottom.z();
        bottom.set_z(0.0f);
        float r1 = to_vector(bottom).norm();
        float r0 = r1 * (1 + h / (a * s3));

        model::create_cylinder(mesh, "base", 3, 1, r0, r1, z0 - h, z0);
        weld(mesh, "body/body0", "base/end1"); // bottom face of octahedron body and top face of base

        mesh.pack();
        mesh.check();
        return;
    }

    //--------------------------------------------------------------------------
    void create_snowflake(mesh_type& mesh, float radius, float scale, unsigned int nLevels)
    {
        id_type body_id = model::create_tetrahedron(mesh, "body", radius);

        auto face_ids = mesh.submesh(body_id).face_ids();
        for (id_type face_id : face_ids)
        {
            subdivide_spike(mesh, face_id, scale, nLevels);
        }

        mesh.check();
        return;
    }

    //--------------------------------------------------------------------------
    void create_hollow_tetrahedron(mesh_type& mesh, float radius, float ratio)
    {
        model::create_tetrahedron(mesh, "tetrahedron", radius);

        vector_type t0 = {0.0f, 0.0f, 1.0f};
        vector_type c0 = math::normalize<vector_traits>({1.0f, 1.0f, 1.0f});
        auto right = normalize(cross(c0, t0));
        auto matrix = geometry::alignment(c0, right, t0, right) * geometry::rotation_z(3.0f * math::PiFourth<float>);

        float side = radius * ratio * 2.0f / sqrt(3.0f);
        id_type cube_id = model::create_box(mesh, "cube", side, side, side);
        model::transform(mesh.submesh(cube_id), matrix);

        mesh.check();
        return;
    }
/*
    //--------------------------------------------------------------------------
    void create_cross3_axial(mesh_type& mesh, float ro, float ri)
    {
        mesh_type m;

        float roo = ro - ri;
        float rii = ri / 2.0f;
        float n2 = sqrt(2.0f) / 2.0f;
        float n3 = sqrt(3.0f) / 3.0f;

        // positive z, first quadrant
        auto nv = m.m_vertices.size();
        m.create_vertex({{rii, rii, rii}, {n2, n2, 0.0f}, {0.0f, 0.0f}});
        m.create_vertex({{ri, 0.0f, ri}, {n2, n2, 0.0f}, {0.0f, 0.0f}});
        m.create_vertex({{0.0f, ri, ri}, {n2, n2, 0.0f}, {0.0f, 0.0f}});
        m.create_vertex({{ri, 0.0f, roo}, {n2, n2, 0.0f}, {0.0f, 0.0f}});
        m.create_vertex({{0.0f, ri, roo}, {n2, n2, 0.0f}, {0.0f, 0.0f}});
        m.create_vertex({{ri, 0.0f, roo}, {n3, n3, n3}, {0.0f, 0.0f}});
        m.create_vertex({{0.0f, ri, roo}, {n3, n3, n3}, {0.0f, 0.0f}});
        m.create_vertex({{0.0f, 0.0f, ro}, {n3, n3, n3}, {0.0f, 0.0f}});
        m.append_indices({nv + 0, nv + 2, nv + 1, nv + 1, nv + 2, nv + 4, nv + 4, nv + 3, nv + 1, nv + 5, nv + 6, nv + 7});

        // positive z, second quadrant
        nv = m.m_vertices.size();
        m.create_vertex({{-rii, rii, rii}, {-n2, n2, 0.0f}, {0.0f, 0.0f}});
        m.create_vertex({{0.0f, ri, ri}, {-n2, n2, 0.0f}, {0.0f, 0.0f}});
        m.create_vertex({{-ri, 0.0f, ri}, {-n2, n2, 0.0f}, {0.0f, 0.0f}});
        m.create_vertex({{0.0f, ri, roo}, {-n2, n2, 0.0f}, {0.0f, 0.0f}});
        m.create_vertex({{-ri, 0.0f, roo}, {-n2, n2, 0.0f}, {0.0f, 0.0f}});
        m.create_vertex({{0.0f, ri, roo}, {-n3, n3, n3}, {0.0f, 0.0f}});
        m.create_vertex({{-ri, 0.0f, roo}, {-n3, n3, n3}, {0.0f, 0.0f}});
        m.create_vertex({{0.0f, 0.0f, ro}, {-n3, n3, n3}, {0.0f, 0.0f}});
        m.append_indices({nv + 0, nv + 2, nv + 1, nv + 1, nv + 2, nv + 4, nv + 4, nv + 3, nv + 1, nv + 5, nv + 6, nv + 7});

        // positive z, third quadrant
        nv = m.m_vertices.size();
        m.create_vertex({{-rii, -rii, rii}, {-n2, -n2, 0.0f}, {0.0f, 0.0f}});
        m.create_vertex({{-ri, 0.0f, ri}, {-n2, -n2, 0.0f}, {0.0f, 0.0f}});
        m.create_vertex({{0.0f, -ri, ri}, {-n2, -n2, 0.0f}, {0.0f, 0.0f}});
        m.create_vertex({{-ri, 0.0f, roo}, {-n2, -n2, 0.0f}, {0.0f, 0.0f}});
        m.create_vertex({{0.0f, -ri, roo}, {-n2, -n2, 0.0f}, {0.0f, 0.0f}});
        m.create_vertex({{-ri, 0.0f, roo}, {-n3, -n3, n3}, {0.0f, 0.0f}});
        m.create_vertex({{0.0f, -ri, roo}, {-n3, -n3, n3}, {0.0f, 0.0f}});
        m.create_vertex({{0.0f, 0.0f, ro}, {-n3, -n3, n3}, {0.0f, 0.0f}});
        m.append_indices({nv + 0, nv + 2, nv + 1, nv + 1, nv + 2, nv + 4, nv + 4, nv + 3, nv + 1, nv + 5, nv + 6, nv + 7});

        // positive z, fourth quadrant
        nv = m.m_vertices.size();
        m.create_vertex({{rii, -rii, rii}, {n2, -n2, 0.0f}, {0.0f, 0.0f}});
        m.create_vertex({{0.0f, -ri, ri}, {n2, -n2, 0.0f}, {0.0f, 0.0f}});
        m.create_vertex({{ri, 0.0f, ri}, {n2, -n2, 0.0f}, {0.0f, 0.0f}});
        m.create_vertex({{0.0f, -ri, roo}, {n2, -n2, 0.0f}, {0.0f, 0.0f}});
        m.create_vertex({{ri, 0.0f, roo}, {n2, -n2, 0.0f}, {0.0f, 0.0f}});
        m.create_vertex({{0.0f, -ri, roo}, {n3, -n3, n3}, {0.0f, 0.0f}});
        m.create_vertex({{ri, 0.0f, roo}, {n3, -n3, n3}, {0.0f, 0.0f}});
        m.create_vertex({{0.0f, 0.0f, ro}, {n3, -n3, n3}, {0.0f, 0.0f}});
        m.append_indices({nv + 0, nv + 2, nv + 1, nv + 1, nv + 2, nv + 4, nv + 4, nv + 3, nv + 1, nv + 5, nv + 6, nv + 7});

        // ...

        mesh.insert(m);

        assert(mesh.m_indices.size() % 3 == 0);
        check_normals(mesh);

        return;
    }

    //--------------------------------------------------------------------------
    void add_arm_face(mesh_type& m, float ro, float ri, float b, const vector_type& pos0, const vector_type& u0, const vector_type& u01, const vector_type& u20, const vector_type& center, const vector_type& n0, const vector_type& n12)
    {
        auto nv = m.m_vertices.size();

        // Internal rectangle vertices
        vector_type varm = (ro - 2.0f * ri) * u0;
        vector_type v0 = pos0 + b * u01;
        vector_type v1 = pos0 + b * -u20;
        vector_type v2 = v0 - varm;
        vector_type v3 = v1 - varm;

        // Arm vertices
        m.create_vertex({pos0, n0, {0.0f, 0.0f}});
        m.create_vertex({v0, n0, {0.0f, 0.0f}});
        m.create_vertex({v1, n0, {0.0f, 0.0f}});
        m.create_vertex({v0, n12, {0.0f, 0.0f}});
        m.create_vertex({v1, n12, {0.0f, 0.0f}});
        m.create_vertex({v2, n12, {0.0f, 0.0f}});
        m.create_vertex({v3, n12, {0.0f, 0.0f}});
        m.create_vertex({center, n12, {0.0f, 0.0f}});

        m.append_indices({nv + 0, nv + 1, nv + 2, nv + 3, nv + 5, nv + 6, nv + 6, nv + 4, nv + 3, nv + 6, nv + 5, nv + 7});

        return;
    }

    //--------------------------------------------------------------------------
    void create_cross3_planar(mesh_type& mesh, float a, float b)
    {
        // a is octahedron side length
        // b is end pyramid diagonal: 0 < b < a / 2

        float ro = a * sqrt(2.0f) / 2.0f;
        float ri = b * sqrt(2.0f) / 2.0f;

        // Body geometry used for reference vertices
        mesh_type mbody;
        create_octahedron(mbody, ro, false);

        mesh_type m;

        // Add three cross arm sides for each face
        assert((mbody.m_indices.size() % 3) == 0);
        for (unsigned int i = 0; i < mbody.m_indices.size(); i += 3)
        {
            // Original octahedron vertex positions
            vector_type pos0 = mbody.m_vertices[mbody.m_indices[i + 0]].m_position;
            vector_type pos1 = mbody.m_vertices[mbody.m_indices[i + 1]].m_position;
            vector_type pos2 = mbody.m_vertices[mbody.m_indices[i + 2]].m_position;

            // Normals for arm end faces
            vector_type n0 = mbody.m_vertices[mbody.m_indices[i + 0]].m_normal;
            vector_type n1 = mbody.m_vertices[mbody.m_indices[i + 1]].m_normal;
            vector_type n2 = mbody.m_vertices[mbody.m_indices[i + 2]].m_normal;

            // Normals for arm internal faces
            vector_type n12 = normalize(pos1 + pos2);
            vector_type n20 = normalize(pos2 + pos0);
            vector_type n01 = normalize(pos0 + pos1);

            // Unit vectors for use in internal face calculations
            vector_type u0 = normalize(pos0);
            vector_type u1 = normalize(pos1);
            vector_type u2 = normalize(pos2);

            vector_type u12 = normalize(pos2 - pos1);
            vector_type u01 = normalize(pos1 - pos0);
            vector_type u20 = normalize(pos0 - pos2);

            // Common center vertex position
            vector_type center = ri * sqrt(3.0f) / 2.0f * normalize(pos0 + pos1 + pos2);

            // Arm faces
            add_arm_face(m, ro, ri, b, pos0, u0, u01, u20, center, n0, n12);
            add_arm_face(m, ro, ri, b, pos1, u1, u12, u01, center, n1, n20);
            add_arm_face(m, ro, ri, b, pos2, u2, u20, u12, center, n2, n01);
        }

        mesh.insert(m);

        assert(mesh.m_indices.size() % 3 == 0);
        check_normals(mesh);

        return;
    }
*/

    //--------------------------------------------------------------------------
    template<typename M>
    void check(M& mesh)
    {
        mesh;
//        check_surface_vertex_consistency_exact(mesh);
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

    name = "stellated_octahedron_vertex";
    cout << name << endl;
    mesh.clear();
    create_stellated_octahedron(mesh, 20.0f);
    model::write_obj(mesh, to_filename(name));

    name = "stellated_octahedron_edge";
    cout << name << endl;
    mesh.clear();
    create_stellated_octahedron(mesh, 20.0f, false);
    model::write_obj(mesh, to_filename(name));

    name = "stellated_octahedron_rhombicdodecahedron";
    cout << name << endl;
    mesh.clear();
    create_stellated_octahedron(mesh, 20.0f, 6.0f);
    model::write_obj(mesh, to_filename(name));

    name = "octahedron_cube";
    cout << name << endl;
    mesh.clear();
    create_octahedron_cube(mesh, 20.0f, 0.5f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "spiketahedron";
    cout << name << endl;
    mesh.clear();
    create_spiketahedron(mesh, 10.0f);
    model::write_obj(mesh, to_filename(name));

    name = "spoctahedron1";
    cout << name << endl;
    mesh.clear();
    create_spoctahedron(mesh, 20.0f, 1.618f, 1);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "spoctahedron2";
    cout << name << endl;
    mesh.clear();
    create_spoctahedron(mesh, 20.0f, 1.618f, 2);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    // tetrehadral forms, separate program? ...

    name = "snowflake0";
    cout << name << endl;
    mesh.clear();
    create_snowflake(mesh, 20.0f * sqrt(3.0f), 1.0f, 0);
    model::write_obj(mesh, to_filename(name));

    name = "snowflake1";
    cout << name << endl;
    mesh.clear();
    create_snowflake(mesh, 20.0f * sqrt(3.0f), 1.0f, 1);
    model::write_obj(mesh, to_filename(name));

    name = "snowflake2";
    cout << name << endl;
    mesh.clear();
    create_snowflake(mesh, 20.0f * sqrt(3.0f), 1.0f, 2);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
check(mesh);

    name = "spectrahedron0";
    cout << name << endl;
    mesh.clear();
    create_spectrahedron(mesh, 30.0f, 1.618f, 0);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "spectrahedron1";
    cout << name << endl;
    mesh.clear();
    create_spectrahedron(mesh, 30.0f, 1.618f, 1);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "spectrahedron2";
    cout << name << endl;
    mesh.clear();
    create_spectrahedron(mesh, 30.0f, 1.618f, 2);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "hollow_tetrahedron";
    cout << name << endl;
    mesh.clear();
    create_hollow_tetrahedron(mesh, 20.0f, 2.0f / 3.0f);
    model::write_obj(mesh, to_filename(name));
/*
    name = "cross3";
    cout << name << endl;
    mesh.clear();
    create_cross3_planar(mesh, 40.0f, 12.0f);
    model::write_obj(mesh, to_filename(name));
*/
/*
    const size_t n = 8;
    value_type r = 10.0f;
    std::array<point_type, n> points;
    for (size_t i = 0; i < 8; ++i)
    {
        value_type t = value_type(i) / n;
        value_type phi = t * math::PiTwo<value_type>;
        value_type x = r * cos(phi);
        value_type y = r * sin(phi);
        value_type z = x + y;
        points[i] = {x + 2.0f, y + 3.0f, z + 4.0f};
    }
    using vertices_type = geometry::Polygon<vector_traits>::vertices_type;
    using vertex_type = vertices_type::value_type;
    geometry::Polygon<vector_traits> polygon(points);
    point_type c1 = polygon.centroid();
    point_type c2 = centroid(polygon);
    cout << c1 << endl;
    cout << c2 << endl;

    auto cf0 = to_polygon(mesh.faces().front()).centroid();
    auto cf1 = centroid(mesh.faces().front());
    cout << cf0 << endl;
    cout << cf1 << endl;

    auto vcf0 = to_polygon(mesh.faces().front()).vertex_centroid();
    auto vcf1 = vertex_centroid(mesh.faces().front());
    cout << vcf0 << endl;
    cout << vcf1 << endl;
*/
    validator.report_results();
    return 0;
}
