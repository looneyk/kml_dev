//------------------------------------------------------------------------------
// mosely_snowflake
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/clip.hpp"
#include "quetzal/brep/connection.hpp"
#include "quetzal/brep/triangulation.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/geometry/HalfSpace.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include <iostream>
#include <string>
#include <cassert>

#include "quetzal/brep/Visualizer.hpp"
#include "quetzal/brep/visualization.hpp"

using namespace std;
using namespace quetzal;

namespace g = quetzal::geometry;

namespace
{

    using value_type = float;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;
    using point_type = mesh_type::point_type;
    using vector_type = point_type::vector_type;

    static const filesystem::path dir_out = "out/";
    static const filesystem::path dir_ref = "reference/";
    static const string app_name = "mosely_snowflake";

    //--------------------------------------------------------------------------
    filesystem::path to_filename(const string& name)
    {
        return dir_out / (name + ".obj");
    }

    //--------------------------------------------------------------------------
    void iterate(mesh_type& mesh, id_type submesh_id, value_type size, bool heavy)
    {
        string name = mesh.submesh(submesh_id).name();
        mesh_type m;
        id_type cell_id = mesh.move(submesh_id, m, name);

        struct CellPlacement
        {
            string name;
            typename mesh_type::vector_type offset;
        };

        std::vector<CellPlacement> placements =
        {
            {"cell_0", {0.0f, -size, -size}},
            {"cell_1", {-size, 0.0f, -size}},
            {"cell_2", {0.0f, 0.0f, -size}},
            {"cell_3", {size, 0.0f, -size}},
            {"cell_4", {0.0f, size, -size}},
            {"cell_5", {-size, -size, 0.0f}},
            {"cell_6", {0.0f, -size, 0.0f}},
            {"cell_7", {size, -size, 0.0f}},
            {"cell_8", {-size, 0.0f, 0.0f}},
            {"cell_9", {0.0f, 0.0f, 0.0f}},
            {"cell_10", {size, 0.0f, 0.0f}},
            {"cell_11", {-size, size, 0.0f}},
            {"cell_12", {0.0f, size, 0.0f}},
            {"cell_13", {size, size, 0.0f}},
            {"cell_14", {0.0f, -size, size}},
            {"cell_15", {-size, 0.0f, size}},
            {"cell_16", {0.0f, 0.0f, size}},
            {"cell_17", {size, 0.0f, size}},
            {"cell_18", {0.0f, size, size}},
        };

        for (const auto& placement : placements)
        {
            if (heavy || placement.name != "cell_9")
            {
                mesh.insert(m, cell_id, placement.name, [&placement](typename mesh_type::vertex_type vertex) -> typename mesh_type::vertex_type { vertex.attributes().position() *= g::translation(placement.offset); return vertex; });
            }
        }

        weld(mesh, "cell_0/front", "cell_2/back");
        weld(mesh, "cell_1/right", "cell_2/left");
        weld(mesh, "cell_2/right", "cell_3/left");
        weld(mesh, "cell_2/front", "cell_4/back");
        weld(mesh, "cell_0/top", "cell_6/bottom");
        weld(mesh, "cell_1/top", "cell_8/bottom");
        weld(mesh, "cell_3/top", "cell_10/bottom");
        weld(mesh, "cell_4/top", "cell_12/bottom");

        weld(mesh, "cell_5/right", "cell_6/left");
        weld(mesh, "cell_6/right", "cell_7/left");
        weld(mesh, "cell_5/front", "cell_8/back");
        weld(mesh, "cell_7/front", "cell_10/back");
        weld(mesh, "cell_8/front", "cell_11/back");
        weld(mesh, "cell_10/front", "cell_13/back");
        weld(mesh, "cell_11/right", "cell_12/left");
        weld(mesh, "cell_12/right", "cell_13/left");

        weld(mesh, "cell_6/top", "cell_14/bottom");
        weld(mesh, "cell_8/top", "cell_15/bottom");
        weld(mesh, "cell_10/top", "cell_17/bottom");
        weld(mesh, "cell_12/top", "cell_18/bottom");
        weld(mesh, "cell_14/front", "cell_16/back");
        weld(mesh, "cell_15/right", "cell_16/left");
        weld(mesh, "cell_16/right", "cell_17/left");
        weld(mesh, "cell_16/front", "cell_18/back");

        if (heavy)
        {
            weld(mesh, "cell_2/top", "cell_9/bottom");
            weld(mesh, "cell_6/front", "cell_9/back");
            weld(mesh, "cell_8/right", "cell_9/left");
            weld(mesh, "cell_9/right", "cell_10/left");
            weld(mesh, "cell_9/front", "cell_12/back");
            weld(mesh, "cell_9/top", "cell_16/bottom");
        }

        // Reassign all interior surfaces to internal surface of primary submesh

        std::string surface_name_internal = name + "/internal"; // single internal surface is problematic ...

        mesh.move_surface("cell_0/left", surface_name_internal);
        mesh.move_surface("cell_0/right", surface_name_internal);
        mesh.move_surface("cell_1/back", surface_name_internal);
        mesh.move_surface("cell_1/front", surface_name_internal);
        mesh.move_surface("cell_3/back", surface_name_internal);
        mesh.move_surface("cell_3/front", surface_name_internal);
        mesh.move_surface("cell_4/left", surface_name_internal);
        mesh.move_surface("cell_4/right", surface_name_internal);

        mesh.move_surface("cell_5/bottom", surface_name_internal);
        mesh.move_surface("cell_5/top", surface_name_internal);
        mesh.move_surface("cell_7/bottom", surface_name_internal);
        mesh.move_surface("cell_7/top", surface_name_internal);
        mesh.move_surface("cell_11/bottom", surface_name_internal);
        mesh.move_surface("cell_11/top", surface_name_internal);
        mesh.move_surface("cell_13/bottom", surface_name_internal);
        mesh.move_surface("cell_13/top", surface_name_internal);

        mesh.move_surface("cell_14/left", surface_name_internal);
        mesh.move_surface("cell_14/right", surface_name_internal);
        mesh.move_surface("cell_15/back", surface_name_internal);
        mesh.move_surface("cell_15/front", surface_name_internal);
        mesh.move_surface("cell_17/back", surface_name_internal);
        mesh.move_surface("cell_17/front", surface_name_internal);
        mesh.move_surface("cell_18/left", surface_name_internal);
        mesh.move_surface("cell_18/right", surface_name_internal);

        if (!heavy)
        {
            mesh.move_surface("cell_2/top", surface_name_internal);
            mesh.move_surface("cell_6/front", surface_name_internal);
            mesh.move_surface("cell_8/right", surface_name_internal);
            mesh.move_surface("cell_10/left", surface_name_internal);
            mesh.move_surface("cell_12/back", surface_name_internal);
            mesh.move_surface("cell_16/bottom", surface_name_internal);
        }

        // Ensure that all surfaces are part of the main submesh

        for (auto& surface : mesh.surfaces())
        {
            if (surface.deleted())
            {
                continue;
            }

            if (surface.submesh().name() != name)
            {
                mesh.move_surface(surface.id(), brep::extended_surface_name(name, surface.name()));
            }
        }

        mesh.pack();
        return;
    }

