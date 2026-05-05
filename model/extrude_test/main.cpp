//------------------------------------------------------------------------------
// extrude_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/triangulation.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/geometry/Polygon.hpp"
#include "quetzal/geometry/PolygonWithHoles.hpp"
#include "quetzal/math/CartesianTraits.hpp"
#include "quetzal/math/transformation_matrix.hpp"
#include "quetzal/model/geometry.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include <iostream>
#include <string>
#include <cassert>

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
//    using value_type = double;
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

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    FileValidator validator(dirOut, dirReference);

    string name;
    mesh_type mesh;

    geometry::Polygon<vector_traits> polygon = model::create_circle<vector_traits>(20.0f, 12);
    for (size_t i = 1; i < 12; i += 2)
    {
        polygon.vertex(i) /= value_type(2);
    }

    name = "star6_cylinder";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 1, 0.0f, 40.0f, polygon);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "star6_extrusion";
    cout << name << endl;
    mesh.clear();
    model::create_extrusion(mesh, name, polygon, {0.0f, 0.0f, 40.0f});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    geometry::PolygonWithHoles<vector_traits> polygon_holes;
    polygon_holes.polygon() = model::create_square<vector_traits>(40.0f);

    polygon = model::create_circle<vector_traits>(7.0f, 4);
    geometry::Polygon<vector_traits> hole = polygon;
    model::translate(hole, {-9.0f, -9.0f, 0.0f});
    polygon_holes.holes().push_back(hole);

    hole = polygon;
    model::translate(hole, {9.0f, -9.0f, 0.0f});
    polygon_holes.holes().push_back(hole);

    hole = polygon;
    model::translate(hole, {9.0f, 9.0f, 0.0f});
    polygon_holes.holes().push_back(hole);

    hole = polygon;
    model::translate(hole, {-9.0f, 9.0f, 0.0f});
    polygon_holes.holes().push_back(hole);

    name = "holes_cylinder";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 1, 0.0f, 40.0f, polygon_holes);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "holes_extrusion";
    cout << name << endl;
    mesh.clear();
    model::create_extrusion(mesh, name, polygon_holes, {0.0f, 0.0f, 40.0f});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    validator.report_results();
    return 0;
}
