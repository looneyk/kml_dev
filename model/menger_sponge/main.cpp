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

    //--------------------------------------------------------------------------
    void check_vertices(mesh_type& mesh)
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
    void iterate(mesh_type& mesh, id_type submesh_id, value_type size)
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
            {"cell_0", {-size, -size, -size}},
            {"cell_1", {0.0f, -size, -size}},
            {"cell_2", {size, -size, -size}},
            {"cell_3", {-size, 0.0f, -size}},
            {"cell_4", {size, 0.0f, -size}},
            {"cell_5", {-size, size, -size}},
            {"cell_6", {0.0f, size, -size}},
            {"cell_7", {size, size, -size}},
            {"cell_8", {-size, -size, 0.0f}},
            {"cell_9", {size, -size, 0.0f}},
            {"cell_10", {-size, size, 0.0f}},
            {"cell_11", {size, size, 0.0f}},
            {"cell_12", {-size, -size, size}},
            {"cell_13", {0.0f, -size, size}},
            {"cell_14", {size, -size, size}},
            {"cell_15", {-size, 0.0f, size}},
            {"cell_16", {size, 0.0f, size}},
            {"cell_17", {-size, size, size}},
            {"cell_18", {0.0f, size, size}},
            {"cell_19", {size, size, size}},
        };

        for (const auto& placement : placements)
        {
            mesh.insert(m, cell_id, placement.name, [&placement](typename mesh_type::vertex_type vertex) -> typename mesh_type::vertex_type { vertex.attributes().position() *= g::translation(placement.offset); return vertex; });
        }

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

        std::string surface_name_internal = name + "/internal";

        mesh.move_surface("cell_1/front", surface_name_internal);
        mesh.move_surface("cell_1/top", surface_name_internal);
        mesh.move_surface("cell_3/right", surface_name_internal);
        mesh.move_surface("cell_3/top", surface_name_internal);
        mesh.move_surface("cell_4/left", surface_name_internal);
        mesh.move_surface("cell_4/top", surface_name_internal);
        mesh.move_surface("cell_6/back", surface_name_internal);
        mesh.move_surface("cell_6/top", surface_name_internal);
        mesh.move_surface("cell_8/right", surface_name_internal);
        mesh.move_surface("cell_8/front", surface_name_internal);
        mesh.move_surface("cell_9/front", surface_name_internal);
        mesh.move_surface("cell_9/left", surface_name_internal);
        mesh.move_surface("cell_10/right", surface_name_internal);
        mesh.move_surface("cell_10/back", surface_name_internal);
        mesh.move_surface("cell_11/left", surface_name_internal);
        mesh.move_surface("cell_11/back", surface_name_internal);
        mesh.move_surface("cell_13/bottom", surface_name_internal);
        mesh.move_surface("cell_13/front", surface_name_internal);
        mesh.move_surface("cell_15/bottom", surface_name_internal);
        mesh.move_surface("cell_15/right", surface_name_internal);
        mesh.move_surface("cell_16/bottom", surface_name_internal);
        mesh.move_surface("cell_16/left", surface_name_internal);
        mesh.move_surface("cell_18/bottom", surface_name_internal);
        mesh.move_surface("cell_18/back", surface_name_internal);

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
        value_type size = 1.0f;
        id_type submesh_id = model::create_box(mesh, name, size, size, size);

        for (size_t i = 0; i < nLevel; ++i)
        {
            cout << "Level " << i << " size " << size << endl;
            iterate(mesh, submesh_id, size);
            size *= 3.0f;
        }
check_vertices(mesh);

        model::transform_position(mesh, g::scaling(scale / size));
check_vertices(mesh);
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
check_vertices(mesh);
    triangulate(mesh);
check_vertices(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "menger_sponge_0_diagonal";
    cout << name << endl;
check_vertices(mesh);
    model::transform(mesh, matrix);
check_vertices(mesh);
    model::write_obj(mesh, to_filename(name));

    mesh_diagonal = mesh;

    name = "menger_sponge_0_diagonal_half";
    cout << name << endl;
    brep::clip(mesh_diagonal, planeUpper, "slice");
check_vertices(mesh_diagonal);
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

    name = "menger_sponge_3";
    cout << name << endl;
    mesh.clear();
    create_menger_sponge(mesh, name, 3, 100.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
//validate(mesh);

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
