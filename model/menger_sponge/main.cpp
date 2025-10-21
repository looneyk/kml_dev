//------------------------------------------------------------------------------
// menger_sponge
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/clip.hpp"
#include "quetzal/brep/split.hpp"
#include "quetzal/brep/connection.hpp"
#include "quetzal/brep/triangulation.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/geometry/Plane.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/model/mesh_attributes.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include <iostream>
#include <string>
#include <cassert>

using namespace std;
using namespace quetzal;

namespace g = quetzal::geometry;

namespace
{

    using value_type = float;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;

    static const filesystem::path dir_out = "out/";
    static const filesystem::path dir_ref = "reference/";
    static const string app_name = "menger_sponge";

    //--------------------------------------------------------------------------
    filesystem::path to_filename(const string& name)
    {
        return dir_out / (name + ".obj");
    }

    void check_vertexes(mesh_type& mesh)
    {
//mesh;
        for (const auto& vertex : mesh.vertices())
        {
            assert(math::float_eq(vertex.attributes().normal().norm(), 1.0f));
//            assert(math::float_eq(vertex.attributes().position().w(), 1.0f));
//            assert(math::float_eq0(vertex.attributes().normal().w()));
        }
    }

    //--------------------------------------------------------------------------
    void iterate(mesh_type& mesh, value_type size)
    {
        assert(mesh.submesh_count() == 1);
        string name = mesh.submesh(0).name();

        mesh_type mCell = move(mesh);
        mesh.clear();
        mesh_type m;

        m = mCell;
        m.rename_submesh(0, "cell_0");
        model::transform_position(m, g::translation(-size, -size, -size));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_1");
        model::transform_position(m, g::translation(0.0f, -size, -size));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_2");
        model::transform_position(m, g::translation(size, -size, -size));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_3");
        model::transform_position(m, g::translation(-size, 0.0f, -size));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_4");
        model::transform_position(m, g::translation(size, 0.0f, -size));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_5");
        model::transform_position(m, g::translation(-size, size, -size));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_6");
        model::transform_position(m, g::translation(0.0f, size, -size));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_7");
        model::transform_position(m, g::translation(size, size, -size));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_8");
        model::transform_position(m, g::translation(-size, -size, 0.0f));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_9");
        model::transform_position(m, g::translation(size, -size, 0.0f));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_10");
        model::transform_position(m, g::translation(-size, size, 0.0f));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_11");
        model::transform_position(m, g::translation(size, size, 0.0f));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_12");
        model::transform_position(m, g::translation(-size, -size, size));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_13");
        model::transform_position(m, g::translation(0.0f, -size, size));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_14");
        model::transform_position(m, g::translation(size, -size, size));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_15");
        model::transform_position(m, g::translation(-size, 0.0f, size));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_16");
        model::transform_position(m, g::translation(size, 0.0f, size));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_17");
        model::transform_position(m, g::translation(-size, size, size));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_18");
        model::transform_position(m, g::translation(0.0f, size, size));
        mesh.insert(m);

        m = mCell;
        m.rename_submesh(0, "cell_19");
        model::transform_position(m, g::translation(size, size, size));
        mesh.insert(m);

        // weld makes no changes to surface and submesh membership, so it is left in an inconsistent state
        // necessary so that cell addressing will continue to work
        weld(mesh, "cell_0/right", "cell_1/left");
        weld(mesh, "cell_1/right", "cell_2/left");
        weld(mesh, "cell_0/front", "cell_3/back");
        weld(mesh, "cell_2/front", "cell_4/back");
        weld(mesh, "cell_3/front", "cell_5/back");
        weld(mesh, "cell_5/right", "cell_6/left");
        weld(mesh, "cell_6/right", "cell_7/left");
        weld(mesh, "cell_4/front", "cell_7/back");
        weld(mesh, "cell_0/top", "cell_8/bottom");
        weld(mesh, "cell_2/top", "cell_9/bottom");
        weld(mesh, "cell_5/top", "cell_10/bottom");
        weld(mesh, "cell_7/top", "cell_11/bottom");
        weld(mesh, "cell_8/top", "cell_12/bottom");
        weld(mesh, "cell_12/right", "cell_13/left");
        weld(mesh, "cell_13/right", "cell_14/left");
        weld(mesh, "cell_9/top", "cell_14/bottom");
        weld(mesh, "cell_12/front", "cell_15/back");
        weld(mesh, "cell_14/front", "cell_16/back");
        weld(mesh, "cell_15/front", "cell_17/back");
        weld(mesh, "cell_10/top", "cell_17/bottom");
        weld(mesh, "cell_17/right", "cell_18/left");
        weld(mesh, "cell_18/right", "cell_19/left");
        weld(mesh, "cell_16/front", "cell_19/back");
        weld(mesh, "cell_11/top", "cell_19/bottom");

        // Reassign all interior surfaces to internal surface of primary submesh

        std::string nameSurfaceInternal = name + "/internal";

        mesh.move_surface("cell_1/front", nameSurfaceInternal);
        mesh.move_surface("cell_1/top", nameSurfaceInternal);
        mesh.move_surface("cell_3/right", nameSurfaceInternal);
        mesh.move_surface("cell_3/top", nameSurfaceInternal);
        mesh.move_surface("cell_4/left", nameSurfaceInternal);
        mesh.move_surface("cell_4/top", nameSurfaceInternal);
        mesh.move_surface("cell_6/back", nameSurfaceInternal);
        mesh.move_surface("cell_6/top", nameSurfaceInternal);
        mesh.move_surface("cell_8/right", nameSurfaceInternal);
        mesh.move_surface("cell_8/front", nameSurfaceInternal);
        mesh.move_surface("cell_9/front", nameSurfaceInternal);
        mesh.move_surface("cell_9/left", nameSurfaceInternal);
        mesh.move_surface("cell_10/right", nameSurfaceInternal);
        mesh.move_surface("cell_10/back", nameSurfaceInternal);
        mesh.move_surface("cell_11/left", nameSurfaceInternal);
        mesh.move_surface("cell_11/back", nameSurfaceInternal);
        mesh.move_surface("cell_13/bottom", nameSurfaceInternal);
        mesh.move_surface("cell_13/front", nameSurfaceInternal);
        mesh.move_surface("cell_15/bottom", nameSurfaceInternal);
        mesh.move_surface("cell_15/right", nameSurfaceInternal);
        mesh.move_surface("cell_16/bottom", nameSurfaceInternal);
        mesh.move_surface("cell_16/left", nameSurfaceInternal);
        mesh.move_surface("cell_18/bottom", nameSurfaceInternal);
        mesh.move_surface("cell_18/back", nameSurfaceInternal);

        // Ensure that all surfaces are part of the main submesh

        for (auto& surface : mesh.surfaces())
        {
            if (surface.submesh().name() != name)
            {
                mesh.move_surface(surface.id(), name + "/" + surface.name());
            }
        }

        mesh.pack();
        return;
    }

