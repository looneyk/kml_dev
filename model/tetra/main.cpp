//------------------------------------------------------------------------------
// tetra
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/clip.hpp"
#include "quetzal/brep/connection.hpp"
//#include "quetzal/brep/validation.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/geometry/OrientedPoint.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/math/Matrix.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include "quetzal/model/transforms.hpp"
#include <array>
#include <filesystem>
#include <iostream>
#include <string>
#include <cassert>

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;
    using matrix_type = math::Matrix<value_type>;

    const filesystem::path dirOut = "out";
    const filesystem::path dirReference = "reference";

    //--------------------------------------------------------------------------
    filesystem::path to_filename(const string& name)
    {
        return dirOut / (name + ".obj");
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
            cout << "    " << surface.id() << " " << surface.name() << " " << surface.extended_name() << " " << surface.face_count() << " faces " << (surface.deleted() ? "D" : "") << endl;
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

    //-------------------------------------------------------------------------
    vector<id_type> vertex_selector(const mesh_type& mesh, std::function<bool(const mesh_type::vertex_type&)> selector = [](const mesh_type::vertex_type&) -> bool { return true; })
    {
        mesh.reset();

        vector<id_type> ids;
        for (const auto& vertex : mesh.vertices())
        {
            if (vertex.checked())
            {
                continue;
            }

            if (selector(vertex))
            {
                ids.push_back(vertex.id());
            }

            for (auto& halfedge : vertex.halfedges())
            {
                halfedge.vertex().set_checked();
            }
        }

        return ids;
    }

    //-------------------------------------------------------------------------
    vector<id_type> face_selector(const mesh_type& mesh, std::function<bool(const mesh_type::face_type&)> selector = [](const mesh_type::face_type&) -> bool { return true; })
    {
        mesh.reset();

        vector<id_type> ids;
        for (const auto& face : mesh.faces())
        {
            if (selector(face))
            {
                ids.push_back(face.id());
            }
        }

        return ids;
    }

    //--------------------------------------------------------------------------
    void create_hollow_tetrahedron_1(mesh_type& mesh, const string& name, value_type r_tetra, value_type d_cube, bool clip_inner = true)
    {
        using vector_type = mesh_type::vector_type;

        mesh_type m_outer;
        string outer_name = "outer";
        auto id_tetra = model::create_tetrahedron(m_outer, outer_name, r_tetra);
        mesh.insert(m_outer);

        // cube should be sized such that this clips without overlap, a cuboctahedron ...
        mesh_type m_inner;
        string inner_name = "inner";
        auto id_cube = model::create_cube(m_inner, inner_name, d_cube);

        // align cube on vertex, not just on expected orientation ...
        vector_type up_from = {1.0f, 1.0f, 1.0f};
        up_from.normalize();
        vector_type right_from = {1.0f, -0.5f, -0.5f};
        right_from.normalize();

        auto up_to = vector_type::z_direction();
        auto right_to = vector_type::x_direction();

        matrix_type matrix = geometry::alignment(up_from, right_from, up_to, right_to);
        model::transform(m_inner, matrix);

        // Clip once at each unique original vertex position
        vector<id_type> vertex_ids;
        for (const auto& vertex : m_inner.vertices())
        {
            if (vertex.checked())
            {
                continue;
            }

            vertex_ids.push_back(vertex.id());

            for (auto& halfedge : vertex.halfedges())
            {
                halfedge.vertex().set_checked();
            }
        }

// tetrahedron incenter length for position for clip ...
//        auto r_insphere = tetrahedron_insphere_circumsphere_ratio(r_tetra); // 1/3 ...
        auto r_insphere = r_tetra / 3.0f;

        vector<array<string, 2>> face_mapping; // tetrahedron surfaces to cube clip surfaces

        size_t n = 0;
        for (id_type vertex_id : vertex_ids)
        {
            auto normal = normalize(to_vector(m_inner.vertex(vertex_id).attributes().position()));
            auto position = r_insphere * normal;
            string clip_surface_name = format("clip_{}", n);
            string inner_surface_name = format("{}/{}", inner_name, clip_surface_name);

            // if clipping inner, then all vertices will be clipped here
            // clip_outer is always true
            if (clip_inner)
            {
                brep::clip(m_inner, {position, normal}, clip_surface_name);
            }

            for (const auto& face : m_outer.faces())
            {
                if (float_eq(normal, face.attributes().normal())) // codirectional, vector operation? same thing for unit vectors ...
                {
                    // if not clipping inner, then only outer vertices will be clipped here
                    if (!clip_inner)
                    {
                        brep::clip(m_inner, {position, normal}, clip_surface_name);
                    }

                    face_mapping.push_back({face.surface().extended_name(), inner_surface_name});
                }
            }

            ++n;
        }

        invert(m_inner); // m_inner.invert() ...
        mesh.insert(m_inner);

        for (const auto& e : face_mapping)
        {
            attach(mesh, e[0], e[1]);
        }
/*
// this should work ...
        for (const auto& e : face_mapping)
        {
            auto id_face_outer = e.first;
            auto normal = mesh.face(id_face_outer).attributes().normal();

            const auto& face_inner = mesh.surface(e.second).faces().front();
            auto id_hole_inner = face_inner.halfedge().partner_id();
            mesh.delete_face(face_inner.id());

            create_border_hole(mesh, id_hole_inner, normal, id_face_outer);
        }
*/
        mesh.rename_submesh(inner_name, name);
        mesh.rename_submesh(outer_name, name);

        return;
    }

    //--------------------------------------------------------------------------
    void create_hollow_tetrahedron_2(mesh_type& mesh, const string& name, value_type r_outer, value_type r_inner)
    {
        using vector_type = mesh_type::vector_type;

        mesh_type m_outer;
        string outer_name = "outer";
        auto id_outer = model::create_tetrahedron(m_outer, outer_name, r_outer);
        mesh.insert(m_outer);

        mesh_type m_inner;
        string inner_name = "inner";
        auto id_inner = model::create_tetrahedron(m_inner, inner_name, r_inner);

        // rename inner body surfaces ...
        // this should not have to be done explicitly, but handled when inserting, surface name conflict handler? ...
        // instead insert is resulting in unwanted merging of surfaces ...
        m_inner.rename_surfaces([&inner_name](const std::string& name) -> std::string { return format("{}_{}", inner_name, name); });

        // align dynamically ...
        auto axis = vector_type::y_direction();
        matrix_type matrix = geometry::rotation_axis(axis, math::Pi<value_type>);
        model::transform(m_inner, matrix);

        // Clip once at each unique original vertex position
        vector<id_type> vertex_ids = vertex_selector(m_inner);

        // tetrahedron incenter length for position for clip ...
//        auto r_insphere = tetrahedron_insphere_circumsphere_ratio(r_outer); // 1/3 ...
        auto r_insphere = r_outer / 3.0f;

        vector<array<string, 2>> face_mapping; // outer surfaces to inner clip surfaces

        size_t n = 0;
        for (id_type vertex_id : vertex_ids)
        {
            auto normal = normalize(to_vector(m_inner.vertex(vertex_id).attributes().position()));
            auto position = r_insphere * normal;
            string clip_surface_name = format("clip_{}", n);
            string inner_surface_name = format("{}/{}", inner_name, clip_surface_name);

            brep::clip(m_inner, {position, normal}, clip_surface_name);

            cout << clip_surface_name << "\t" << length(m_inner.surface(id_inner, clip_surface_name).faces().front().halfedge()) << endl;

            for (const auto& face : m_outer.faces())
            {
                auto a = face.surface().extended_name();
                auto normal_face1 = face.attributes().normal();
                if (float_eq(normal, face.attributes().normal())) // codirectional, vector operation? same thing for unit vectors ...
                {
                    face_mapping.push_back({face.surface().extended_name(), inner_surface_name});
                }
            }

            ++n;
        }

        invert(m_inner); // m_inner.invert() ...
        mesh.insert(m_inner);

        for (const auto& e : face_mapping)
        {
            attach(mesh, e[0], e[1]);
        }

        mesh.rename_submesh(inner_name, name);
        mesh.rename_submesh(outer_name, name);

        return;
    }

    //--------------------------------------------------------------------------
    void create_hollow_octahedron(mesh_type& mesh, const string& name, value_type r_outer, value_type r_inner)
    {
        using vector_type = mesh_type::vector_type;

        mesh_type m_outer;
        string outer_name = "outer";
        auto id_outer = model::create_octahedron(m_outer, outer_name, r_outer, false);
        mesh.insert(m_outer);

        mesh_type m_inner;
        string inner_name = "inner";
        auto id_inner = model::create_cube(m_inner, inner_name, r_inner);

        // align dynamically ...
        vector_type up_from = {1.0f, 1.0f, 1.0f};
        up_from.normalize();
        vector_type right_from = {0.5f, 0.5f, -1.0f};
        right_from.normalize();

        auto up_to = vector_type::z_direction();
        auto right_to = vector_type::x_direction();

        matrix_type matrix = geometry::alignment(up_from, right_from, up_to, right_to);
        model::transform(m_inner, matrix);

        // Clip once at each unique original vertex position
        vector<id_type> vertex_ids = vertex_selector(m_inner);

        // tetrahedron incenter length for position for clip ...
        //        auto r_insphere = octahedron_radius_insphere_given_circumsphere(r_outer); // 1/sqrt(3) ...
        auto r_insphere = r_outer / sqrt(3.0f);

        vector<array<string, 2>> face_mapping; // outer surfaces to inner clip surfaces

        size_t n = 0;
        for (id_type vertex_id : vertex_ids)
        {
            auto normal = normalize(to_vector(m_inner.vertex(vertex_id).attributes().position()));
            auto position = r_insphere * normal;
            string clip_surface_name = format("clip_{}", n);
            string inner_surface_name = format("{}/{}", inner_name, clip_surface_name);

            brep::clip(m_inner, {position, normal}, clip_surface_name);

            cout << clip_surface_name << "\t" << length(m_inner.surface(id_inner, clip_surface_name).faces().front().halfedge()) << endl;

            for (const auto& face : m_outer.faces())
            {
                auto a = face.surface().extended_name();
                auto normal_face1 = face.attributes().normal();
                if (float_eq(normal, face.attributes().normal())) // codirectional, vector operation? same thing for unit vectors ...
                {
                    face_mapping.push_back({face.surface().extended_name(), inner_surface_name});
                }
            }

            ++n;
        }

        invert(m_inner); // m_inner.invert() ...
        mesh.insert(m_inner);

        for (const auto& e : face_mapping)
        {
            attach(mesh, e[0], e[1]);
        }

        mesh.rename_submesh(inner_name, name);
        mesh.rename_submesh(outer_name, name);

        return;
    }

    //--------------------------------------------------------------------------
    void create_hollow_stellated_octahedron(mesh_type& mesh, const string& name, value_type scale)
    {
        value_type r_tetra = 0.25f * scale;

        //auto d_side = 1.0f / tetrahedron_circumsphere_side_ratio(r_tetra);
        auto d_side = r_tetra * 4.0f / sqrt(6.0f);

        //auto r_octa = octahedron_circumsphere_side_ratio(d_side);
        auto r_octa = d_side * sqrt(2.0f) / 2.0f;

        create_hollow_octahedron(mesh, "body", r_octa, 0.875f * r_octa);

        vector<id_type> face_ids = face_selector(mesh, [](const mesh_type::face_type& face) -> bool { return face.surface().name().starts_with("body"); });

        mesh_type m_spike;
        create_hollow_tetrahedron_2(m_spike, "spike", r_tetra, 0.75f * r_tetra);
        const auto& base = m_spike.surface("spike/body0").faces().front();
        auto base_center = centroid(base);
        auto base_position_0 = base.halfedge().vertex_attributes().position();
        geometry::OrientedPoint<mesh_type::vector_traits> op_spike{base_center, {-base.attributes().normal(), normalize(base_position_0 - base_center)}};

        size_t n = 0;
        for (id_type face_id : face_ids)
        {
            const auto& face = mesh.face(face_id);
            auto face_center = centroid(face);
            auto face_position_0 = face.halfedge().vertex_attributes().position();
            geometry::OrientedPoint<mesh_type::vector_traits> op_face{face_center, {face.attributes().normal(), normalize(face_position_0 - face_center)}};

            mesh_type m = m_spike;
            auto submesh_name = format("spike_{}", n);
            m.rename_submesh("spike", submesh_name);
            auto base_extended_name = format("{}/{}", submesh_name, "body0");

            // transformation matrix for oriented points ...
            auto matrix =
                geometry::translation(-base_center.vector()) *
                geometry::alignment(op_spike.orientation().direction(), op_spike.orientation().right(), op_face.orientation().direction(), op_face.orientation().right()) *
                geometry::translation(face_center.vector());

            model::transform(m, matrix);
            mesh.insert(m);

            weld(mesh, mesh.face(face_id).surface().extended_name(), base_extended_name);
            ++n;
        }

        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    FileValidator validator(dirOut, dirReference);

    string name;
    mesh_type mesh;
/*
    name = "hollow_tetrahedron_1a";
    cout << name << endl;
    mesh.clear();
    create_hollow_tetrahedron_1(mesh, name, 30.0f, 17.3f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "hollow_tetrahedron_1b";
    cout << name << endl;
    mesh.clear();
    create_hollow_tetrahedron_1(mesh, name, 30.0f, 17.3f, false);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    value_type scale = 120.0f;

    value_type r_tetra = 0.25f * scale;

    // now r_inner ...

    name = "hollow_tetrahedron_2";
    cout << name << endl;
    mesh.clear();
    create_hollow_tetrahedron_2(mesh, name, r_tetra, 0.75f * r_tetra);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    //auto d_side = 1.0f / tetrahedron_circumsphere_side_ratio(r_tetra);
    auto d_side = r_tetra * 4.0f / sqrt(6.0f);

    //auto r_octa = octahedron_circumsphere_side_ratio(d_side);
    auto r_octa = d_side * sqrt(2.0f) / 2.0f;
    cout << "r_octa\t" << r_octa << endl;

    name = "hollow_octahedron";
    cout << name << endl;
    mesh.clear();
    create_hollow_octahedron(mesh, name, r_octa, 0.875f * r_octa);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
*/
    value_type scale = 120.0f;
scale = 69.28204f;

    name = "hollow_stellated_octahedron";
    cout << name << endl;
    mesh.clear();
    create_hollow_stellated_octahedron(mesh, name, scale);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    validator.report_results();
    return 0;
}
