//------------------------------------------------------------------------------
// sandbox
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/connection.hpp"
#include "quetzal/brep/triangulation.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/math/Interval.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include "quetzal/model/mesh_attributes.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include <filesystem>
#include <iostream>
#include <string>
#include <cassert>

#include "quetzal/brep/connection2.hpp"

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;

    const filesystem::path dirOut = "out/";
    const filesystem::path dirReference = "reference/";

    //--------------------------------------------------------------------------
    filesystem::path to_filename(const string& name)
    {
        return dirOut / (name + ".obj");
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
/*
    name = "first_layer_block";
    cout << name << endl;
    mesh.clear();
    model::create_box(mesh, name, 125.0f, 25.0f, 5.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "axis_shaft";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 12, 1, 0.02f, 0.02f, 0.0f, 1.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "axis_head";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 12, 1, 0.05f, 0.0f, 0.0f, 0.2f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "axis_origin";
    cout << name << endl;
    mesh.clear();
    model::create_icosahedron(mesh, name, 0.05f, true, true);
    model::write_obj(mesh, to_filename(name));

    name = "unit_cylinder";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 24, 1, 1.0f, 1.0f, 0.0f, 1.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "unit_cone";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 24, 1, 1.0f, 0.0f, 0.0f, 1.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "unit_icosahedron";
    cout << name << endl;
    mesh.clear();
    model::create_icosahedron(mesh, name, 1.0f, true, true);
    model::write_obj(mesh, to_filename(name));

    name = "unit_geosphere2";
    cout << name << endl;
    mesh.clear();
    model::create_geodesic_sphere(mesh, name, 1.0f, 2, true, true);
    model::write_obj(mesh, to_filename(name));
*/
    mesh_type m;

    name = "cubes_attach1";
    cout << name << endl;
    mesh.clear();
    model::create_box(mesh, "box0", 20.0f, 20.0f, 20.0f);
    m.clear();
    model::create_box(m, "box1", 20.0f, 20.0f, 20.0f);
    model::transform(m, geometry::translation(10.0f, 10.0f, 20.0f));
    mesh.insert(m);
    attach_x(mesh, "box0/top", "box1/bottom");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cubes_attach2";
    cout << name << endl;
    mesh.clear();
    model::create_box(mesh, "box0", 20.0f, 20.0f, 20.0f);
    m.clear();
    model::create_box(m, "box1", 20.0f, 20.0f, 20.0f);
    model::transform(m, geometry::rotation_z3(math::PiFourth<float>) * geometry::translation(7.0f, 7.0f, 20.0f));
    mesh.insert(m);
    attach_x(mesh, "box0/top", "box1/bottom");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cubes_attach3";
    cout << name << endl;
    mesh.clear();
    model::create_box(mesh, "box0", 20.0f, 20.0f, 20.0f);
    m.clear();
    model::create_box(m, "box1", 20.0f, 20.0f, 20.0f);
    model::transform(m, geometry::rotation_z3(math::PiFourth<float>) * geometry::translation(13.0f, 13.0f, 20.0f));
    mesh.insert(m);
    attach_x(mesh, "box0/top", "box1/bottom");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cubes_attach4";
    cout << name << endl;
    mesh.clear();
    model::create_box(mesh, "box0", 20.0f, 20.0f, 20.0f);
    m.clear();
    model::create_box(m, "box1", 20.0f, 20.0f, 20.0f);
    model::transform(m, geometry::rotation_z3(math::PiFourth<float>) * geometry::translation(0.0f, 0.0f, 20.0f));
    mesh.insert(m);
    attach_x(mesh, "box0/top", "box1/bottom");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cubes_attach5";
    cout << name << endl;
    mesh.clear();
    model::create_box(mesh, "box0", 20.0f, 20.0f, 20.0f);
    m.clear();
    model::create_box(m, "box1", 20.0f, 20.0f, 20.0f);
    model::transform(m, geometry::rotation_z3(math::PiFourth<float>) * geometry::translation(10.0f, 10.0f, 20.0f));
    mesh.insert(m);
    attach_x(mesh, "box0/top", "box1/bottom");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cubes_attach6";
    cout << name << endl;
    mesh.clear();
    model::create_box(mesh, "box0", 20.0f, 20.0f, 20.0f);
    m.clear();
    model::create_box(m, "box1", 20.0f, 20.0f, 20.0f);
    model::transform(m, geometry::rotation_z3(math::PiFourth<float>) * geometry::translation(-10.0f, 10.0f, 20.0f));
    mesh.insert(m);
    attach_x(mesh, "box0/top", "box1/bottom");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cubes_attach7";
    cout << name << endl;
    mesh.clear();
    model::create_box(mesh, "box0", 20.0f, 20.0f, 20.0f);
    m.clear();
    model::create_box(m, "box1", 20.0f, 20.0f, 20.0f);
    model::transform(m, geometry::rotation_z3(math::PiFourth<float>) * geometry::translation(10.0f, -4.1421356f, 20.0f));
    mesh.insert(m);
    attach_x(mesh, "box0/top", "box1/bottom");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cubes_attach8";
    cout << name << endl;
    mesh.clear();
    model::create_box(mesh, "box0", 20.0f, 20.0f, 20.0f);
    m.clear();
    model::create_box(m, "box1", 20.0f, 20.0f, 20.0f);
    model::transform(m, geometry::rotation_z3(math::PiFourth<float>) * geometry::translation(-10.0f, -4.1421356f, 20.0f));
    mesh.insert(m);
    attach_x(mesh, "box0/top", "box1/bottom");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cubes_attach9";
    cout << name << endl;
    mesh.clear();
    model::create_box(mesh, "box0", 20.0f, 20.0f, 20.0f);
    m.clear();
    model::create_cylinder(m, "hex1", 6, 1, 10.0f, 10.0f, -10.0f, 10.0);
    model::transform(m, geometry::translation(10.0f, 1.339746f, 20.0f));
    mesh.insert(m);
    attach_x(mesh, "box0/top", "hex1/end0");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cubes_attach10";
    cout << name << endl;
    mesh.clear();
    model::create_box(mesh, "box0", 20.0f, 20.0f, 20.0f);
    m.clear();
    model::create_cylinder(m, "hex1", 6, 1, 10.0f, 10.0f, -10.0f, 10.0);
    model::transform(m, geometry::translation(-10.0f, 1.339746f, 20.0f));
    mesh.insert(m);
    attach_x(mesh, "box0/top", "hex1/end0");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));


    name = "cap";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, "outer", 20, 1, 15.0f, 15.0f, 0.0f, 30.0f);
    m.clear();
    model::create_cylinder(m, "inner", 20, 1, 10.0f, 10.0f, 5.0f, 30.0f);
    invert(m);
    mesh.insert(m);
    attach(mesh, "outer/end1", "inner/end1");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "stand";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, "outer", 6, 1, 24.0f, 21.0f, 0.0f, 25.0f);
    m.clear();
    model::create_cylinder(m, "inner", 6, 1, 18.0f, 18.0f, 0.0f, 25.0f);
    invert(m);
    mesh.insert(m);
    attach(mesh, "outer/end1", "inner/end1");
    attach(mesh, "outer/end0", "inner/end0");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));


    mesh_type m0;
    mesh_type m1;
    mesh_type m2;

    name = "cubes_3";
    cout << name << endl;
    float offset = 20.0f / 3.0f;
    mesh.clear();
    m0.clear();
    model::create_box(m0, "box0", 20.0f, 20.0f, 20.0f);
    model::transform(m0, geometry::translation(offset, -offset, 0.0f));
    mesh.insert(m0);
    m1.clear();
    model::create_box(m1, "box1", 20.0f, 20.0f, 20.0f);
    model::transform(m1, geometry::translation(0.0f, offset, -offset));
    mesh.insert(m1);
    m2.clear();
    model::create_box(m2, "box2", 20.0f, 20.0f, 20.0f);
    model::transform(m2, geometry::translation(-offset, 0.0f, offset));
    mesh.insert(m2);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cubes_3a1";
    cout << name << endl;
    model::transform(m0, geometry::translation(-1 * offset, -1 * offset, 2 * offset));
    mesh.insert(m0);
    model::transform(m1, geometry::translation(-2 * offset, 1 * offset, 1 * offset));
    mesh.insert(m1);
    model::transform(m2, geometry::translation(-1 * offset, -1 * offset, 2 * offset));
    mesh.insert(m2);
    model::transform(m1, geometry::translation(-1 * offset, -1 * offset, 2 * offset));
    mesh.insert(m1);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cubes_3a2";
    cout << name << endl;
    mesh.clear();
    m.clear();
    model::create_box(m, "box0", 20.0f, 20.0f, 20.0f);

    mesh_type::vector_type di{-2 * offset, 1 * offset, 1 * offset};
    mesh_type::vector_type dj{-1 * offset, -1 * offset, 2 * offset};
    for (size_t i = 0; i < 4; ++i)
    {
        for (size_t j = 0; j < 4; ++j)
        {
            mesh_type mt = m;
            mesh_type::vector_type t = float(i) * di + float(j) * dj;
            model::transform(mt, geometry::translation(t));
            mesh.insert(mt);
        }
    }

    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    using contour_point_type = geometry::Point<math::CartesianTraits<typename mesh_type::value_type, 2>>;

    std::vector<contour_point_type> contour_outer_a = {
        {0, 45.0f},
        {140.0f, 30.0f},
        {170.0f, 45.0f},
        {200.0f, 45.0f},
        {300.0f, 0.0f},
    };

    std::vector<contour_point_type> contour_inner_a = {
        {0, 40.0f},
        {140.0f, 25.0f},
        {170.0f, 40.0f},
        {200.0f, 40.0f},
        {290.0f, 0.0f},
    };

    name = "shroom3a";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, "outer", 3, contour_outer_a);
    m.clear();
    model::create_cylinder(m, "inner", 3, contour_inner_a);
    invert(m);
    mesh.insert(m);
    //    attach(mesh, "outer/end1", "inner/end1");
    attach(mesh, "outer/end0", "inner/end0");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "shroom6a";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, "outer", 6, contour_outer_a, Properties{{"azimuth_smooth", "false"}, {"axix_smooth", "false"}});
    m.clear();
    model::create_cylinder(m, "inner", 6, contour_inner_a);
    invert(m);
    mesh.insert(m);
    attach(mesh, "outer/end0", "inner/end0");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    // glass pattern prototype
    name = "shroom6a_solid";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, "outer", 6, contour_outer_a, Properties{{"azimuth_smooth", "false"}, {"axix_smooth", "false"}});
    model::write_obj(mesh, to_filename(name));

    name = "shroom6a_g0";
    cout << name << endl;
    m = mesh;
    model::transform(m, geometry::scaling(0.5f, 0.5f, 0.5f));
    model::write_obj(m, to_filename(name));

    name = "shroom6a_g1";
    cout << name << endl;
    m = mesh;
    model::transform(m, geometry::scaling(0.6f, 0.6f, 0.5f));
    model::write_obj(m, to_filename(name));

    std::vector<contour_point_type> contour_outer_b = {
        {0, 30.0f},
        {85.0f, 20.0f},
        {105.0f, 30.0f},
        {125.0f, 30.0f},
        {145.0f, 20.0f},
    };

    std::vector<contour_point_type> contour_inner_b = {
        {0, 25.0f},
        {85.0f, 15.0f},
        {105.0f, 25.0f},
        {125.0f, 25.0f},
        {145.0f, 15.0f},
    };

    name = "shroom3b";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, "outer", 3, contour_outer_b);
    m.clear();
    model::create_cylinder(m, "inner", 3, contour_inner_b);
    invert(m);
    mesh.insert(m);
    attach(mesh, "outer/end1", "inner/end1");
    attach(mesh, "outer/end0", "inner/end0");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "shroom6b";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, "outer", 6, contour_outer_b);
    m.clear();
    model::create_cylinder(m, "inner", 6, contour_inner_b);
    invert(m);
    mesh.insert(m);
    attach(mesh, "outer/end1", "inner/end1");
    attach(mesh, "outer/end0", "inner/end0");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    std::vector<contour_point_type> contour_outer_c = {
        {0, 30.0f},
        {100.0f, 20.0f},
        {150.0f, 30.0f},
        {175.0f, 20.0f},
    };

    std::vector<contour_point_type> contour_inner_c = {
        {0, 25.0f},
        {100.0f, 15.0f},
        {150.0f, 25.0f},
        {175.0f, 15.0f},
    };

    name = "shroom3c";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, "outer", 3, contour_outer_c);
    m.clear();
    model::create_cylinder(m, "inner", 3, contour_inner_c);
    invert(m);
    mesh.insert(m);
    attach(mesh, "outer/end1", "inner/end1");
    attach(mesh, "outer/end0", "inner/end0");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "shroom6c";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, "outer", 6, contour_outer_c);
    m.clear();
    model::create_cylinder(m, "inner", 6, contour_inner_c);
    invert(m);
    mesh.insert(m);
    attach(mesh, "outer/end1", "inner/end1");
    attach(mesh, "outer/end0", "inner/end0");
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    validator.report_results();
    return 0;
}