    //--------------------------------------------------------------------------
    void create_menger_sponge(mesh_type& mesh, const string& name, size_t nLevel, value_type scale)
    {
        mesh_type m;
        value_type size = 1.0f;
        model::create_box(m, name, size, size, size);

        for (size_t i = 0; i < nLevel; ++i)
        {
            cout << "Level " << i << " size " << size << endl;
            iterate(m, size);
            size *= 3.0f;
        }
check_vertexes(m);

        model::transform_position(m, g::scaling(scale / size));
check_vertexes(m);
        mesh.insert(m);
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
    mesh_type mesh_diagonal;

    mesh_type::vector_type diagonal = math::normalize<vector_traits>({1.0f, 1.0f, 1.0f});
    mesh_type::value_type angle = angle_between_unit(diagonal, {0.0f, 0.0f, 1.0f});
    mesh_type::vector_type axis = normalize(cross(diagonal, {0.0f, 0.0f, 1.0f}));
    auto matrix = g::rotation_axis_unit(axis, angle);

    mesh_type::point_type point = {0.0f, 0.0f, 0.0f};
    g::Plane<vector_traits> planeUpper(point, {0.0f, 0.0f, -1.0f});
    g::Plane<vector_traits> planeLower(point, {0.0f, 0.0f, 1.0f});

    name = "menger_sponge_0";
    cout << name << endl;
    mesh.clear();
    create_menger_sponge(mesh, name, 0, 100.0f);
check_vertexes(mesh);
    triangulate(mesh);
check_vertexes(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "menger_sponge_0_diagonal";
    cout << name << endl;
check_vertexes(mesh);
    model::transform(mesh, matrix);
check_vertexes(mesh);
    model::write_obj(mesh, to_filename(name));

    mesh_diagonal = mesh;

    name = "menger_sponge_0_diagonal_half";
    cout << name << endl;
    brep::clip(mesh_diagonal, planeUpper, "slice");
check_vertexes(mesh_diagonal);
    mesh_diagonal.pack();
    triangulate(mesh_diagonal);
    model::write_obj(mesh_diagonal, to_filename(name));

    mesh_diagonal = mesh;

    name = "menger_sponge_0_diagonal_half_lower";
    cout << name << endl;
    brep::clip(mesh_diagonal, planeLower, "slice");
    mesh_diagonal.pack();
    triangulate(mesh_diagonal);
    model::write_obj(mesh_diagonal, to_filename(name));

    name = "menger_sponge_0_diagonal_halves";
    cout << name << endl;
    brep::split(mesh, planeUpper, "slice");
    model::transform(mesh.submesh("menger_sponge_0_0"), g::translation(0.0f, 0.0f, 10.0f));
    mesh.pack();
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

// validate will produce normal and texcoord warnings because the surfaces are not corrected after welding ...

    name = "menger_sponge_1";
    cout << name << endl;
    mesh.clear();
    create_menger_sponge(mesh, name, 1, 100.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
//validate(mesh);

    name = "menger_sponge_1_diagonal";
    cout << name << endl;
    model::transform(mesh, matrix);
    model::write_obj(mesh, to_filename(name));

    mesh_diagonal = mesh;

    name = "menger_sponge_1_diagonal_half";
    cout << name << endl;
    brep::clip(mesh_diagonal, planeUpper, "slice");
    mesh_diagonal.pack();
    triangulate(mesh_diagonal);
    model::write_obj(mesh_diagonal, to_filename(name));
//validate(mesh_diagonal);

    mesh_diagonal = mesh;

    name = "menger_sponge_1_diagonal_half_lower";
    cout << name << endl;
    brep::clip(mesh_diagonal, planeLower, "slice");
    mesh_diagonal.pack();
    triangulate(mesh_diagonal);
    model::write_obj(mesh_diagonal, to_filename(name));
//validate(mesh_diagonal);

    name = "menger_sponge_1_diagonal_halves";
    cout << name << endl;
    brep::split(mesh, planeUpper, "slice");
    model::transform(mesh.submesh("menger_sponge_1_0"), g::translation(0.0f, 0.0f, 10.0f));
    mesh.pack();
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
//validate(mesh);

    name = "menger_sponge_2";
    cout << name << endl;
    mesh.clear();
    create_menger_sponge(mesh, name, 2, 100.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
//validate(mesh);

    name = "menger_sponge_2_diagonal";
    cout << name << endl;
    model::transform(mesh, matrix);
    model::write_obj(mesh, to_filename(name));

    mesh_diagonal = mesh;

    name = "menger_sponge_2_diagonal_half";
    cout << name << endl;
//mesh_diagonal.clear();
//create_menger_sponge(mesh_diagonal, name, 2, 100.0f);
//model::transform(mesh_diagonal, matrix);
    brep::clip(mesh_diagonal, planeUpper, "slice");
    mesh_diagonal.pack();
    triangulate(mesh_diagonal);
    model::write_obj(mesh_diagonal, to_filename(name));
//validate(mesh_diagonal);

    mesh_diagonal = mesh;

    name = "menger_sponge_2_diagonal_half_lower";
    cout << name << endl;
    brep::clip(mesh_diagonal, planeLower, "slice");
    mesh_diagonal.pack();
    triangulate(mesh_diagonal);
    model::write_obj(mesh_diagonal, to_filename(name));
//validate(mesh_diagonal);

    name = "menger_sponge_2_diagonal_halves";
    cout << name << endl;
    brep::split(mesh, planeUpper, "slice");
    model::transform(mesh.submesh("menger_sponge_2_0"), g::translation(0.0f, 0.0f, 10.0f));
    mesh.pack();
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
//validate(mesh);
/*
    name = "menger_sponge_3";
    cout << name << endl;
    mesh.clear();
    create_menger_sponge(mesh, name, 3, 100.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "menger_sponge_3_diagonal";
    cout << name << endl;
    model::transform(mesh, matrix);
    model::write_obj(mesh, to_filename(name));

    mesh_diagonal = mesh;

    name = "menger_sponge_3_diagonal_half";
    cout << name << endl;
    brep::clip(mesh_diagonal, planeUpper, "slice");
    mesh_diagonal.pack();
    triangulate(mesh_diagonal);
    model::write_obj(mesh_diagonal, to_filename(name));
//validate(mesh_diagonal);

    mesh_diagonal = mesh;

    name = "menger_sponge_3_diagonal_half_lower";
    cout << name << endl;
    brep::clip(mesh_diagonal, planeLower, "slice");
    mesh_diagonal.pack();
    triangulate(mesh_diagonal);
    model::write_obj(mesh_diagonal, to_filename(name));
//validate(mesh_diagonal);

    name = "menger_sponge_3_diagonal_halves";
    cout << name << endl;
    brep::split(mesh, planeUpper, "slice");
    model::transform(mesh.submesh("menger_sponge_3_0"), g::translation(0.0f, 0.0f, 10.0f));
    mesh.pack();
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
//validate(mesh);
/*
    name = "menger_sponge_4";
    cout << name << endl;
    mesh.clear();
    create_menger_sponge(mesh, name, 4, 100.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
*/

    validator.report_results();
    return 0;
}
