//------------------------------------------------------------------------------
// spikodecahedron
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/connection.hpp"
#include "quetzal/brep/triangulation.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include "quetzal/model/helix_pyramid.hpp"
#include "quetzal/model/mesh_attributes.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
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

    const size_t s_nLevels = 4;

    // Control revs on first (low, 1) and higher levels
    const bool s_bTwistLow = true;
    const bool s_bTwistHigh = true;

    static const filesystem::path dir_out = "out/";
    static const filesystem::path dir_ref = "reference/";
    static const string app_name = "spikodecahedron";

    //--------------------------------------------------------------------------
    filesystem::path to_filename(const string& name)
    {
        return dir_out / (name + ".obj");
    }

    //--------------------------------------------------------------------------
    // move to common module ...
    std::string strip_root_name(const std::string& name)
    {
        return name | std::views::drop_while([](char c) { return !isdigit(c); }) | std::ranges::to<std::string>();
    }

} // namespace

namespace
{

    //--------------------------------------------------------------------------
    id_type create_spikodecahedron(mesh_type& mesh, const string& name, value_type r0, value_type r1, value_type z1, value_type revsSpikeMin, value_type revsSpikeMax, size_t nzSpikeMax)
    {
        assert(nzSpikeMax > 0);

        id_type submesh_id = model::create_dodecahedron(mesh, name, r0);

        value_type z0 = mesh.submesh(submesh_id).bounds().lower().z();

        auto face_ids = mesh.submesh(submesh_id).face_ids();
        for (auto face_id : face_ids)
        {
            mesh_type::vector_type upSpike = mesh_type::vector_type::z_direction();
            mesh_type::vector_type upFace = mesh.face(face_id).attributes().normal();

            // Angle to rotate face so that it is perpendicular to z-axis
            value_type theta = math::angle_between_unit(upSpike, upFace);

            size_t nLevel = 0; // Controls detail and rotation
            value_type r = 0; // Length of spike

            if (math::float_eq(theta, math::Pi<value_type>)) // Bottom face, no spike
            {
                continue;
            }
            else if (math::float_ne0(theta))
            {
                nLevel = (math::PiHalf<value_type> - theta) > 0 ? 2 : 1;

                value_type theta0 = math::PiHalf<value_type> - asin(upFace.z());
                value_type theta2 = asin(z1 * sin(theta0) / r1);
                value_type theta1 = math::Pi<value_type> - (theta0 + theta2);
                r = r1 * sin(theta1) / sin(theta0);
            }
            else // Top face
            {
                nLevel = s_nLevels - 1;
                r = r1 + z1;
            }

            if ((!s_bTwistLow && nLevel == 1) || (!s_bTwistHigh && nLevel > 1) || (revsSpikeMin == 0.0f && revsSpikeMax == 0.0f) || nzSpikeMax  == 1) // Replace face with pyramidal projection
            {
                triangulate_central_vertex(mesh, face_id, upFace * r, false);
                continue;
            }

            size_t nAzimuth = mesh.face(face_id).halfedges().size();
            size_t nz = nzSpikeMax * nLevel / (s_nLevels - 1);
            value_type dz = r - r0;
            value_type revs = revsSpikeMin + (revsSpikeMax - revsSpikeMin) * nLevel / (s_nLevels - 1); // lerp? ...

            string spike_name = "spike" + strip_root_name(mesh.face(face_id).surface().name());
            string spike_name_base = brep::extended_surface_name(spike_name, "end0");

            // Body face position and orientation
            const auto& face = mesh.face(face_id);
            mesh_type::point_type position = centroid(face);
//            mesh_type::vector_type upFace = face.attributes().normal();
            mesh_type::vector_type vFace = face.halfedge().vertex_attributes().position() - position;
            mesh_type::vector_type rightFace = normalize(vFace);
            value_type rBase = vFace.norm();

            // Spike and orientation
            id_type spike_id = model::create_helix_pyramid(mesh, spike_name, nAzimuth, nz, rBase, dz, revs);
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
        clip_base(mesh.submesh(submesh_id), {0.0f, 0.0f, z0}, "clip_base")

        return submesh_id;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    FileValidator validator(dir_out, dir_ref);

    string name;
    mesh_type mesh;

    name = "spiked_dodecahedron";
    cout << name << endl;
    mesh.clear();
    create_spikodecahedron(mesh, "spikodecahedon", 29.0f, 58.0f, 31.1688f, 0.0f, 0.0f, 1);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "twisted_dodecahedron";
    cout << name << endl;
    mesh.clear();
    create_spikodecahedron(mesh, "spikodecahedon", 29.0f, 58.0f, 31.1688f, 0.0f, 0.5f, 100);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "twisted_dodecahedron_ball";
    cout << name << endl;
    mesh.clear();
    create_spikodecahedron(mesh, "spikodecahedon", 30.0f, 36.0f, 0.0f, 0.25f, 0.25f, 100);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "twisted_dodecahedron_cactus";
    cout << name << endl;
    mesh.clear();
    create_spikodecahedron(mesh, "spikodecahedon", 45.0f, 54.0f, 0.0f, 0.25f, 0.25f, 100);
    model::transform(mesh, geometry::scaling(1.0f, 1.0f, 0.666f));
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "twisted_dodecahedron_faceted";
    cout << name << endl;
    mesh.clear();
    create_spikodecahedron(mesh, "spikodecahedon", 29.0f, 58.0f, 31.1688f, 0.0f, 0.25f, 3);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    validator.report_results();
    return 0;
}
