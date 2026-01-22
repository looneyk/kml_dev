//------------------------------------------------------------------------------
// spikosahedron
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/connection.hpp"
#include "quetzal/brep/mesh_util.hpp"
#include "quetzal/brep/validation.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/math/Matrix.hpp"
#include "quetzal/model/helix_pyramid.hpp"
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

    const size_t s_nLevels = 6;

    static const filesystem::path dir_out = "out/";
    static const filesystem::path dir_ref = "reference/";
    static const string app_name = "spikosahedron";

    //--------------------------------------------------------------------------
    filesystem::path to_filename(const string& name)
    {
        return dir_out / (name + ".obj");
    }

    //--------------------------------------------------------------------------
    void validate_normals(mesh_type& mesh)
    {
        for (const auto& vertex : mesh.vertices())
        {
            auto normal = surface_normal(vertex);
            if (!float_eq(vertex.attributes().normal(), normal))
            {
                std::cout << std::format("mismatch id {} vertex {} surface {} face {} face calc {}", vertex.id(), vertex.attributes().normal(), normal, vertex.halfedge().face_attributes().normal(), geometry::normal(vertex.halfedge().face())) << std::endl;
            }
        }
    }

    //--------------------------------------------------------------------------
    // move to common module ...
    std::string strip_root_name(const std::string& name)
    {
        return name | std::views::drop_while([](char c) { return !isdigit(c); }) | std::ranges::to<std::string>();
    }

    //--------------------------------------------------------------------------
    id_type create_spikosahedron(mesh_type& mesh, const string& name, value_type r0, value_type r1, value_type z1, size_t nzSpikeMax, value_type revsSpikeMax, value_type tuskification = 0.0f, value_type undulation = 0.0f)
    {
        assert(nzSpikeMax > 0);

        id_type submesh_id = model::create_icosahedron(mesh, name, r0, false);

        value_type z0 = 0;

        auto face_ids = mesh.submesh(submesh_id).face_ids();
        for (const auto& face_id : face_ids)
        {
            mesh_type::vector_type upSpike = {0.0f, 0.0f, 1.0f};
            mesh_type::vector_type upFace = mesh.face(face_id).attributes().normal();

            // Angle to rotate face so that it is perpendicular to z-axis
            value_type theta = math::angle_between_unit(upSpike, upFace);

            size_t nLevel = 0; // Controls detail and rotation
            value_type r = 0; // Length of spike

            if (math::float_eq(theta, math::Pi<value_type>)) // Bottom face
            {
                nLevel = 0;
                r = r0;
            }
            else if (math::float_ne(theta, value_type(0)))
            {
                nLevel = static_cast<size_t>((math::Pi<value_type> - theta) / math::Pi<value_type> * 4 + 0.05f) + 1;

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

            std::string spike_name = std::format("spike{}", strip_root_name(mesh.face(face_id).surface().name())); // replace base name but retain surface counter, virtual submesh for lower levels

            if (nLevel == 0) // Bottom face, just record base z coordinate
            {
                z0 = mesh.face(face_id).halfedge().vertex_attributes().position().z();
                mesh.rename_surface(mesh.face(face_id).surface_id(), spike_name);
                continue;
            }

            if (nLevel == 1 || nzSpikeMax == 1) // Replace face with pyramidal projection
            {
                auto apex = upFace * r;
                if (nLevel == 1) // First level must be at exactly base height, this is a specific requirement for printing with this spikosahedron algorithm
                {
                    apex.set_z(z0);
                }
                mesh.rename_surface(mesh.face(face_id).surface_id(), spike_name);
                triangulate_central_vertex(mesh, face_id, apex, false);
                continue;
            }

            size_t nAzimuth = mesh.face(face_id).halfedges().size();
            size_t nz = nzSpikeMax * (nLevel - 1) / (s_nLevels - 2);
            value_type dzSpike = r - r0;
            value_type revs = revsSpikeMax * (nLevel - 1) / (s_nLevels - 2);

            // Body face position and orientation
            mesh_type::point_type position = centroid(mesh.face(face_id));
            mesh_type::vector_type vFace = mesh.face(face_id).halfedge().vertex_attributes().position() - position;
            mesh_type::vector_type rightFace = normalize(vFace);
//            auto rightFace = to_polygon(mesh.face(face_id)).right(); // really need a single function to return the magnitude and direction for a vector ...
            value_type rBase = vFace.norm();

            // Spike and orientation
            id_type spike_id = model::create_helix_pyramid(mesh, spike_name, nAzimuth, nz, rBase, dzSpike, revs);
            mesh_type::vector_type rightSpike = normalize(to_vector(mesh.surface(brep::extended_surface_name(spike_name, "end0")).faces().front().halfedge().vertex_attributes().position()));
            assert(math::float_eq0(rightSpike.z()));

            // Apply spike transformations

            if (undulation != 0)
            {
                model::undulate(mesh.submesh(spike_id), undulation, 3.0f, 0.0f, dzSpike);
            }

            if (tuskification != 0 && theta != 0)
            {
                mesh_type::vector_type axisRotationBase;
                mesh_type::vector_type rightFaceOriented = rightFace;

                axisRotationBase = normalize(cross(upSpike, upFace)); // axis to rotate face, perpendicular to z-axis
                rightFaceOriented *= geometry::rotation_axis_unit(axisRotationBase, -theta); // Rotate face parallel to xy-plane
                value_type phi = atan2(rightFaceOriented.y(), rightFaceOriented.x()) - atan2(rightSpike.y(), rightSpike.x());

                model::tuskify(mesh.submesh(spike_id), axisRotationBase * geometry::rotation_z(-phi), tuskification, 0.0f, dzSpike);
            }

            // Rotate spike to align with face normal and translate spike to face position
//            matrix_type matrix = alignment2(upSpike, rightSpike, upFace, rightFace) * translation(position);
            matrix_type matrix = geometry::alignment(upSpike, rightSpike, upFace, rightFace) * geometry::translation(to_vector(position));
            model::transform(mesh.submesh(spike_id), matrix);

            brep::weld(mesh, mesh.face(face_id).surface().extended_name(), spike_name + "/end0");
            mesh.rename_submesh_surfaces(spike_id, [spike_name](const std::string& surface_name) {
                return std::format("{}_{}", spike_name, strip_root_name(surface_name));
            });
            mesh.rename_submesh(spike_name, name);
        }

        mesh.pack();

        // Ensure that nothing projects below the bottom face
        clip_base(mesh.submesh(submesh_id), {0.0f, 0.0f, z0}, "clip_base");
//validate_normals(mesh);
        return submesh_id;
    }

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

    mesh_type mesh;
    string name;

    // Make sure that z1 is large enough that lower spikes don't extend below base

    name = "spikosahedron";
    cout << name << endl;
    mesh.clear();
    create_spikosahedron(mesh, name, 29.0f, 58.0f, 31.1688f, 1, 0.0f); // Not swirled, not twisted, single section spike
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
check(mesh);

    // this is so we have extra material to carve a sharp point at the tip, the tip was not printing cleanly
    name = "spikosahedron_tip";
    cout << name << endl;
    mesh.clear();
    create_spikosahedron(mesh, name, 29.0f, 58.0f, 31.1688f, 1, 0.0f); // Not swirled, not twisted, single section spike
    float z1 = mesh.bounds().upper().z();
    model::create_cylinder(mesh, "tip_tip", 3, 1, 2.0f, 0.0f, z1 - 1.0f, z1, 0.5f, 0.5f);
    model::create_cylinder(mesh, "tip_body", 3, 1, 2.0f, 2.0f, z1 - 10.0f, z1 - 1.0f, 0.5f, 0.5f);
    weld(mesh, "tip_body/end1", "tip_tip/end0");

    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
check(mesh);

    // dimensions for stained glass model
    name = "spikosahedron_glass_1";
    cout << name << endl;
    mesh.clear();
    create_spikosahedron(mesh, "spikosahedron", 72.471f, 144.942f, 77.8858f, 1, 0.0f); // Not swirled, not twisted, single section spike
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
//dump(mesh);
check(mesh);

    // dimensions for stained glass model without base pane
    name = "spikosahedron_glass_1_open";
    cout << name << endl;
    mesh.clear();
    create_spikosahedron(mesh, "spikosahedron", 72.471f, 144.942f, 77.8858f, 1, 0.0f); // Not swirled, not twisted, single section spike
    mesh.delete_face(0);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
/*
for (const auto& halfedge : mesh.halfedge_store())
{
    if (halfedge.deleted())
    {
        cout << "deleted: " << halfedge.id() << endl;
    }
    if (halfedge.border())
    {
        cout << "border: " << halfedge.id() << endl;
    }
}
*/
    // For glass, may be better off not twisted, but still with divisions, but maybe more interesting divisions ...
check(mesh);

    name = "spikosahedron_glass";
    cout << name << endl;
    mesh.clear();
    create_spikosahedron(mesh, "spikosahedron", 29.0f, 58.0f, 31.1688f, 4, 0.0f); // Chunky spike
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
check(mesh);

    name = "twisted_spikosahedron_glass";
    cout << name << endl;
    mesh.clear();
    create_spikosahedron(mesh, "spikosahedron", 29.0f, 58.0f, 31.1688f, 4, 0.25f); // Twisted, chunky spike
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
check(mesh);

    name = "tuskified_spikosahedron_glass";
    cout << name << endl;
    mesh.clear();
    create_spikosahedron(mesh, "spikosahedron", 29.0f, 58.0f, 31.1688f, 4, 0.0f, math::PiHalf<value_type>); // Tuskified, chunky spike
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
check(mesh);

    name = "tuskified_twisted_spikosahedron_glass";
    cout << name << endl;
    mesh.clear();
    create_spikosahedron(mesh, "spikosahedron", 29.0f, 58.0f, 31.1688f, 4, 0.25f, math::PiHalf<value_type>); //  Tuskified, twisted, chunky spike
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
check(mesh);

    // Smooth combinations

    name = "swirled_spikosahedron";
    cout << name << endl;
    mesh.clear();
    create_spikosahedron(mesh, "spikosahedron", 29.0f, 58.0f, 31.1688f, 100, 0.0f); // Swirled, smooth spike
    model::swirl(mesh, math::Pi<value_type>);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
check(mesh);

    name = "twisted_spikosahedron";
    cout << name << endl;
    mesh.clear();
    create_spikosahedron(mesh, "spikosahedron", 29.0f, 58.0f, 31.1688f, 100, 0.5f); // Twisted, smooth spike
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
check(mesh);

    // Use previous mesh
    name = "swirled_twisted_spikosahedron";
    cout << name << endl;
    model::swirl(mesh, math::Pi<value_type>);
    model::write_obj(mesh, to_filename(name));
check(mesh);

    name = "screwed_spikosahedron";
    cout << name << endl;
    mesh.clear();
    create_spikosahedron(mesh, "spikosahedron", 29.0f, 58.0f, 31.1688f, 100, 2.5f); // Twisted, smooth spike
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
check(mesh);

    // Tuskification

    name = "tuskified_spikosahedron";
    cout << name << endl;
    mesh.clear();
    create_spikosahedron(mesh, "spikosahedron", 29.0f, 58.0f, 31.1688f, 200, 0.0f, math::PiHalf<value_type>); // Tuskified, smooth spike
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
check(mesh);

    // Use previous mesh
    name = "tuskified_swirled_spikosahedron";
    cout << name << endl;
    model::swirl(mesh, math::Pi<value_type>);
    model::write_obj(mesh, to_filename(name));
check(mesh);

    name = "tuskified_twisted_spikosahedron";
    cout << name << endl;
    mesh.clear();
    create_spikosahedron(mesh, "spikosahedron", 29.0f, 58.0f, 31.1688f, 200, 0.5f, math::PiHalf<value_type>); // Tuskified, twisted, smooth spike
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
check(mesh);

    name = "tuskified_twisted_spikosahedron_model";
    cout << name << endl;
    mesh.clear();
    create_spikosahedron(mesh, "spikosahedron", 29.0f, 58.0f, 31.1688f, 20, 0.5f, math::PiHalf<value_type>); // Tuskified, twisted, smooth spike
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
check(mesh);

    // Use previous mesh
    name = "tuskified_swirled_twisted_spikosahedron";
    cout << name << endl;
    model::swirl(mesh, math::Pi<value_type>);
    model::write_obj(mesh, to_filename(name));
check(mesh);
/*
    // Undulation

    name = "undulating_spikosahedron";
    cout << name << endl;
    mesh.clear();
    create_spikosahedron(mesh, "spikosahedron", 29.0f, 58.0f, 31.1688f, 200, 0.0f, 0.0f, 3.0f * math::PiTwo<value_type>); // Undulating, smooth spike
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    // Use previous mesh
    name = "undulating_swirled_spikosahedron";
    cout << name << endl;
    model::swirl(mesh, math::Pi<value_type>);
    model::write_obj(mesh, to_filename(name));

    name = "undulating_twisted_spikosahedron";
    cout << name << endl;
    mesh.clear();
    create_spikosahedron(mesh, "spikosahedron", 29.0f, 58.0f, 31.1688f, 200, 0.5f, 0.0f, 3.0f * math::PiTwo<value_type>); // Undulating, twisted, smooth spike
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    // Use previous mesh
    name = "undulating_swirled_twisted_spikosahedron";
    cout << name << endl;
    model::swirl(mesh, math::Pi<value_type>);
    model::write_obj(mesh, to_filename(name));
*/
    validator.report_results();
    return 0;
}