    //--------------------------------------------------------------------------
    id_type create_mosely_snowflake(mesh_type& mesh, const string& name, size_t nLevel, value_type scale, bool heavy = true)
    {
        value_type size = 1.0f;
        id_type submesh_id = model::create_box(mesh, name, size, size, size);

        for (size_t i = 0; i < nLevel; ++i)
        {
            cout << "Level " << i << " size " << size << endl;
            iterate(mesh, submesh_id, size, heavy);
            size *= 3.0f;
        }

        model::transform_positions(mesh, g::scaling(scale / size));
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
    id_type submesh_id = nullid;

    vector_type diagonal = math::normalize<vector_traits>({1.0f, 1.0f, 1.0f});
    value_type angle = angle_between_unit(diagonal, vector_type::z_direction());
    vector_type axis = normalize(cross(diagonal, vector_type::z_direction()));
    auto matrix = g::rotation_axis_unit(axis, angle);

    point_type point = {};
    g::HalfSpace<vector_traits> halfspace_upper{{point, -vector_type::z_direction()}, -vector_type::z_direction()};

    name = "mosely_snowflake_heavy_0";
    cout << name << endl;
    mesh.clear();
    submesh_id = create_mosely_snowflake(mesh, name, 0, 100.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_heavy_0_diagonal";
    cout << name << endl;
    model::transform(mesh, matrix);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_heavy_0_diagonal_half";
    cout << name << endl;
    brep::clip(mesh, halfspace_upper, "slice");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_0";
    cout << name << endl;
    mesh.clear();
    submesh_id = create_mosely_snowflake(mesh, name, 0, 100.0f, false);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_0_diagonal";
    cout << name << endl;
    model::transform(mesh, matrix);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_0_diagonal_half";
    cout << name << endl;
    brep::clip(mesh, halfspace_upper, "slice");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_heavy_1";
    cout << name << endl;
    mesh.clear();
    submesh_id = create_mosely_snowflake(mesh, name, 1, 100.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_heavy_1_diagonal";
    cout << name << endl;
    model::transform(mesh, matrix);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_heavy_1_diagonal_half";
    cout << name << endl;
    brep::clip(mesh, halfspace_upper, "slice");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_1";
    cout << name << endl;
    mesh.clear();
    submesh_id = create_mosely_snowflake(mesh, name, 1, 100.0f, false);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_1_diagonal";
    cout << name << endl;
    model::transform(mesh, matrix);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_1_diagonal_half";
    cout << name << endl;
    brep::clip(mesh, halfspace_upper, "slice");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_heavy_2";
    cout << name << endl;
    mesh.clear();
    submesh_id = create_mosely_snowflake(mesh, name, 2, 100.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_heavy_2_diagonal";
    cout << name << endl;
    model::transform(mesh, matrix);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_heavy_2_diagonal_half";
    cout << name << endl;
mesh.clear();
submesh_id = create_mosely_snowflake(mesh, name, 2, 100.0f);
model::transform(mesh, matrix);
    brep::clip(mesh, halfspace_upper, "slice");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_2";
    cout << name << endl;
    mesh.clear();
    submesh_id = create_mosely_snowflake(mesh, name, 2, 100.0f, false);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_2_diagonal";
    cout << name << endl;
    model::transform(mesh, matrix);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_2_diagonal_half";
    cout << name << endl;
mesh.clear();
submesh_id = create_mosely_snowflake(mesh, name, 2, 100.0f, false);
model::transform(mesh, matrix);
    brep::clip(mesh, halfspace_upper, "slice");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_heavy_3";
    cout << name << endl;
    mesh.clear();
    submesh_id = create_mosely_snowflake(mesh, name, 3, 100.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_heavy_3_diagonal";
    cout << name << endl;
    model::transform(mesh, matrix);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_heavy_3_diagonal_half";
    cout << name << endl;
    brep::clip(mesh, halfspace_upper, "slice");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_3";
    cout << name << endl;
    mesh.clear();
    submesh_id = create_mosely_snowflake(mesh, name, 3, 100.0f, false);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_3_diagonal";
    cout << name << endl;
    model::transform(mesh, matrix);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_3_diagonal_half";
    cout << name << endl;
    brep::clip(mesh, halfspace_upper, "slice");
const auto& face = mesh.faces().back();
const auto& normal = face.attributes().normal();
std::cout << "normal " << normal << std::endl;
auto right = normalize(cross(normal, vector_type::y_direction()));
brep::draw_face<mesh_type>("clip_face.svg", face, geometry::alignment(normal, right, vector_type::z_direction(), vector_type::x_direction()));

right = normalize(vertex_position(face.halfedge()) - vertex_centroid(face));
geometry::Orientation<vector_traits> orientation(normal, right);
auto projection = geometry::Projection(orientation);
brep::Visualizer<mesh_type> visualizer(projection, 0.05f, 0.1f);
visualizer.draw_face(face);
for (const auto& halfedge : face.halfedges())
{
    visualizer.draw_face(halfedge.partner().face());
}
visualizer.write("clip_face_projection.svg");

auto forward = vector_type(0.0f, -0.5f, 1.0f);
right = vector_type::x_direction();
auto up = normalize(cross(right, forward));
orientation = {up, right};
projection.init(orientation);
visualizer.clear();
visualizer.set_projection(projection);
visualizer.set_scale(0.05f, 0.05f);
visualizer.draw_face(face);
visualizer.write("clip_face_projection2.svg");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
/*
    name = "mosely_snowflake_heavy_4";
    cout << name << endl;
    mesh.clear();
    submesh_id = create_mosely_snowflake(mesh, name, 4, 100.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_heavy_4_diagonal";
    cout << name << endl;
    transform(mesh, matrix);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_heavy_4_diagonal_half";
    cout << name << endl;
    brep::clip(mesh, halfspace_upper, "slice");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_4";
    cout << name << endl;
    mesh.clear();
    submesh_id = create_mosely_snowflake(mesh, name, 4, 100.0f, false);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_4_diagonal";
    cout << name << endl;
    transform(mesh, matrix);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_4_diagonal_half";
    cout << name << endl;
    brep::clip(mesh, halfspace_upper, "slice");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
*/

    validator.report_results();
    return 0;
}
