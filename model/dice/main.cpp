//------------------------------------------------------------------------------
// dice
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/connection.hpp"
#include "quetzal/brep/inversion.hpp"
#include "quetzal/brep/triangulation.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/math/CartesianTraits.hpp"
#include "quetzal/math/transformation_matrix.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include <iostream>
#include <string>
#include <cassert>

#include "quetzal/brep/validation.hpp"

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
    using vector_traits = math::CartesianTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;
    using matrix_type = math::Matrix<value_type>;

    static const string dirOut = "out/";
    static const string dirReference = "reference/";

    //--------------------------------------------------------------------------
    string to_filename(const string& name)
    {
        return dirOut + name + ".obj";
    }

    //--------------------------------------------------------------------------
    model::Extent<value_type> unbanded_side_extent(value_type tLower, value_type tUpper)
    {
        return model::Extent<value_type>(tLower, tUpper, {model::Termination::Type::Center, false}, {model::Termination::Type::Center, false}, model::SurfaceName::Side0, model::SurfaceName::Side1);
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
            cout << "    " << surface.id() << " " << surface.name() << " " << surface.face_count() << " faces " << (surface.deleted() ? "D" : "") << endl;
        }
        cout << "  submesh index" << endl;
        for (const auto e : mesh.submesh_index())
        {
            cout << "    " << e.first << " " << e.second << endl;
        }
        cout << "  surface index" << endl;
        for (const auto e : mesh.surface_index())
        {
            cout << "    " << e.first << " " << e.second << endl;
        }

        return;
    }

    //--------------------------------------------------------------------------
    void create_d6_blank(mesh_type& mesh, const string& name, value_type extent, value_type r)
    {
        mesh_type m;

        value_type extentInner = extent - 2.0f * r;
        value_type inner = 0.5f * extentInner;
        value_type offset = inner + 0.5f * r;

        m.clear();
        model::create_sphere(m, "corner0", 12, 12, r, unbanded_side_extent(0.5f, 0.75f), model::ExtentEndsFlat<value_type>(0.0f, 0.5f));
        model::transform(m, math::translation(-inner, -inner, -inner));
        mesh.insert(m);

        m.clear();
        model::create_sphere(m, "corner1", 12, 12, r, unbanded_side_extent(0.75f, 1.0f), model::ExtentEndsFlat<value_type>(0.0f, 0.5f));
        model::transform(m, math::translation(inner, -inner, -inner));
        mesh.insert(m);

        m.clear();
        model::create_sphere(m, "corner2", 12, 12, r, unbanded_side_extent(0.0f, 0.25f), model::ExtentEndsFlat<value_type>(0.0f, 0.5f));
        model::transform(m, math::translation(inner, inner, -inner));
        mesh.insert(m);

        m.clear();
        model::create_sphere(m, "corner3", 12, 12, r, unbanded_side_extent(0.25f, 0.5f), model::ExtentEndsFlat<value_type>(0.0f, 0.5f));
        model::transform(m, math::translation(-inner, inner, -inner));
        mesh.insert(m);

        m.clear();
        model::create_sphere(m, "corner4", 12, 12, r, unbanded_side_extent(0.5f, 0.75f), model::ExtentEndsFlat<value_type>(0.5f, 1.0f));
        model::transform(m, math::translation(-inner, -inner, inner));
        mesh.insert(m);

        m.clear();
        model::create_sphere(m, "corner5", 12, 12, r, unbanded_side_extent(0.75f, 1.0f), model::ExtentEndsFlat<value_type>(0.5f, 1.0f));
        model::transform(m, math::translation(inner, -inner, inner));
        mesh.insert(m);

        m.clear();
        model::create_sphere(m, "corner6", 12, 12, r, unbanded_side_extent(0.0f, 0.25f), model::ExtentEndsFlat<value_type>(0.5f, 1.0f));
        model::transform(m, math::translation(inner, inner, inner));
        mesh.insert(m);

        m.clear();
        model::create_sphere(m, "corner7", 12, 12, r, unbanded_side_extent(0.25f, 0.5f), model::ExtentEndsFlat<value_type>(0.5f, 1.0f));
        model::transform(m, math::translation(-inner, inner, inner));
        mesh.insert(m);

        m.clear();
        model::create_cylinder(m, "edgez0", 12, 1, r, r, -inner, inner, unbanded_side_extent(0.5f, 0.75f));
        model::transform(m, math::translation(-inner, -inner, 0.0f));
        mesh.insert(m);

        m.clear();
        model::create_cylinder(m, "edgez1", 12, 1, r, r, -inner, inner, unbanded_side_extent(0.75f, 1.0f));
        model::transform(m, math::translation(inner, -inner, 0.0f));
        mesh.insert(m);

        m.clear();
        model::create_cylinder(m, "edgez2", 12, 1, r, r, -inner, inner, unbanded_side_extent(0.0f, 0.25f));
        model::transform(m, math::translation(inner, inner, 0.0f));
        mesh.insert(m);

        m.clear();
        model::create_cylinder(m, "edgez3", 12, 1, r, r, -inner, inner, unbanded_side_extent(0.25f, 0.5f));
        model::transform(m, math::translation(-inner, inner, 0.0f));
        mesh.insert(m);

        m.clear();
        model::create_cylinder(m, "edgey0", 12, 1, r, r, -inner, inner, unbanded_side_extent(0.5f, 0.75f));
        model::transform(m, math::rotation_x(-math::PiHalf<value_type>));
        model::transform(m, math::translation(-inner, 0.0f, inner));
        mesh.insert(m);

        m.clear();
        model::create_cylinder(m, "edgey1", 12, 1, r, r, -inner, inner, unbanded_side_extent(0.75f, 1.0f));
        model::transform(m, math::rotation_x(-math::PiHalf<value_type>));
        model::transform(m, math::translation(inner, 0.0f, inner));
        mesh.insert(m);

        m.clear();
        model::create_cylinder(m, "edgey2", 12, 1, r, r, -inner, inner, unbanded_side_extent(0.0f, 0.25f));
        model::transform(m, math::rotation_x(-math::PiHalf<value_type>));
        model::transform(m, math::translation(inner, 0.0f, -inner));
        mesh.insert(m);

        m.clear();
        model::create_cylinder(m, "edgey3", 12, 1, r, r, -inner, inner, unbanded_side_extent(0.25f, 0.5f));
        model::transform(m, math::rotation_x(-math::PiHalf<value_type>));
        model::transform(m, math::translation(-inner, 0.0f, -inner));
        mesh.insert(m);

        m.clear();
        model::create_cylinder(m, "edgex0", 12, 1, r, r, -inner, inner, unbanded_side_extent(0.5f, 0.75f));
        model::transform(m, math::rotation_y(math::PiHalf<value_type>));
        model::transform(m, math::translation(0.0f, -inner, inner));
        mesh.insert(m);

        m.clear();
        model::create_cylinder(m, "edgex1", 12, 1, r, r, -inner, inner, unbanded_side_extent(0.75f, 1.0f));
        model::transform(m, math::rotation_y(math::PiHalf<value_type>));
        model::transform(m, math::translation(0.0f, -inner, -inner));
        mesh.insert(m);

        m.clear();
        model::create_cylinder(m, "edgex2", 12, 1, r, r, -inner, inner, unbanded_side_extent(0.0f, 0.25f));
        model::transform(m, math::rotation_y(math::PiHalf<value_type>));
        model::transform(m, math::translation(0.0f, inner, -inner));
        mesh.insert(m);

        m.clear();
        model::create_cylinder(m, "edgex3", 12, 1, r, r, -inner, inner, unbanded_side_extent(0.25f, 0.5f));
        model::transform(m, math::rotation_y(math::PiHalf<value_type>));
        model::transform(m, math::translation(0.0f, inner, inner));
        mesh.insert(m);

        m.clear();
        model::create_box(m, "facez0", extentInner, extentInner, r);
        model::transform(m, math::translation(0.0f, 0.0f, -offset));
        mesh.insert(m);

        m.clear();
        model::create_box(m, "facez1", extentInner, extentInner, r);
        model::transform(m, math::translation(0.0f, 0.0f, offset));
        mesh.insert(m);

        m.clear();
        model::create_box(m, "facey0", extentInner, r, extentInner);
        model::transform(m, math::translation(0.0f, -offset, 0.0f));
        mesh.insert(m);

        m.clear();
        model::create_box(m, "facey1", extentInner, r, extentInner);
        model::transform(m, math::translation(0.0f, offset, 0.0f));
        mesh.insert(m);

        m.clear();
        model::create_box(m, "facex0", r, extentInner, extentInner);
        model::transform(m, math::translation(-offset, 0.0f, 0.0f));
        mesh.insert(m);

        m.clear();
        model::create_box(m, "facex1", r, extentInner, extentInner);
        model::transform(m, math::translation(offset, 0.0f, 0.0f));
        mesh.insert(m);

        m.clear();
        model::create_box(m, "body", extentInner, extentInner, extentInner);
        mesh.insert(m);

        weld(mesh, "body/bottom", "facez0/top");
        weld(mesh, "body/right", "facex1/left");
        weld(mesh, "body/front", "facey1/back");
        weld(mesh, "body/left", "facex0/right");
        weld(mesh, "body/back", "facey0/front");
        weld(mesh, "body/top", "facez1/bottom");

        weld(mesh, "facez0/right", "edgey2/side1");
        weld(mesh, "facez0/front", "edgex2/side0");
        weld(mesh, "facez0/left", "edgey3/side0");
        weld(mesh, "facez0/back", "edgex1/side1");
        weld(mesh, "facey0/bottom", "edgex1/side0");
        weld(mesh, "facey0/right", "edgez1/side0");
        weld(mesh, "facey0/top", "edgex0/side1");
        weld(mesh, "facey0/left", "edgez0/side1");
        weld(mesh, "facex1/bottom", "edgey2/side0");
        weld(mesh, "facex1/front", "edgez2/side0");
        weld(mesh, "facex1/top", "edgey1/side1");
        weld(mesh, "facex1/back", "edgez1/side1");
        weld(mesh, "facey1/bottom", "edgex2/side1");
        weld(mesh, "facey1/right", "edgez2/side1");
        weld(mesh, "facey1/top", "edgex3/side0");
        weld(mesh, "facey1/left", "edgez3/side0");
        weld(mesh, "facex0/bottom", "edgey3/side1");
        weld(mesh, "facex0/front", "edgez3/side1");
        weld(mesh, "facex0/top", "edgey0/side0");
        weld(mesh, "facex0/back", "edgez0/side0");
        weld(mesh, "facez1/right", "edgey1/side0");
        weld(mesh, "facez1/front", "edgex3/side1");
        weld(mesh, "facez1/left", "edgey0/side1");
        weld(mesh, "facez1/back", "edgex0/side0");

        weld(mesh, "edgez0/end0", "corner0/end1");
        weld(mesh, "edgez0/end1", "corner4/end0");
        weld(mesh, "edgez1/end0", "corner1/end1");
        weld(mesh, "edgez1/end1", "corner5/end0");
        weld(mesh, "edgez2/end0", "corner2/end1");
        weld(mesh, "edgez2/end1", "corner6/end0");
        weld(mesh, "edgez3/end0", "corner3/end1");
        weld(mesh, "edgez3/end1", "corner7/end0");

        weld(mesh, "edgey0/end0", "corner4/side0");
        weld(mesh, "edgey0/end1", "corner7/side1");
        weld(mesh, "edgey1/end0", "corner5/side1");
        weld(mesh, "edgey1/end1", "corner6/side0");
        weld(mesh, "edgey2/end0", "corner1/side1");
        weld(mesh, "edgey2/end1", "corner2/side0");
        weld(mesh, "edgey3/end0", "corner0/side0");
        weld(mesh, "edgey3/end1", "corner3/side1");

        weld(mesh, "edgex0/end0", "corner4/side1");
        weld(mesh, "edgex0/end1", "corner5/side0");
        weld(mesh, "edgex1/end0", "corner0/side1");
        weld(mesh, "edgex1/end1", "corner1/side0");
        weld(mesh, "edgex2/end0", "corner3/side0");
        weld(mesh, "edgex2/end1", "corner2/side1");
        weld(mesh, "edgex3/end0", "corner7/side0");
        weld(mesh, "edgex3/end1", "corner6/side1");

        for (auto& surface : mesh.surfaces())
        {
            assert(!surface.empty());
            string nameDest = name + "/" + surface.submesh().name();
            mesh.move_surface(surface.id(), nameDest);
        }

        mesh.pack();
        return;
    }

    //--------------------------------------------------------------------------
    void append_instance(mesh_type& mesh, mesh_type instance, const string& nameSurfaceA, const string& nameSurfaceB, const matrix_type& matrix)
    {
        model::transform(instance, matrix);
        mesh.insert(instance);
        const auto& surfaceA = mesh.surface(nameSurfaceA);
        const auto& surfaceB = mesh.surface(nameSurfaceB);
        const string nameSubmeshA = surfaceA.submesh().name();
        const string nameSubmeshB = surfaceB.submesh().name();
        assert(surfaceA.faces().size() == 1);
        assert(surfaceB.faces().size() == 1);
        id_type idFaceA = surfaceA.faces().front().id();
        id_type idFaceB = surfaceB.faces().front().id();
        brep::attach(mesh, idFaceA, idFaceB);
        mesh.move_surface(nameSubmeshB + "/" + "body", nameSubmeshA + "/" + nameSubmeshB); // a version of extended_name that simple correctly concatenates with / ...
        return;
    }

    //--------------------------------------------------------------------------
    void apply_six(mesh_type& mesh, const mesh_type& m, const std::string nameSurfaceA, const std::string nameSurfaceB, const matrix_type& matrix)
    {
        append_instance(mesh, m, nameSurfaceA, nameSurfaceB, math::translation(-0.25f, -0.25f, 0.0f) * matrix);
        append_instance(mesh, m, nameSurfaceA, nameSurfaceB, math::translation(0.0f, -0.25f, 0.0f) * matrix);
        append_instance(mesh, m, nameSurfaceA, nameSurfaceB, math::translation(0.25f, -0.25f, 0.0f) * matrix);
        append_instance(mesh, m, nameSurfaceA, nameSurfaceB, math::translation(-0.25f, 0.25f, 0.0f) * matrix);
        append_instance(mesh, m, nameSurfaceA, nameSurfaceB, math::translation(0.0f, 0.25f, 0.0f) * matrix);
        append_instance(mesh, m, nameSurfaceA, nameSurfaceB, math::translation(0.25f, 0.25f, 0.0f) * matrix);
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
    mesh_type m;

    id_type idFaceA = nullid;
    id_type idFaceB = nullid;
/*
    name = "d6_blank";
    cout << name << endl;
    create_d6_blank(mesh, name, 1.0f, 0.125f);
    model::transform(mesh, math::scaling(16.0f));
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "d6_dot";
    cout << name << endl;
    mesh.clear();
    create_d6_blank(mesh, "body", 1.0f, 0.125f);
    m.clear();
    model::create_sphere(m, "pip", 24, 12, 0.125f, {}, model::ExtentEndsFlat<value_type>(0.0f, 0.5f));
    model::transform(m, math::translation(0.0f, 0.0f, 0.5f));
    invert(m);
    mesh.insert(m);
    idFaceA = mesh.surface("body/facez1").faces().front().id();
    idFaceB = mesh.surface("pip/end1").faces().front().id();
    brep::attach(mesh, idFaceA, idFaceB);
    mesh.move_surface("pip/body", "body/pip");
    mesh.pack();
    model::transform(mesh, math::scaling(16.0f));
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "d6_bump";
    cout << name << endl;
    mesh.clear();
    create_d6_blank(mesh, "body", 1.0f, 0.125f);
    m.clear();
    model::create_sphere(m, "pip", 24, 12, 0.125f, {}, model::ExtentEndsFlat<value_type>(0.5f, 1.0f));
    model::transform(m, math::translation(0.0f, 0.0f, 0.5f));
    mesh.insert(m);
    idFaceA = mesh.surface("body/facez1").faces().front().id();
    idFaceB = mesh.surface("pip/end0").faces().front().id();
    brep::attach(mesh, idFaceA, idFaceB);
    mesh.move_surface("pip/body", "body/pip");
    mesh.pack();
    model::transform(mesh, math::scaling(16.0f));
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
*/
    name = "d666";
    cout << name << endl;
    mesh.clear();
    create_d6_blank(mesh, "body", 1.0f, 0.125f);
    m.clear();
    model::create_sphere(m, "pip", 24, 12, 0.1f, {}, model::ExtentEndsFlat<value_type>(0.0f, 0.5f));
    invert(m);
//    append_instance(mesh, m, "body/facez1", "pip/end1", math::translation(-0.25f, -0.25f, 0.5f));
//    append_instance(mesh, m, "body/facez1", "pip/end1", math::translation(0.0f, -0.25f, 0.5f));
//    append_instance(mesh, m, "body/facez1", "pip/end1", math::translation(0.25f, -0.25f, 0.5f));
//    append_instance(mesh, m, "body/facez1", "pip/end1", math::translation(-0.25f, 0.25f, 0.5f));
//    append_instance(mesh, m, "body/facez1", "pip/end1", math::translation(0.0f, 0.25f, 0.5f));
//    append_instance(mesh, m, "body/facez1", "pip/end1", math::translation(0.25f, 0.25f, 0.5f));
    apply_six(mesh, m, "body/facez1", "pip/end1", math::translation(0.0f, 0.0f, 0.5f));
    apply_six(mesh, m, "body/facez0", "pip/end1", math::rotation_x3(math::Pi<value_type>) * math::translation(0.0f, 0.0f, -0.5f));
    apply_six(mesh, m, "body/facey1", "pip/end1", math::rotation_z3(math::PiHalf<value_type>) * math::rotation_x(-math::PiHalf<value_type>) * math::translation(0.0f, 0.5f, 0.0f));
    apply_six(mesh, m, "body/facey0", "pip/end1", math::rotation_z3(math::PiHalf<value_type>) * math::rotation_x(math::PiHalf<value_type>) * math::translation(0.0f, -0.5f, 0.0f));
    apply_six(mesh, m, "body/facex1", "pip/end1", math::rotation_z3(math::PiHalf<value_type>) * math::rotation_y(math::PiHalf<value_type>) * math::translation(0.5f, 0.0f, 0.0f));
    apply_six(mesh, m, "body/facex0", "pip/end1", math::rotation_z3(math::PiHalf<value_type>) * math::rotation_y(-math::PiHalf<value_type>) * math::translation(-0.5f, 0.0f, 0.0f));
    mesh.pack();
    model::transform(mesh, math::scaling(16.0f));
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
/*
    name = "bump";
    cout << name << endl;
    mesh.clear();
    model::create_box(mesh, "body", 1.0f, 1.0f, 1.0f);
    m.clear();
    model::create_sphere(m, "pip", 6, 3, 0.125f, {}, model::ExtentEndsFlat<value_type>(0.5f, 1.0f));
    model::transform(m, math::translation(0.0f, 0.0f, 0.5f));
    mesh.insert(m);
    idFaceA = mesh.surface("body/top").faces().front().id();
    idFaceB = mesh.surface("pip/end0").faces().front().id();
    brep::attach(mesh, idFaceA, idFaceB);
    mesh.pack();
    model::transform(mesh, math::scaling(16.0f));
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "dot";
    cout << name << endl;
    mesh.clear();
    model::create_box(mesh, "body", 1.0f, 1.0f, 1.0f);
    m.clear();
    model::create_sphere(m, "pip", 24, 12, 0.125f, {}, model::ExtentEndsFlat<value_type>(0.0f, 0.5f));
    model::transform(m, math::translation(0.0f, 0.0f, 0.5f));
    invert(m);
    mesh.insert(m);
    idFaceA = mesh.surface("body/top").faces().front().id();
    idFaceB = mesh.surface("pip/end1").faces().front().id();
    brep::attach(mesh, idFaceA, idFaceB);
    mesh.pack();
    model::transform(mesh, math::scaling(16.0f));
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
*/
    validator.report_results();
    return 0;
}
