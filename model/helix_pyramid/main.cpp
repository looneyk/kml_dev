//------------------------------------------------------------------------------
// helix_pyramid
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/mesh_util.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/geometry/Polygon.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/model/helix_pyramid.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include <string>
#include <iostream>
#include <cassert>

#include "quetzal/brep/validation.hpp"

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;

    static const filesystem::path dir_out = "out/";
    static const filesystem::path dir_ref = "reference/";
    static const string app_name = "helix_pyramid";

    //--------------------------------------------------------------------------
    filesystem::path to_filename(const string& name)
    {
        return dir_out / (name + ".obj");
    }

    //--------------------------------------------------------------------------
    template<typename S>
    bool body_surface(const S& surface)
    {
        return surface.name().starts_with("body");
    }

    //--------------------------------------------------------------------------
    template<typename M>
    void check_consistency(M& mesh)
    {
        mesh;
//        check_surface_vertex_consistency_exact(mesh);
//        triangulate(mesh);
//        check_surface_vertex_consistency_exact(mesh);
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
/*
    name = "helix_cone_3_1";
    cout << name << endl;
    mesh.clear();
    model::create_helix_cone(mesh, name, 3, 1, 20.0f, 60.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "helix_cone_3_2";
    cout << name << endl;
    mesh.clear();
    model::create_helix_cone(mesh, name, 3, 2, 20.0f, 60.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "helix_cone_3_3";
    cout << name << endl;
    mesh.clear();
    model::create_helix_cone(mesh, name, 3, 3, 20.0f, 60.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "helix_cone_3_4";
    cout << name << endl;
    mesh.clear();
    model::create_helix_cone(mesh, name, 3, 4, 15.0f, 60.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "helix_cone_3_100";
    cout << name << endl;
    mesh.clear();
    model::create_helix_cone(mesh, name, 3, 100, 20.0f, 60.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "helix_cone_5_1";
    cout << name << endl;
    mesh.clear();
    model::create_helix_cone(mesh, name, 5, 1, 20.0f, 60.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "helix_cone_5_2";
    cout << name << endl;
    mesh.clear();
    model::create_helix_cone(mesh, name, 5, 2, 20.0f, 60.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "helix_cone_5_3";
    cout << name << endl;
    mesh.clear();
    model::create_helix_cone(mesh, name, 5, 3, 20.0f, 60.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "helix_cone_5_100";
    cout << name << endl;
    mesh.clear();
    model::create_helix_cone(mesh, name, 5, 100, 20.0f, 60.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "helix_cone_6_4";
    cout << name << endl;
    mesh.clear();
    model::create_helix_cone(mesh, name, 6, 4, 20.0f, 60.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "helix_cone_3_200_3";
    cout << name << endl;
    mesh.clear();
    model::create_helix_cone(mesh, name, 3, 200, 20.0f, 60.0f, 3.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);
*/
    name = "helix_pyramid_3_1";
    cout << name << endl;
    mesh.clear();
    model::create_helix_pyramid(mesh, name, 3, 1, 20.0f, 60.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "helix_pyramid_3_2";
    cout << name << endl;
    mesh.clear();
    model::create_helix_pyramid(mesh, name, 3, 2, 20.0f, 60.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "helix_pyramid_3_3";
    cout << name << endl;
    mesh.clear();
    model::create_helix_pyramid(mesh, name, 3, 3, 20.0f, 60.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "helix_pyramid_3_4";
    cout << name << endl;
    mesh.clear();
    model::create_helix_pyramid(mesh, name, 3, 4, 15.0f, 60.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "helix_pyramid_3_10";
    cout << name << endl;
    mesh.clear();
    model::create_helix_pyramid(mesh, name, 3, 10, 15.0f, 60.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "helix_pyramid_3_100";
    cout << name << endl;
    mesh.clear();
    model::create_helix_pyramid(mesh, name, 3, 100, 20.0f, 60.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "helix_pyramid_5_1";
    cout << name << endl;
    mesh.clear();
    model::create_helix_pyramid(mesh, name, 5, 1, 20.0f, 60.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "helix_pyramid_5_2";
    cout << name << endl;
    mesh.clear();
    model::create_helix_pyramid(mesh, name, 5, 2, 20.0f, 60.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "helix_pyramid_5_3";
    cout << name << endl;
    mesh.clear();
    model::create_helix_pyramid(mesh, name, 5, 3, 20.0f, 60.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "helix_pyramid_5_10";
    cout << name << endl;
    mesh.clear();
    model::create_helix_pyramid(mesh, name, 5, 10, 20.0f, 60.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "helix_pyramid_5_100";
    cout << name << endl;
    mesh.clear();
    model::create_helix_pyramid(mesh, name, 5, 100, 20.0f, 60.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "helix_pyramid_6_4";
    cout << name << endl;
    mesh.clear();
    model::create_helix_pyramid(mesh, name, 6, 4, 20.0f, 60.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "helix_pyramid_3_200_3";
    cout << name << endl;
    mesh.clear();
    model::create_helix_pyramid(mesh, name, 3, 200, 20.0f, 60.0f, 3.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_3_1";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_edges(mesh, name, 3, 1, 20.0f, 0.0f, 0.0f, 60.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_3_2";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_edges(mesh, name, 3, 2, 20.0f, 0.0f, 0.0f, 60.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_3_3";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_edges(mesh, name, 3, 3, 20.0f, 0.0f, 0.0f, 60.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_3_4";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_edges(mesh, name, 3, 4, 15.0f, 0.0f, 0.0f, 60.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_3_100";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_edges(mesh, name, 3, 100, 20.0f, 0.0f, 0.0f, 60.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_5_1";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_edges(mesh, name, 5, 1, 20.0f, 0.0f, 0.0f, 60.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_5_2";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_edges(mesh, name, 5, 2, 20.0f, 0.0f, 0.0f, 60.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_5_3";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_edges(mesh, name, 5, 3, 20.0f, 0.0f, 0.0f, 60.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_5_100";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_edges(mesh, name, 5, 100, 20.0f, 0.0f, 0.0f, 60.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_6_4";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_edges(mesh, name, 6, 4, 20.0f, 0.0f, 0.0f, 60.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_3_200_3";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_edges(mesh, name, 3, 200, 20.0f, 0.0f, 0.0f, 60.0f, 0.0f, 3.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    auto polygon_3_20 = geometry::regular_polygon<vector_traits>(3, 20.0f);
    auto polygon_3_15 = geometry::regular_polygon<vector_traits>(3, 15.0f);
    auto polygon_5_20 = geometry::regular_polygon<vector_traits>(5, 20.0f);
    auto polygon_6_20 = geometry::regular_polygon<vector_traits>(6, 20.0f);

    name = "cylinder_edges_polygon_3_1";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_edges(mesh, name, polygon_3_20, 1, 0.0f, 60.0f, 1.0f, 0.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_edges_polygon_3_2";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_edges(mesh, name, polygon_3_20, 2, 0.0f, 60.0f, 1.0f, 0.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_edges_polygon_3_3";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_edges(mesh, name, polygon_3_20, 3, 0.0f, 60.0f, 1.0f, 0.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_edges_polygon_3_4";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_edges(mesh, name, polygon_3_15, 4, 0.0f, 60.0f, 1.0f, 0.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_edges_polygon_3_100";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_edges(mesh, name, polygon_3_20, 100, 0.0f, 60.0f, 1.0f, 0.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_edges_polygon_5_1";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_edges(mesh, name, polygon_5_20, 1, 0.0f, 60.0f, 1.0f, 0.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_edges_polygon_5_2";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_edges(mesh, name, polygon_5_20, 2, 0.0f, 60.0f, 1.0f, 0.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_edges_polygon_5_3";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_edges(mesh, name, polygon_5_20, 3, 0.0f, 60.0f, 1.0f, 0.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_edges_polygon_5_100";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_edges(mesh, name, polygon_5_20, 100, 0.0f, 60.0f, 1.0f, 0.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_edges_polygon_6_4";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_edges(mesh, name, polygon_6_20, 4, 0.0f, 60.0f, 1.0f, 0.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_edges_polygon_3_200_3";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_edges(mesh, name, polygon_3_20, 200, 0.0f, 60.0f, 1.0f, 0.0f, 0.0f, 3.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    std::vector<size_t> sections3{0, 1, 2};
    std::vector<size_t> sections5{0, 1, 2, 3, 4};
    std::vector<size_t> sections6{0, 1, 2, 3, 4, 5};
    polygon_3_20.sections() = sections3;
    polygon_3_15.sections() = sections3;
    polygon_5_20.sections() = sections5;
    polygon_6_20.sections() = sections6;

    name = "cylinder_sections_polygon_3_1";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_sections(mesh, name, polygon_3_20, 1, 0.0f, 60.0f, 1.0f, 0.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_sections_polygon_3_2";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_sections(mesh, name, polygon_3_20, 2, 0.0f, 60.0f, 1.0f, 0.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_sections_polygon_3_3";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_sections(mesh, name, polygon_3_20, 3, 0.0f, 60.0f, 1.0f, 0.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_sections_polygon_3_4";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_sections(mesh, name, polygon_3_15, 4, 0.0f, 60.0f, 1.0f, 0.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_sections_polygon_3_100";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_sections(mesh, name, polygon_3_20, 100, 0.0f, 60.0f, 1.0f, 0.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_sections_polygon_5_1";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_sections(mesh, name, polygon_5_20, 1, 0.0f, 60.0f, 1.0f, 0.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_sections_polygon_5_2";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_sections(mesh, name, polygon_5_20, 2, 0.0f, 60.0f, 1.0f, 0.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_sections_polygon_5_3";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_sections(mesh, name, polygon_5_20, 3, 0.0f, 60.0f, 1.0f, 0.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_sections_polygon_5_100";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_sections(mesh, name, polygon_5_20, 100, 0.0f, 60.0f, 1.0f, 0.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_sections_polygon_6_4";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_sections(mesh, name, polygon_6_20, 4, 0.0f, 60.0f, 1.0f, 0.0f, 0.0f, 0.25f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    name = "cylinder_sections_polygon_3_200_3";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder_sections(mesh, name, polygon_3_20, 200, 0.0f, 60.0f, 1.0f, 0.0f, 0.0f, 3.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
    check_consistency(mesh);

    validator.report_results();
    return 0;
}
