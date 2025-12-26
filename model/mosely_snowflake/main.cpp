//------------------------------------------------------------------------------
// mosely_snowflake
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/clip.hpp"
#include "quetzal/brep/connection.hpp"
#include "quetzal/brep/triangulation.hpp"
//#include "quetzal/brep/validation.hpp"
#include "quetzal/common/FileValidator.hpp"
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

    static const string dirOut = "out/";
    static const string dirReference = "reference/";

    //--------------------------------------------------------------------------
    string to_filename(const string& name)
    {
        return dirOut + name + ".obj";
    }

    //--------------------------------------------------------------------------
    void iterate(mesh_type& mesh, value_type size, bool bHeavy)
    {
        assert(mesh.submesh_count() == 1);
        string name = mesh.submesh(0).name();

        mesh_type mCell = move(mesh);
        mesh.clear();
        mesh_type m;

        m = mCell;
        m.rename_submesh(0, "cell_0");
        model::transform_positions(m, g::translation(0.0f, -size, -size));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_1");
        model::transform_positions(m, g::translation(-size, 0.0f, -size));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_2");
        model::transform_positions(m, g::translation(0.0f, 0.0f, -size));
        mesh.insert(m);                                 

        m = mCell;
        m.rename_submesh(0, "cell_3");
        model::transform_positions(m, g::translation(size, 0.0f, -size));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_4");
        model::transform_positions(m, g::translation(0.0f, size, -size));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_5");
        model::transform_positions(m, g::translation(-size, -size, 0.0f));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_6");
        model::transform_positions(m, g::translation(0.0f, -size, 0.0f));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_7");
        model::transform_positions(m, g::translation(size, -size, 0.0f));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_8");
        model::transform_positions(m, g::translation(-size, 0.0f, 0.0f));
        mesh.insert(m);

        if (bHeavy)
        {
            m = mCell;
            m.rename_submesh(0, "cell_9");
            model::transform_positions(m, g::translation(0.0f, 0.0f, 0.0f));
            mesh.insert(m);
        }

        m = mCell;
        m.rename_submesh(0, "cell_10");
        model::transform_positions(m, g::translation(size, 0.0f, 0.0f));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_11");
        model::transform_positions(m, g::translation(-size, size, 0.0f));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_12");
        model::transform_positions(m, g::translation(0.0f, size, 0.0f));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_13");
        model::transform_positions(m, g::translation(size, size, 0.0f));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_14");
        model::transform_positions(m, g::translation(0.0f, -size, size));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_15");
        model::transform_positions(m, g::translation(-size, 0.0f, size));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_16");
        model::transform_positions(m, g::translation(0.0f, 0.0f, size));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_17");
        model::transform_positions(m, g::translation(size, 0.0f, size));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_18");
        model::transform_positions(m, g::translation(0.0f, size, size));
        mesh.insert(m);

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

        if (bHeavy)
        {
            weld(mesh, "cell_2/top", "cell_9/bottom");
            weld(mesh, "cell_6/front", "cell_9/back");
            weld(mesh, "cell_8/right", "cell_9/left");
            weld(mesh, "cell_9/right", "cell_10/left");
            weld(mesh, "cell_9/front", "cell_12/back");
            weld(mesh, "cell_9/top", "cell_16/bottom");
        }

        // Reassign all interior surfaces to internal surface of primary submesh

        std::string nameSurfaceInternal = name + "/internal";

        mesh.move_surface("cell_0/left", nameSurfaceInternal);
        mesh.move_surface("cell_0/right", nameSurfaceInternal);
        mesh.move_surface("cell_1/back", nameSurfaceInternal);
        mesh.move_surface("cell_1/front", nameSurfaceInternal);
        mesh.move_surface("cell_3/back", nameSurfaceInternal);
        mesh.move_surface("cell_3/front", nameSurfaceInternal);
        mesh.move_surface("cell_4/left", nameSurfaceInternal);
        mesh.move_surface("cell_4/right", nameSurfaceInternal);

        mesh.move_surface("cell_5/bottom", nameSurfaceInternal);
        mesh.move_surface("cell_5/top", nameSurfaceInternal);
        mesh.move_surface("cell_7/bottom", nameSurfaceInternal);
        mesh.move_surface("cell_7/top", nameSurfaceInternal);
        mesh.move_surface("cell_11/bottom", nameSurfaceInternal);
        mesh.move_surface("cell_11/top", nameSurfaceInternal);
        mesh.move_surface("cell_13/bottom", nameSurfaceInternal);
        mesh.move_surface("cell_13/top", nameSurfaceInternal);

        mesh.move_surface("cell_14/left", nameSurfaceInternal);
        mesh.move_surface("cell_14/right", nameSurfaceInternal);
        mesh.move_surface("cell_15/back", nameSurfaceInternal);
        mesh.move_surface("cell_15/front", nameSurfaceInternal);
        mesh.move_surface("cell_17/back", nameSurfaceInternal);
        mesh.move_surface("cell_17/front", nameSurfaceInternal);
        mesh.move_surface("cell_18/left", nameSurfaceInternal);
        mesh.move_surface("cell_18/right", nameSurfaceInternal);

        if (!bHeavy)
        {
            mesh.move_surface("cell_2/top", nameSurfaceInternal);
            mesh.move_surface("cell_6/front", nameSurfaceInternal);
            mesh.move_surface("cell_8/right", nameSurfaceInternal);
            mesh.move_surface("cell_10/left", nameSurfaceInternal);
            mesh.move_surface("cell_12/back", nameSurfaceInternal);
            mesh.move_surface("cell_16/bottom", nameSurfaceInternal);
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
                mesh.move_surface(surface.id(), name + "/" + surface.name());
            }
        }

        mesh.pack();
        return;
    }

    //--------------------------------------------------------------------------
    void create_mosely_snowflake(mesh_type& mesh, const string& name, size_t nLevel, value_type scale, bool bHeavy = true)
    {
        mesh_type m;
        value_type size = 1.0f;
        model::create_box(m, name, size, size, size);

        for (size_t i = 0; i < nLevel; ++i)
        {
            cout << "Level " << i << " size " << size << endl;
            iterate(m, size, bHeavy);
            size *= 3.0f;
        }

        model::transform_positions(m, g::scaling(scale / size));
        mesh.insert(m);
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

    vector_type diagonal = math::normalize<vector_traits>({1.0f, 1.0f, 1.0f});
    value_type angle = angle_between_unit(diagonal, {0.0f, 0.0f, 1.0f});
    vector_type axis = normalize(cross(diagonal, {0.0f, 0.0f, 1.0f}));
    auto matrix = g::rotation_axis_unit(axis, angle);

    point_type point = {0.0f, 0.0f, 0.0f};
    vector_type direction = {0.0f, 0.0f, -1.0f};
    g::Plane plane(point, direction);

    name = "mosely_snowflake_heavy_0";
    cout << name << endl;
    mesh.clear();
    create_mosely_snowflake(mesh, name, 0, 100.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_heavy_0_diagonal";
    cout << name << endl;
    model::transform(mesh, matrix);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_heavy_0_diagonal_half";
    cout << name << endl;
    brep::clip(mesh, plane, "slice");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_0";
    cout << name << endl;
    mesh.clear();
    create_mosely_snowflake(mesh, name, 0, 100.0f, false);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_0_diagonal";
    cout << name << endl;
	model::transform(mesh, matrix);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_0_diagonal_half";
    cout << name << endl;
    brep::clip(mesh, plane, "slice");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_heavy_1";
    cout << name << endl;
    mesh.clear();
    create_mosely_snowflake(mesh, name, 1, 100.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_heavy_1_diagonal";
    cout << name << endl;
	model::transform(mesh, matrix);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_heavy_1_diagonal_half";
    cout << name << endl;
    brep::clip(mesh, plane, "slice");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_1";
    cout << name << endl;
    mesh.clear();
    create_mosely_snowflake(mesh, name, 1, 100.0f, false);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_1_diagonal";
    cout << name << endl;
	model::transform(mesh, matrix);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_1_diagonal_half";
    cout << name << endl;
    brep::clip(mesh, plane, "slice");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_heavy_2";
    cout << name << endl;
    mesh.clear();
    create_mosely_snowflake(mesh, name, 2, 100.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_heavy_2_diagonal";
    cout << name << endl;
	model::transform(mesh, matrix);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_heavy_2_diagonal_half";
    cout << name << endl;
mesh.clear();
create_mosely_snowflake(mesh, name, 2, 100.0f);
model::transform(mesh, matrix);
    brep::clip(mesh, plane, "slice");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_2";
    cout << name << endl;
    mesh.clear();
    create_mosely_snowflake(mesh, name, 2, 100.0f, false);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_2_diagonal";
    cout << name << endl;
	model::transform(mesh, matrix);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_2_diagonal_half";
    cout << name << endl;
mesh.clear();
create_mosely_snowflake(mesh, name, 2, 100.0f, false);
model::transform(mesh, matrix);
    brep::clip(mesh, plane, "slice");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_heavy_3";
    cout << name << endl;
    mesh.clear();
    create_mosely_snowflake(mesh, name, 3, 100.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_heavy_3_diagonal";
    cout << name << endl;
	model::transform(mesh, matrix);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_heavy_3_diagonal_half";
    cout << name << endl;
    brep::clip(mesh, plane, "slice");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_3";
    cout << name << endl;
    mesh.clear();
    create_mosely_snowflake(mesh, name, 3, 100.0f, false);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_3_diagonal";
    cout << name << endl;
	model::transform(mesh, matrix);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_3_diagonal_half";
    cout << name << endl;
    brep::clip(mesh, plane, "slice");
const auto& face = mesh.faces().back();
const auto& normal = face.attributes().normal();
std::cout << "normal " << normal << std::endl;
auto right = normalize(cross(normal, vector_type::y_direction()));
brep::draw_face<mesh_type>("clip_face.svg", face, geometry::alignment(normal, right, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}));

right = normalize(vertex_position(face.halfedge()) - vertex_centroid(face));
geometry::Orientation<vector_traits> orientation(normal, right);
auto projection = geometry::Projection(orientation);
brep::Visualizer<mesh_type> visualizer(projection, 0.05, 0.1);
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
visualizer.set_scale(0.05, 0.05);
visualizer.draw_face(face);
visualizer.write("clip_face_projection2.svg");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
/*
    name = "mosely_snowflake_heavy_4";
    cout << name << endl;
    mesh.clear();
    create_mosely_snowflake(mesh, name, 4, 100.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_heavy_4_diagonal";
    cout << name << endl;
    transform(mesh, matrix);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_heavy_4_diagonal_half";
    cout << name << endl;
    brep::clip(mesh, plane, "slice");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_4";
    cout << name << endl;
    mesh.clear();
    create_mosely_snowflake(mesh, name, 4, 100.0f, false);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_4_diagonal";
    cout << name << endl;
    transform(mesh, matrix);
    model::write_obj(mesh, to_filename(name));

    name = "mosely_snowflake_light_4_diagonal_half";
    cout << name << endl;
    brep::clip(mesh, plane, "slice");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
*/

    validator.report_results();
    return 0;
}
