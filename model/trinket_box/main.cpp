//------------------------------------------------------------------------------
// trinket_box
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/connection.hpp"
#include "quetzal/brep/geometry.hpp"
#include "quetzal/brep/triangulation.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include "quetzal/model/helix_pyramid.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include <iostream>
#include <string>
#include <cassert>

//#include "quetzal/brep/validation.hpp"

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
//    using value_type = double;
// rotation_axis_unit_xm ...
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;
    using matrix_type = math::Matrix<value_type>;

    static const filesystem::path dir_out = "out/";
    static const filesystem::path dir_ref = "reference/";
    static const string app_name = "trinket_box";

    //--------------------------------------------------------------------------
    filesystem::path to_filename(const string& name)
    {
        return dir_out / (name + ".obj");
    }

    //--------------------------------------------------------------------------
    value_type dodecahedron_insphere_radius(value_type r)
    {
        using T = value_type;
        return r * T(2) * sqrt((T(25) + T(11) * sqrt(T(5))) / T(10)) / (sqrt(T(3)) * (T(1) + sqrt(T(5))));
    }

    //--------------------------------------------------------------------------
    // move to common module ...
    std::string strip_root_name(const std::string& name)
    {
        return name | std::views::drop_while([](char c) { return !isdigit(c); }) | std::ranges::to<std::string>();
    }

    //--------------------------------------------------------------------------
    id_type create_spikodecahedron(mesh_type& mesh, const string& name, value_type r0, value_type r1, value_type revsSpike, size_t nzSpike)
    {
        assert(nzSpike > 0);

        id_type submesh_id = model::create_dodecahedron(mesh, name, r0);

        value_type dzSpike = r1 - dodecahedron_insphere_radius(r0); // Length of spike
        value_type z0 = mesh.submesh(submesh_id).bounds().lower().z();

        auto face_ids = mesh.submesh(submesh_id).face_ids()
            | std::views::filter([&mesh](id_type face_id) { const auto& surface_name = mesh.face(face_id).surface().name(); return surface_name != "body0" && surface_name != "body11"; })
            | std::ranges::to<std::vector>();
        for (id_type face_id : face_ids)
        {
            size_t nAzimuth = mesh.face(face_id).halfedges().size();
            string spike_name = "spike" + strip_root_name(mesh.face(face_id).surface().name());
            string spike_name_base = brep::extended_surface_name(spike_name, "end0");

            // Body face position and orientation
            const auto& face = mesh.face(face_id);
            mesh_type::point_type position = centroid(face);
            mesh_type::vector_type upFace = face.attributes().normal();
            mesh_type::vector_type vFace = face.halfedge().vertex_attributes().position() - position;
            mesh_type::vector_type rightFace = normalize(vFace);
            value_type rBase = vFace.norm();

            // Spike and orientation
            id_type spike_id = model::create_helix_pyramid(mesh, spike_name, nAzimuth, nzSpike, rBase, dzSpike, revsSpike);
            mesh_type::vector_type upSpike = mesh_type::vector_type::z_direction();
            mesh_type::vector_type rightSpike = normalize(to_vector(mesh.surface(spike_name_base).faces().front().halfedge().vertex_attributes().position()));
            assert(math::float_eq0(rightSpike.z()));

            // Rotate spike to align with face normal and translate spike to face position
            matrix_type matrix = geometry::alignment(upSpike, rightSpike, upFace, rightFace) * geometry::translation(to_vector(position));
            model::transform(mesh.submesh(spike_id), matrix);

            // Connect spike to body
            brep::weld(mesh, mesh.face(face_id).surface().extended_name(), spike_name_base);
            mesh.rename_submesh_surfaces(spike_id, [spike_name](const std::string& surface_name)
            {
                return std::format("{}_{}", spike_name, strip_root_name(surface_name));
            });
            mesh.rename_submesh(spike_name, name);
        }

        mesh.pack();

        // Ensure that nothing projects below the bottom face
        // redo this, try for_each_vertex? ...
        // for each vertex position change, normals need to be recalculated ...
        for (auto& vertex : mesh.vertices())
        {
            if (vertex.attributes().position().z() < z0)
            {
                vertex.attributes().position().set_z(z0);
            }
        }

        return submesh_id;
    }

    //--------------------------------------------------------------------------
    void unify_vertices(mesh_type& mesh)
    {
        size_t nVertexInequalities = 0;

        mesh.reset();
        for (auto& vertex : mesh.vertices())
        {
            if (!vertex.marked())
            {
                bool bUnequal = false;
                auto position = vertex.attributes() .position();
                for (auto& halfedge : vertex.halfedges())
                {
                    if (!epsilon_eq(position, halfedge.vertex_attributes().position()))
                    {
                        cout << "vertices " << vertex.id() << " " << halfedge.vertex_id() << " failed precision check." << endl;
                    }

                    if (position != halfedge.vertex_attributes().position())
                    {
                        bUnequal = true;
                    }

                    halfedge.vertex_attributes().set_position(position);
                    halfedge.vertex().set_marked();
                }

                if (bUnequal)
                {
                    ++nVertexInequalities;
                }
            }
        }

        cout << nVertexInequalities << " vertex inequalities." << endl;

        return;
    }

    //--------------------------------------------------------------------------
    void check_vertices(mesh_type& mesh)
    {
        size_t nVertexInequalities = 0;
        size_t nTotalInequalities = 0;
        mesh.reset();
        for (auto& vertex : mesh.vertices())
        {
            if (!vertex.marked())
            {
                bool bUnequal = false;
                auto position = vertex.attributes() .position();
                for (auto& halfedge : vertex.halfedges())
                {
                    if (position != halfedge.vertex_attributes().position())
                    {
                        ++nTotalInequalities;
                        bUnequal = true;
                    }

                    halfedge.vertex().set_marked();
                }

                if (bUnequal)
                {
                    ++nVertexInequalities;
                }
            }
        }

        cout << nVertexInequalities << " vertex inequalities." << endl;
        cout << nTotalInequalities << " total inequalities." << endl;

        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    value_type r0 = 1.0f;
    value_type r1 = 1.1f;
    size_t nzSpike = 300; // Resolution
    value_type dzSpike = r1 - dodecahedron_insphere_radius(r0); // Length of spike
    value_type revsSpike = 0.25f;
//nzSpike = 1;
//revsSpike = 0.0f;

    FileValidator validator(dir_out, dir_ref, app_name);

    string name;
    mesh_type mesh;

    name = "trinket_box_open";
    cout << name << endl;
    mesh.clear();

    // Outer surface
    id_type outer_id = create_spikodecahedron(mesh, "outer", 1.0f, 1.1f, revsSpike, nzSpike);
    auto polygonUpperOrig = to_polygon(mesh.surface("outer/body11").faces().front());
    polygonUpperOrig.sections() = std::vector<id_type>();
    model::transform(mesh.submesh(outer_id), geometry::scaling(50.0f, 50.0f, 25.0f));
    auto polygonUpper = to_polygon(mesh.surface("outer/body11").faces().front());
    polygonUpper.sections() = std::vector<id_type>(); // this should have better semantics, could also be = {}, .clear(), something that indicates what missing sections means ...

    // Inner surface
    id_type inner_id = model::create_dodecahedron(mesh, "inner", 1.0f);
    model::transform(mesh.submesh(inner_id), geometry::scaling(45.0f, 45.0f, 20.0f));
    auto polygonLower = to_polygon(mesh.surface("inner/body11").faces().front());
    polygonLower.sections() = std::vector<id_type>();
    invert(mesh.submesh(inner_id));

    // Neck
    size_t nh = mesh.halfedge_count();
    id_type neck_id = model::create_cylinder_edges(mesh, "neck", polygonLower, polygonUpper, 3);

    size_t n = polygonLower.vertex_count();
    size_t nBodyVertices = 3 * n * 4;
    value_type z = (mesh.vertex(nh + 0).attributes().position().z() + mesh.vertex(nh + nBodyVertices - n).attributes().position().z()) * value_type(0.5);
    for (size_t i = 0; i < n; ++i)
    {
        size_t j = nh + 4 * n + i;
        size_t k = nh + nBodyVertices - n + i;
        size_t l = nh + nBodyVertices - 4 * n + i;

        typename mesh_type::point_type pointLower = mesh.vertex(nh + i).attributes().position();
        pointLower.set_z(z);

        for (auto& halfedge : mesh.vertex(j).halfedges())
        {
            halfedge.vertex_attributes().set_position(pointLower);
        }

        typename mesh_type::point_type pointUpper = mesh.vertex(k).attributes().position();
        pointUpper.set_z(z);

        for (auto& halfedge : mesh.vertex(l).halfedges())
        {
            halfedge.vertex_attributes().set_position(pointUpper);
        }
    }

    invert(mesh.submesh(neck_id));

    // Connect outer and inner surfaces with neck
    weld(mesh, "outer/body11", "neck/end1");
    weld(mesh, "inner/body11", "neck/end0");

    std::string submesh_name = mesh.submesh(outer_id).name();
    mesh.rename_submesh_surfaces(outer_id, [submesh_name](const std::string& surface_name)
    {
        return brep::extended_surface_name(submesh_name, surface_name);
    });
    mesh.rename_submesh(submesh_name, name);

    submesh_name = mesh.submesh(inner_id).name();
    mesh.rename_submesh_surfaces(inner_id, [submesh_name](const std::string& surface_name)
    {
        return brep::extended_surface_name(submesh_name, surface_name);
    });
    mesh.rename_submesh(submesh_name, name);

    submesh_name = mesh.submesh(neck_id).name();
    mesh.rename_submesh_surfaces(neck_id, [submesh_name](const std::string& surface_name)
    {
        return brep::extended_surface_name(submesh_name, surface_name);
    });
    mesh.rename_submesh(submesh_name, name);

    triangulate(mesh);
//unify_vertices(mesh);
//check_vertices(mesh);
    model::write_obj(mesh, to_filename(name));
//validate(mesh);
//dump(mesh);

    name = "trinket_box_cover";
    cout << name << endl;
    mesh.clear();

    id_type external_id = model::create_helix_pyramid(mesh, "external", polygonUpperOrig, nzSpike, dzSpike, revsSpike);
    model::transform(mesh.submesh(external_id), geometry::scaling(50.0f, 50.0f, 25.0f));

    value_type dz = polygonUpper.vertex(0).z() - z;
//    model::create_cylinder(m, "internal", polygonUpper, 1, -dz, value_type(0.0), {{"azimuth_smooth", "false"}});
for (auto& v : polygonUpper.vertices())
{
    v.set_z(value_type(0));
}
polygonLower = polygonUpper;
for (auto& p : polygonLower.vertices())
{
    p *= value_type(0.995);
    p.set_z(-dz);
}

    model::create_cylinder_edges(mesh, "internal", polygonLower, polygonUpper, 1);

    weld(mesh, "external/end0", "internal/end1");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
//validate(mesh);
//check_surface_vertex_consistency_exact(mesh);

    validator.report_results();
    return 0;
}
