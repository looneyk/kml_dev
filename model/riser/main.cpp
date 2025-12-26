//------------------------------------------------------------------------------
// suburbmesh
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/connection.hpp"
#include "quetzal/brep/connection2.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/geometry/Polygon.hpp"
#include "quetzal/geometry/PolygonWithHoles.hpp"
#include "quetzal/math/CartesianTraits.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include "quetzal/model/stl_io.hpp"
#include <iostream>
#include <string>
#include <cassert>

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
    using vector_traits = math::CartesianTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;

    static const string dirIn = "in/";
    static const string dirOut = "out/";
    static const string dirReference = "reference/";

    //--------------------------------------------------------------------------
    string to_filename_obj_out(const string& name)
    {
        return dirOut + name + ".obj";
    }

    //--------------------------------------------------------------------------
    string to_filename_stl_in(const string& name)
    {
        return dirIn + name + ".stl";
    }

    //--------------------------------------------------------------------------
    string to_filename_stl_out(const string& name)
    {
        return dirOut + name + ".stl";
    }

    //--------------------------------------------------------------------------
    void create_riser(mesh_type& mesh, const std::string& name, int mode)
    {
        value_type x0 = 0.0f;
        value_type x1 = 3.8f;
//        value_type x2 = 12.0f;
        value_type x2 = 9.25f;
        value_type x3 = 32.0f;
        value_type x4 = 37.45f;
        value_type x5 = 41.25f;

        value_type y0 = 0.0f;
        value_type y1 = 3.8f;
        value_type y2 = 19.8f;
        value_type y3 = 35.8f;
        value_type y4 = 39.6f;
        
        value_type z0 = 0.0f;
        value_type z1 = 42.0f;

        geometry::Polygon<vector_traits> outer = {
            {x1, y0, z0},
            {x4, y0, z0},
            {x5, y1, z0},
            {x5, y3, z0},
            {x4, y4, z0},
            {x1, y4, z0},
            {x0, y3, z0},
            {x0, y1, z0},
        };

        geometry::Polygon<vector_traits> inner;
        value_type r = 3.02f;
        int n = 54;
        mesh_type::vector_type d0 = {x2, y2, z0};
        mesh_type::vector_type d1 = {x3, y2, z0};

        for (int i = 0; i < n; ++i)
        {
            value_type phi = math::PiTwo<value_type> * i / n;
            inner.vertices().push_back({r * cos(phi), -r * sin(phi), z0}); // reverse winding order for hole
        }

        int n2 = n / 2;
        int n4 = n / 4;
        for (int i = 0; i <= n4; ++i)
        {
            inner.vertex(i) += d1;
            if (i > 0) inner.vertex(n2 - i) += d0;
            inner.vertex(n2 + i) += d0;
            if (i > 0) inner.vertex(n - i) += d1;
        }

        geometry::PolygonWithHoles<vector_traits> polygon;
        polygon.set_polygon(outer);
        polygon.holes().push_back(inner);

        if (mode == 0)
        {
            model::create_cylinder(mesh, name, 1, z0, z1, polygon);
        }
        else if (mode == 1)
        {
            model::create_extrusion(mesh, name, polygon, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, z1 - z0});
        }

        mesh_type m;
        model::create_box(m, "a", 6.0f, 6.5f, 6.5f);
        model::transform_positions(m, math::translation(x0 + 3.05f, y2, z1 + 3.25f));
        mesh.append(m);
        attach(mesh, name + "/end1", "a/bottom");

        m.clear();
        model::create_box(m, "b", 6.0f, 6.5f, 6.5f);
        model::transform_positions(m, math::translation(x5 - 3.05f, y2, z1 + 3.25f));
        mesh.append(m);
        attach(mesh, name + "/end1", "b/bottom");

//validate(mesh);
        triangulate(mesh);
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

    name = "Samyang_135_riser";
    cout << name << endl;
    mesh.clear();
    cout << "read_stl" << endl;
    model::read_stl(mesh, to_filename_stl_in(name));

    cout << "write_obj" << endl;
    model::write_obj(mesh, to_filename_obj_out(name));

    name = "riser_cylinder";
    cout << name << endl;
    mesh.clear();
    create_riser(mesh, name, 0);
    model::write_obj(mesh, to_filename_obj_out(name));

    name = "riser_extrusion";
    cout << name << endl;
    mesh.clear();
    create_riser(mesh, name, 1);
    model::write_obj(mesh, to_filename_obj_out(name));

    validator.report_results();
    return 0;
}
