//------------------------------------------------------------------------------
// spikodesic
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/clip.hpp"
#include "quetzal/brep/connection.hpp"
#include "quetzal/brep/validation.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/geometry/intersect.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/math/Matrix.hpp"
#include "quetzal/model/helix_pyramid.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include "quetzal/model/transforms.hpp"
#include <algorithm>
#include <array>
#include <string>
#include <iostream>
#include <cassert>

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;
    using matrix_type = math::Matrix<value_type>;

    static const mesh_type::vector_type down = -mesh_type::vector_type::z_direction();

    static const filesystem::path dir_out = "out/";
    static const filesystem::path dir_ref = "reference/";
    static const string app_name = "spikodesic";

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

    //--------------------------------------------------------------------------
    bool adjacent_faces(const mesh_type& mesh, id_type idFaceA, id_type idFaceB)
    {
        for (const auto& halfedge : mesh.face(idFaceA).halfedges())
        {
            if (halfedge.partner().face_id() == idFaceB)
            {
                return true;
            }
        }

        return false;
    }

    //--------------------------------------------------------------------------
    id_type select_base_face(const mesh_type& mesh, id_type submesh_id)
    {
        for (const auto& face : mesh.submesh(submesh_id).faces())
        {
            assert(face.halfedges().size() == 3);

            auto position0 = face.halfedge().vertex_attributes().position();
            auto position1 = face.halfedge().next().vertex_attributes().position();
            auto position2 = face.halfedge().prev().vertex_attributes().position();

            auto normal = normalize(cross(position1 - position0, position2 - position0));
            assert(math::float_eq(normal, face.attributes().normal()));

            mesh_type::value_type length0 = (position1 - position0).norm();
            mesh_type::value_type length1 = (position2 - position1).norm();
            mesh_type::value_type length2 = (position0 - position2).norm();

            // check that equilateral works for all nSubdivisions ...
            bool bEquilateral = math::float_eq0(length1 - length0) && math::float_eq0(length2 - length1);
            if (bEquilateral)
            {
                return face.id();
            }
        }

        return nullid;
    }

    //--------------------------------------------------------------------------
    id_type orient_vertical(mesh_type& mesh, id_type submesh_id)
    {
        id_type face_id0 = select_base_face(mesh, submesh_id);
        auto face = mesh.face(face_id0);

        matrix_type matrix = geometry::alignment(face.attributes().normal(), down);
        model::transform(mesh.submesh(submesh_id), matrix);

        value_type z0 = centroid(face).z();

        for (auto& halfedge : face.halfedges())
        {
            auto position = halfedge.vertex_attributes().position();
            position.set_z(z0);

// this won't work until select_base_face is completed, but it seems that normals might not be calculated right ...
//            model::unify_vertex_position(halfedge.vertex(), position);
        }

        return face_id0;
    }

    //--------------------------------------------------------------------------
    id_type create_spikodesic(mesh_type& mesh, const string& name, value_type r0, size_t nSubdivisions, std::function<value_type(value_type)> radius, size_t nzSpikeMax, value_type revsSpikeMax)
    {
        assert(nzSpikeMax > 0);

        id_type submesh_id = model::create_geodesic_sphere(mesh, name, r0, nSubdivisions, false, false);

        id_type face_id0 = orient_vertical(mesh, submesh_id);
        value_type z0 = mesh.submesh(submesh_id).bounds().lower().z();

        // Base plane
        geometry::Plane<mesh_type::vector_traits> plane({0.0f, 0.0f, z0}, down);

        auto face_ids = mesh.submesh(submesh_id).face_ids();
        for (const auto& face_id : face_ids)
        {
            if (face_id == face_id0) // Bottom face
            {
                continue;
            }

            const auto& face = mesh.face(face_id);
            mesh_type::vector_type upFace = face.attributes().normal();

            if (adjacent_faces(mesh, face_id0, face_id)) // First level, replace face with pyramidal projection to be flush with base
            {
                geometry::Line<mesh_type::vector_traits> line(centroid(face), upFace);
                auto inter = intersection(line, plane);
                assert(inter.locus() == geometry::Locus::Point);
                auto p = inter.point();
                assert(math::float_eq(p.z(), z0));
                p.set_z(z0);

                // maybe just set nz to 1 here and use clip at end? ...
                // maybe do that for any apex below z0? ...

                triangulate_central_vertex(mesh, face_id, p, false);
                continue;
            }

            mesh_type::point_type position = centroid(face);
            mesh_type::vector_type rightFace = normalize(face.halfedge().vertex_attributes().position() - position);
            mesh_type::vector_type upSpike = mesh_type::vector_type::z_direction();
            mesh_type::vector_type rightSpike = mesh_type::vector_type::x_direction();

            geometry::Polygon<mesh_type::vector_traits> polygon = to_polygon(face);
            polygon.set_sections_edges();
            matrix_type matrix = geometry::translation(-position.vector()) * geometry::alignment(upFace, rightFace, upSpike, rightSpike);
            polygon.transform(matrix);

            for (auto& v : polygon.vertices())
            {
                assert(math::float_eq0(v.z()));
                v.set_z(0.0f);
            }

            value_type theta = math::angle_between_unit(down, face.attributes().normal());
            assert(theta >= 0.0f && theta <= math::Pi<value_type>);
            value_type t = theta / math::Pi<value_type>;

            // Spike parameters
            size_t nz = static_cast<size_t>(clamp(floor(nzSpikeMax * t) + 1.0f, 1.0f, value_type(nzSpikeMax)));
            value_type r = radius(t);
            value_type dz = r - r0;
            assert(dz >= 0.0f);
            value_type revs = revsSpikeMax * t;

            string spike_name = std::format("spike{}", strip_root_name(mesh.face(face_id).surface().name())); // replace base name but retain surface counter, virtual submesh for lower levels
            string spike_name_base = brep::extended_surface_name(spike_name, "end0");

            // Spike and orientation
            id_type spike_id = model::create_helix_pyramid(mesh, spike_name, polygon, nz, dz, revs);

            // Rotate spike to align with face normal and translate spike to face position
//            matrix = alignment2(upSpike, rightSpike, upFace, rightFace) * translation(position);
            matrix = geometry::alignment(upSpike, rightSpike, upFace, rightFace) * geometry::translation(position.vector());
            model::transform(mesh.submesh(spike_id), matrix);

            // this may actually be more efficient, but a single clip at the end should work too ...
            if ((position + upFace * dz).z() < z0)
            {
                brep::clip(mesh.submesh(spike_id), {plane, down});
            }

            // Connect spike to body
            brep::weld(mesh, mesh.face(face_id).surface().extended_name(), spike_name_base);
            mesh.rename_submesh_surfaces(spike_id, [spike_name](const std::string& surface_name) {
                return std::format("{}_{}", spike_name, strip_root_name(surface_name));
            });
            mesh.rename_submesh(spike_name, name);
        }

// should work here once instead of separately above ...
// but seems like a problem likely because there is a face in the clip plane ...
// test this separately ...
// 
//        brep::clip(mesh.submesh(submesh_id), plane);

        mesh.pack();
        return submesh_id;
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

    value_type r0 = 30.0f;
    size_t nSubdivisions = 2;

    auto radius = [&](value_type t) -> value_type
    {
        return r0 * (1.0f + 1.5f * t); // spiral ...
    };

    name = "spikodesic";
    cout << name << endl;
    mesh.clear();
    create_spikodesic(mesh, name, r0, nSubdivisions, radius, 1, 0.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "swirled_spikodesic";
    cout << name << endl;
    mesh.clear();
//    create_spikodesic(mesh, name, r0, nSubdivisions, radius, 100, 0.0f);
create_spikodesic(mesh, name, r0, nSubdivisions, radius, 10, 0.0f);
    model::swirl(mesh, math::Pi<value_type>);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "twisted_spikodesic";
    cout << name << endl;
    mesh.clear();
//    create_spikodesic(mesh, name, r0, nSubdivisions, radius, 100, 0.5f);
create_spikodesic(mesh, name, r0, nSubdivisions, radius, 10, 0.5f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    // Use previous mesh
    name = "swirled_twisted_spikodesic";
    cout << name << endl;
	model::swirl(mesh, math::Pi<value_type>);
    model::write_obj(mesh, to_filename(name));

    validator.report_results();
    return 0;
}
