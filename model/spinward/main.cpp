//------------------------------------------------------------------------------
// spinward
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
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
#include <filesystem>
#include <format>
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
    using matrix_type = geometry::transformation_matrix_type<vector_traits>;
    using vector_type = mesh_type::vector_type;

    static const filesystem::path dir_out = "out/";
    static const filesystem::path dir_ref = "reference/";
    static const string app_name = "spinward";

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
    void create_spinward(mesh_type& mesh, const string& name, size_t nSubdivisions, value_type r0, value_type r1, value_type scaleInner = value_type(1))
    {
        id_type submesh_id = model::create_geodesic_sphere(mesh, name, r0, nSubdivisions, false, false);

        auto face_ids = mesh.submesh(submesh_id).face_ids();
        for (auto face_id : face_ids)
        {
            auto face = mesh.face(face_id);
            auto center = centroid(face);

            vector_type upFace = face.attributes().normal();
            vector_type rightFace = normalize(face.halfedge().vertex_attributes().position() - center);
            vector_type upSpike = vector_type::z_direction();
            vector_type rightSpike = vector_type::x_direction();

            // Create spike base polygon from face
            auto polygon = to_polygon(face);
            auto matrix = geometry::translation(-to_vector(center));
            matrix *= geometry::alignment(upFace, rightFace, upSpike, rightSpike);
            matrix *= geometry::scaling(scaleInner, scaleInner, 0.0f); // simple projection ...
            polygon.transform(matrix);

            // Spike
            string spike_name = "spike";
            string spike_name_base = brep::extended_surface_name(spike_name, "end0");
            id_type spike_id = model::create_cylinder_edges(mesh, spike_name, polygon, 1, 0.0f, r1 - r0, 1.0f, 0.0f, 0.0f, 0.0f);

            // Rotate spike back to align with face normal and translate spike to face position
            matrix = geometry::alignment(upSpike, rightSpike, upFace, rightFace); // alignment2 ...
            matrix *= geometry::translation(to_vector(center));
            model::transform(mesh.submesh(spike_id), matrix);

            // Connect spike to body
            std::string body_name = face.surface().name();
            if (scaleInner == 1.0f)
            {
                brep::weld(mesh, face.surface().extended_name(), spike_name_base);
            }
            else
            {
                brep::attach(mesh, face.surface().extended_name(), spike_name_base);
            }

            mesh.rename_submesh_surfaces(spike_id, [body_name](const std::string& surface_name) {
                return std::format("{}_{}", body_name, strip_root_name(surface_name));
            });
            mesh.rename_submesh(spike_name, name);
        }

        mesh.pack();
        return;
    }

    //--------------------------------------------------------------------------
    void create_spinward_flush(mesh_type& mesh, const string& name, size_t nSubdivisions, value_type r0, value_type r1, value_type scaleInner)
    {
        id_type submesh_id = model::create_geodesic_sphere(mesh, name, r0, nSubdivisions, false, false);

        auto face_ids = mesh.submesh(submesh_id).face_ids();
        for (auto face_id : face_ids)
        {
            auto face = mesh.face(face_id);
            auto center = centroid(face);
            auto radial_vector = face.halfedge().vertex_attributes().position() - center;

            vector_type upFace = face.attributes().normal();
            vector_type rightFace = normalize(radial_vector);
            vector_type upSpike = vector_type::z_direction();
            vector_type rightSpike = vector_type::x_direction();

            auto polygon = to_polygon(face);
            auto matrix = geometry::translation(-to_vector(center));
            matrix *= geometry::alignment(upFace, rightFace, upSpike, rightSpike);
            matrix *= geometry::scaling(scaleInner, scaleInner, 0.0f);
            polygon.transform(matrix);

            // Spike
            string spike_name = "spike";
            string spike_name_base = brep::extended_surface_name(spike_name, "end0");
            id_type spike_id = model::create_cylinder_edges(mesh, spike_name, polygon, 1, 0.0f, r1 - r0, 1.0f, 0.0f, 0.0f, 0.0f);

            value_type h = to_vector(center).norm();
            value_type r = radial_vector.norm() / 2.0f;
//                value_type r = (
//                    (face.halfedge().vertex_attributes().position() - center).norm() +
//                    (face.halfedge().next().vertex_attributes().position() - center).norm() +
//                    (face.halfedge().prev().vertex_attributes().position() - center).norm()
//                    ) / 6.0f;
            value_type q = scaleInner * r;
            value_type d = r * (r - q) / h;
if (nSubdivisions == 2)
{
    d = 0.35f;
}
//cout << "c " << c << " h " << h << " r " << r << " q " << q << " d " << d << endl;
            matrix = geometry::translation(0.0f, 0.0f, d);

            // Rotate spike back to align with face normal and translate spike to face position
            matrix *= geometry::alignment(upSpike, rightSpike, upFace, rightFace); // alignment2 ...
            matrix *= geometry::translation(to_vector(center));
            model::transform(mesh.submesh(spike_id), matrix);

            // Connect
            std::string body_name = face.surface().name();
            brep::attach(mesh, face.surface().extended_name(), spike_name_base, false);
            brep::connect_non_planar(mesh, face.id(), false);

            mesh.rename_submesh_surfaces(spike_id, [body_name](const std::string& surface_name) {
                return std::format("{}_{}", body_name, strip_root_name(surface_name));
            });
            mesh.rename_submesh(spike_name, name);
        }

        // now merge adjacent flush edges into the same surface ...
        // create list above, for each, find its partner in the list, remove both, assign surface of one to the other ...
        // or just generate differently such that there is only one surface produced ...

        mesh.pack();
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
    matrix_type matrix;

    name = "spinward_1";
    cout << name << endl;
    mesh.clear();
    create_spinward(mesh, name, 1, 60.0f, 30.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "spinward_2";
    cout << name << endl;
    mesh.clear();
    create_spinward(mesh, name, 2, 60.0f, 30.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "spinward_1_inset";
    cout << name << endl;
    mesh.clear();
    create_spinward(mesh, name, 1, 60.0f, 30.0f, 0.8f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
/*
mesh_type m1;
model::read_obj(m1, dirOut / (name + ".obj"));
mesh_type m2;
model::read_obj(m2, dirReference / (name + ".obj"));
cout << (dirReference / (name + ".obj")) << endl;
size_t n = m1.vertex_count();
assert(n == m2.vertex_count());
for (size_t i = 0; i < n; ++i)
{
    auto texcoord1 = m1.vertex(i).attributes().texcoord();
    auto texcoord2 = m2.vertex(i).attributes().texcoord();
    assert(float_eq(m1.vertex(i).attributes().position(), m2.vertex(i).attributes().position()));
    assert(float_eq(m1.vertex(i).attributes().normal(), m2.vertex(i).attributes().normal()));
    assert(float_eq(texcoord1, texcoord2));
}
*/
    name = "spinward_2_inset";
    cout << name << endl;
    mesh.clear();
    create_spinward(mesh, name, 2, 60.0f, 30.0f, 0.8f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "spinward_1_flush";
    cout << name << endl;
    mesh.clear();
    create_spinward_flush(mesh, name, 1, 60.0f, 30.0f, 0.8f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "spinward_2_flush";
    cout << name << endl;
    mesh.clear();
    create_spinward_flush(mesh, name, 2, 60.0f, 30.0f, 0.8f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    validator.report_results();
    return 0;
}
