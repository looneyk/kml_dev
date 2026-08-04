//------------------------------------------------------------------------------
// primitives
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/texcoord.hpp"
#include "quetzal/brep/mesh_util.hpp"
#include "quetzal/brep/normal.hpp"
#include "quetzal/brep/triangulation.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/math/Interval.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include <filesystem>
#include <iostream>
#include <string>
#include <cassert>

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
//    using value_type = double;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;
    using point_type = mesh_type::point_type;
    using vector_type = point_type::vector_type;

    //--------------------------------------------------------------------------
    // Disk morphed into cone
    template<typename M>
    void create_disk_test(M& mesh, const string& name, size_t nAzimuth, size_t nRadial, value_type radius, const math::Interval<value_type>& intervalAzimuth = math::Interval(value_type(0), value_type(1)))
    {
        id_type submesh_id = model::create_disk(mesh, name, nAzimuth, nRadial, radius, intervalAzimuth);

        model::transform(mesh.submesh(submesh_id), [radius](typename M::traits_type::point_type position) -> typename M::traits_type::point_type
        {
            position.set_z(radius - to_vector(position).norm());
            return position;
        });

        // not correct at apex, cannot be calculated the same way there ...
        brep::calculate_surface_vertex_normals(mesh.submesh(submesh_id).surface(0));

        // Bottom
        typename M::vector_type normal = -vector_type::z_direction();
        id_type idSurface = mesh.create_surface(mesh.submesh_id(name), "bottom", {normal});
        create_border_face(mesh, 0, normal, idSurface);
        calculate_surface_texcoords(mesh, idSurface);

        mesh.check();
        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    filesystem::path dir_out = argc > 1 ? argv[1] : "out/";

    auto to_filename = [&dir_out](const string& name)
    {
        return dir_out / (name + ".obj");
    };

    string name;
    mesh_type mesh;
    id_type submesh_id = nullid;
    id_type surface_id = nullid;

    name = "disk_cone";
    cout << name << endl;
    mesh.clear();
    create_disk_test(mesh, name, 12, 4, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "box";
    cout << name << endl;
    mesh.clear();
    model::create_box(mesh, name, 5.0f, 10.0f, 20.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cylinder";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 12, 10, 5.0f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cylinder_1";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 12, 1, 5.0f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cylinder_left_half";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 6, 10, 5.0f, -10.0f, 10.0f, {{"azimuth_lower", "0.25"}, {"azimuth_upper", "0.75"}, {"azimuth_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cylinder_left_half_center";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 6, 10, 5.0f, -10.0f, 10.0f, {{"azimuth_lower", "0.25"}, {"azimuth_upper", "0.75"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cylinder_right_half";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 6, 10, 5.0f, -10.0f, 10.0f, {{"azimuth_lower", "-0.25"}, {"azimuth_upper", "0.25"}, {"azimuth_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cylinder_quarter";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 3, 10, 5.0f, -10.0f, 10.0f, {{"azimuth_lower", "-0.125"}, {"azimuth_upper", "0.125"}, {"azimuth_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cylinder_three_quarter";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 9, 10, 5.0f, -10.0f, 10.0f, {{"azimuth_lower", "0.125"}, {"azimuth_upper", "0.875"}, {"azimuth_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cylinder_half_center";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 6, 10, 5.0f, -10.0f, 10.0f, {{"azimuth_lower", "0.25"}, {"azimuth_upper", "0.75"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cylinder_quarter_center";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 3, 10, 5.0f, -10.0f, 10.0f, {{"azimuth_lower", "-0.125"}, {"azimuth_upper", "0.125"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cylinder_three_quarter_center";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 9, 10, 5.0f, -10.0f, 10.0f, {{"azimuth_lower", "0.125"}, {"azimuth_upper", "0.875"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cylinder_three_quarter_none";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 9, 10, 5.0f, -10.0f, 10.0f, {{"azimuth_lower", "0.125"}, {"azimuth_upper", "0.875"}, {"azimuth_style", "none"}});
    surface_id = mesh.create_surface(mesh.submesh_id(name), "side", {{1.0f, 0.0f, 0.0f}});
    create_border_face(mesh, 3 * 9, {1.0f, 0.0f, 0.0f}, surface_id);
    calculate_surface_texcoords(mesh, surface_id);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "antiprism_3";
    cout << name << endl;
    mesh.clear();
    model::create_antiprism(mesh, name, 3, 7.5f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "antiprism_6";
    cout << name << endl;
    mesh.clear();
    model::create_antiprism(mesh, name, 6, 7.5f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "anticylinder_3_f";
    cout << name << endl;
    mesh.clear();
    model::create_anticylinder(mesh, name, 3, 1, 7.5f, -10.0f, 10.0f, {{"azimuth_smooth", "false"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "anticylinder_3";
    cout << name << endl;
    mesh.clear();
    model::create_anticylinder(mesh, name, 3, 1, 7.5f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "anticylinder_6_f";
    cout << name << endl;
    mesh.clear();
    model::create_anticylinder(mesh, name, 6, 1, 7.5f, -10.0f, 10.0f, {{"azimuth_smooth", "false"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "anticylinder_6";
    cout << name << endl;
    mesh.clear();
    model::create_anticylinder(mesh, name, 6, 1, 7.5f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "anticylinder_3_2_f";
    cout << name << endl;
    mesh.clear();
    model::create_anticylinder(mesh, name, 3, 2, 7.5f, -10.0f, 10.0f, {{"azimuth_smooth", "false"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "anticylinder_3_2";
    cout << name << endl;
    mesh.clear();
    model::create_anticylinder(mesh, name, 3, 2, 7.5f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "anticylinder_6_2_f";
    cout << name << endl;
    mesh.clear();
    model::create_anticylinder(mesh, name, 6, 2, 7.5f, -10.0f, 10.0f, {{"azimuth_smooth", "false"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "anticylinder_6_2";
    cout << name << endl;
    mesh.clear();
    model::create_anticylinder(mesh, name, 6, 2, 7.5f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "anticylinder_6_8_f";
    cout << name << endl;
    mesh.clear();
    model::create_anticylinder(mesh, name, 6, 8, 7.5f, -10.0f, 10.0f, {{"azimuth_smooth", "false"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "anticylinder_6_8";
    cout << name << endl;
    mesh.clear();
    model::create_anticylinder(mesh, name, 6, 8, 7.5f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cone_up";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 12, 6, 7.5f, 0.0f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cone_up_left_half";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 6, 6, 7.5f, 0.0f, -10.0f, 10.0f, {{"azimuth_lower", "0.25"}, {"azimuth_upper", "0.75"}, {"azimuth_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cone_up_right_half";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 6, 6, 7.5f, 0.0f, -10.0f, 10.0f, {{"azimuth_lower", "-0.25"}, {"azimuth_upper", "0.25"}, {"azimuth_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cone_up_quarter";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 3, 6, 7.5f, 0.0f, -10.0f, 10.0f, {{"azimuth_lower", "-0.125"}, {"azimuth_upper", "0.125"}, {"azimuth_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cone_up_three_quarter";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 9, 6, 7.5f, 0.0f, -10.0f, 10.0f, {{"azimuth_lower", "0.125"}, {"azimuth_upper", "0.875"}, {"azimuth_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cone_up_quarter_center";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 3, 6, 7.5f, 0.0f, -10.0f, 10.0f, {{"azimuth_lower", "-0.125"}, {"azimuth_upper", "0.125"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cone_up_three_quarter_center";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 9, 6, 7.5f, 0.0f, -10.0f, 10.0f, {{"azimuth_lower", "0.125"}, {"azimuth_upper", "0.875"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cone_down";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 12, 6, 0.0f, 7.5f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cone_down_left_half";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 6, 6, 0.0f, 7.5f, -10.0f, 10.0f, {{"azimuth_lower", "0.25"}, {"azimuth_upper", "0.75"}, {"azimuth_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cone_down_right_half";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 6, 6, 0.0f, 7.5f, -10.0f, 10.0f, {{"azimuth_lower", "-0.25"}, {"azimuth_upper", "0.25"}, {"azimuth_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cone_down_quarter";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 3, 6, 0.0f, 7.5f, -10.0f, 10.0f, {{"azimuth_lower", "-0.125"}, {"azimuth_upper", "0.125"}, {"azimuth_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cone_down_three_quarter";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 9, 6, 0.0f, 7.5f, -10.0f, 10.0f, {{"azimuth_lower", "0.125"}, {"azimuth_upper", "0.875"}, {"azimuth_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cone_down_quarter_center";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 3, 6, 0.0f, 7.5f, -10.0f, 10.0f, {{"azimuth_lower", "-0.125"}, {"azimuth_upper", "0.125"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cone_down_three_quarter_center";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 9, 6, 0.0f, 7.5f, -10.0f, 10.0f, {{"azimuth_lower", "0.125"}, {"azimuth_upper", "0.875"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cone_up_1";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 6, 1, 7.5f, 0.0f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cone_down_1";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 6, 1, 0.0f, 7.5f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "prism";
    cout << name << endl;
    mesh.clear();
    model::create_prism(mesh, name, 3, 6, 5.0f, 5.0f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "pyramid_up";
    cout << name << endl;
    mesh.clear();
    model::create_prism(mesh, name, 12, 6, 7.5f, 0.0f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "pyramid_up_left_half";
    cout << name << endl;
    mesh.clear();
    model::create_prism(mesh, name, 6, 6, 7.5f, 0.0f, -10.0f, 10.0f, {{"azimuth_lower", "0.25"}, {"azimuth_upper", "0.75"}, {"azimuth_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "pyramid_up_right_half";
    cout << name << endl;
    mesh.clear();
    model::create_prism(mesh, name, 6, 6, 7.5f, 0.0f, -10.0f, 10.0f, {{"azimuth_lower", "-0.25"}, {"azimuth_upper", "0.25"}, {"azimuth_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "pyramid_up_quarter";
    cout << name << endl;
    mesh.clear();
    model::create_prism(mesh, name, 3, 6, 7.5f, 0.0f, -10.0f, 10.0f, {{"azimuth_lower", "-0.125"}, {"azimuth_upper", "0.125"}, {"azimuth_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "pyramid_up_three_quarter";
    cout << name << endl;
    mesh.clear();
    model::create_prism(mesh, name, 9, 6, 7.5f, 0.0f, -10.0f, 10.0f, {{"azimuth_lower", "0.125"}, {"azimuth_upper", "0.875"}, {"azimuth_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "pyramid_up_quarter_center";
    cout << name << endl;
    mesh.clear();
    model::create_prism(mesh, name, 3, 6, 7.5f, 0.0f, -10.0f, 10.0f, {{"azimuth_lower", "-0.125"}, {"azimuth_upper", "0.125"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "pyramid_up_three_quarter_center";
    cout << name << endl;
    mesh.clear();
    model::create_prism(mesh, name, 9, 6, 7.5f, 0.0f, -10.0f, 10.0f, {{"azimuth_lower", "0.125"}, {"azimuth_upper", "0.875"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "pyramid_down";
    cout << name << endl;
    mesh.clear();
    model::create_prism(mesh, name, 12, 6, 0.0f, 7.5f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "pyramid_down_left_half";
    cout << name << endl;
    mesh.clear();
    model::create_prism(mesh, name, 6, 6, 0.0f, 7.5f, -10.0f, 10.0f, {{"azimuth_lower", "0.25"}, {"azimuth_upper", "0.75"}, {"azimuth_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "pyramid_down_right_half";
    cout << name << endl;
    mesh.clear();
    model::create_prism(mesh, name, 6, 6, 0.0f, 7.5f, -10.0f, 10.0f, {{"azimuth_lower", "-0.25"}, {"azimuth_upper", "0.25"}, {"azimuth_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "pyramid_down_quarter";
    cout << name << endl;
    mesh.clear();
    model::create_prism(mesh, name, 3, 6, 0.0f, 7.5f, -10.0f, 10.0f, {{"azimuth_lower", "-0.125"}, {"azimuth_upper", "0.125"}, {"azimuth_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "pyramid_down_three_quarter";
    cout << name << endl;
    mesh.clear();
    model::create_prism(mesh, name, 9, 6, 0.0f, 7.5f, -10.0f, 10.0f, {{"azimuth_lower", "0.125"}, {"azimuth_upper", "0.875"}, {"azimuth_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "pyramid_down_quarter_center";
    cout << name << endl;
    mesh.clear();
    model::create_prism(mesh, name, 3, 6, 0.0f, 7.5f, -10.0f, 10.0f, {{"azimuth_lower", "-0.125"}, {"azimuth_upper", "0.125"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "pyramid_down_three_quarter_center";
    cout << name << endl;
    mesh.clear();
    model::create_prism(mesh, name, 9, 6, 0.0f, 7.5f, -10.0f, 10.0f, {{"azimuth_lower", "0.125"}, {"azimuth_upper", "0.875"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "pyramid_up_1";
    cout << name << endl;
    mesh.clear();
    model::create_prism(mesh, name, 6, 1, 7.5f, 0.0f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "pyramid_down_1";
    cout << name << endl;
    mesh.clear();
    model::create_prism(mesh, name, 6, 1, 0.0f, 7.5f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "sphere";
    cout << name << endl;
    mesh.clear();
    model::create_sphere(mesh, name, 24, 12, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "sphere_rough";
    cout << name << endl;
    mesh.clear();
    model::create_sphere(mesh, name, 12, 6, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "sphere_fine";
    cout << name << endl;
    mesh.clear();
    model::create_sphere(mesh, name, 36, 18, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "sphere_left_half";
    cout << name << endl;
    mesh.clear();
    model::create_sphere(mesh, name, 12, 12, 10.0f, {{"azimuth_lower", "0.25"}, {"azimuth_upper", "0.75"}, {"azimuth_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "sphere_right_half";
    cout << name << endl;
    mesh.clear();
    model::create_sphere(mesh, name, 12, 12, 10.0f, {{"azimuth_lower", "-0.25"}, {"azimuth_upper", "0.25"}, {"azimuth_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "sphere_quarter";
    cout << name << endl;
    mesh.clear();
    model::create_sphere(mesh, name, 6, 12, 10.0f, {{"azimuth_lower", "-0.125"}, {"azimuth_upper", "0.125"}, {"azimuth_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "sphere_three_quarter";
    cout << name << endl;
    mesh.clear();
    model::create_sphere(mesh, name, 18, 12, 10.0f, {{"azimuth_lower", "0.125"}, {"azimuth_upper", "0.875"}, {"azimuth_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "sphere_quarter_center";
    cout << name << endl;
    mesh.clear();
    model::create_sphere(mesh, name, 6, 12, 10.0f, {{"azimuth_lower", "-0.125"}, {"azimuth_upper", "0.125"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "sphere_three_quarter_center";
    cout << name << endl;
    mesh.clear();
    model::create_sphere(mesh, name, 18, 12, 10.0f, {{"azimuth_lower", "0.125"}, {"azimuth_upper", "0.875"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "sphere_top";
    cout << name << endl;
    mesh.clear();
    model::create_sphere(mesh, name, 24, 6, 10.0f, {{"elevation_lower", "0.0"}, {"elevation_upper", "0.25"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "sphere_bottom";
    cout << name << endl;
    mesh.clear();
    model::create_sphere(mesh, name, 24, 6, 10.0f, {{"elevation_lower", "-0.25"}, {"elevation_upper", "0.0"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "sphere_middle";
    cout << name << endl;
    mesh.clear();
    model::create_sphere(mesh, name, 24, 6, 10.0f, {{"elevation_lower", "-0.125"}, {"elevation_upper", "0.125"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "sphere_top_three_quarter_flat";
    cout << name << endl;
    mesh.clear();
    model::create_sphere(mesh, name, 18, 6, 10.0f, {{"azimuth_lower", "0.125"}, {"azimuth_upper", "0.875"}, {"azimuth_style", "butt"}, {"elevation_lower", "0.0"}, {"elevation_upper", "0.25"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "sphere_top_three_quarter_center";
    cout << name << endl;
    mesh.clear();
    model::create_sphere(mesh, name, 18, 6, 10.0f, {{"azimuth_lower", "0.125"}, {"azimuth_upper", "0.875"}, {"elevation_lower", "0.0"}, {"elevation_upper", "0.25"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "sphere_bottom_three_quarter_flat";
    cout << name << endl;
    mesh.clear();
    model::create_sphere(mesh, name, 18, 6, 10.0f, {{"azimuth_lower", "0.125"}, {"azimuth_upper", "0.875"}, {"azimuth_style", "butt"}, {"elevation_lower", "-0.25"}, {"elevation_upper", "0.0"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "sphere_bottom_three_quarter_center";
    cout << name << endl;
    mesh.clear();
    model::create_sphere(mesh, name, 18, 6, 10.0f, {{"azimuth_lower", "0.125"}, {"azimuth_upper", "0.875"}, {"elevation_lower", "-0.25"}, {"elevation_upper", "0.0"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "sphere_middle_three_quarter_flat";
    cout << name << endl;
    mesh.clear();
    model::create_sphere(mesh, name, 18, 6, 10.0f, {{"azimuth_lower", "0.125"}, {"azimuth_upper", "0.875"}, {"azimuth_style", "butt"}, {"elevation_lower", "-0.125"}, {"elevation_upper", "0.125"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "sphere_middle_three_quarter_center";
    cout << name << endl;
    mesh.clear();
    model::create_sphere(mesh, name, 18, 6, 10.0f, {{"azimuth_lower", "0.125"}, {"azimuth_upper", "0.875"}, {"elevation_lower", "-0.125"}, {"elevation_upper", "0.125"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "sphere_middle_three_quarter_none";
    cout << name << endl;
    mesh.clear();
    model::create_sphere(mesh, name, 18, 6, 10.0f, {{"azimuth_lower", "0.125"}, {"azimuth_upper", "0.875"}, {"azimuth_style", "none"}, {"elevation_lower", "-0.125"}, {"elevation_upper", "0.125"}});
    surface_id = mesh.create_surface(mesh.submesh_id(name), "side", {{1.0f, 0.0f, 0.0f}});
    create_border_face(mesh, 3 * 18, {1.0f, 0.0f, 0.0f}, surface_id);
    calculate_surface_texcoords(mesh, surface_id);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "sphere_octahedron";
    cout << name << endl;
    mesh.clear();
    model::create_sphere(mesh, name, 4, 2, 10.0f);
    model::write_obj(mesh, to_filename(name));

    name = "antisphere";
    cout << name << endl;
    mesh.clear();
    model::create_antisphere(mesh, name, 12, 6, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "torus";
    cout << name << endl;
    mesh.clear();
    model::create_torus(mesh, name, 24, 12, 15.0f, 7.5f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "torus_right_half";
    cout << name << endl;
    mesh.clear();
    model::create_torus(mesh, name, 12, 12, 15.0f, 7.5f, {{"major_azimuth_lower", "-0.25"}, {"major_azimuth_upper", "0.25"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "torus_top_half";
    cout << name << endl;
    mesh.clear();
    model::create_torus(mesh, name, 24, 6, 15.0f, 7.5f, {{"minor_azimuth_lower", "0.5"}, {"minor_azimuth_upper", "1.0"}, {"minor_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "torus_bottom_half";
    cout << name << endl;
    mesh.clear();
    model::create_torus(mesh, name, 24, 6, 15.0f, 7.5f, {{"minor_azimuth_lower", "0.0"}, {"minor_azimuth_upper", "0.5"}, {"minor_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "torus_bottom_quarter";
    cout << name << endl;
    mesh.clear();
    model::create_torus(mesh, name, 24, 3, 15.0f, 7.5f, {{"minor_azimuth_lower", "0.125"}, {"minor_azimuth_upper", "0.375"}, {"minor_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "torus_bottom_three_quarter";
    cout << name << endl;
    mesh.clear();
    model::create_torus(mesh, name, 24, 9, 15.0f, 7.5f, {{"minor_azimuth_lower", "-0.125"}, {"minor_azimuth_upper", "0.625"}, {"minor_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "torus_inward_slant";
    cout << name << endl;
    mesh.clear();
    model::create_torus(mesh, name, 24, 6, 15.0f, 7.5f, {{"minor_azimuth_lower", "0.125"}, {"minor_azimuth_upper", "0.625"}, {"minor_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "torus_outward_slant";
    cout << name << endl;
    mesh.clear();
    model::create_torus(mesh, name, 24, 6, 15.0f, 7.5f, {{"minor_azimuth_lower", "-0.125"}, {"minor_azimuth_upper", "0.375"}, {"minor_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "torus_inside_half";
    cout << name << endl;
    mesh.clear();
    model::create_torus(mesh, name, 24, 6, 15.0f, 7.5f, {{"minor_azimuth_lower", "-0.25"}, {"minor_azimuth_upper", "0.25"}, {"minor_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "torus_outside_half";
    cout << name << endl;
    mesh.clear();
    model::create_torus(mesh, name, 24, 6, 15.0f, 7.5f, {{"minor_azimuth_lower", "0.25"}, {"minor_azimuth_upper", "0.75"}, {"minor_style", "butt"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "torus_inside_three_quarter_center";
    cout << name << endl;
    mesh.clear();
    model::create_torus(mesh, name, 24, 9, 15.0f, 7.5f, {{"minor_azimuth_lower", "-0.375"}, {"minor_azimuth_upper", "0.375"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "torus_outside_quarter_center";
    cout << name << endl;
    mesh.clear();
    model::create_torus(mesh, name, 24, 3, 15.0f, 7.5f, {{"minor_azimuth_lower", "0.375"}, {"minor_azimuth_upper", "0.625"}});
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "tetrahedron";
    cout << name << endl;
    mesh.clear();
    model::create_tetrahedron(mesh, name, 10.0f);
    model::write_obj(mesh, to_filename(name));

    name = "octahedron_vertex";
    cout << name << endl;
    mesh.clear();
    model::create_octahedron(mesh, name, 10.0f, true);
    model::write_obj(mesh, to_filename(name));

    name = "octahedron_face";
    cout << name << endl;
    mesh.clear();
    model::create_octahedron(mesh, name, 10.0f, false);
    model::write_obj(mesh, to_filename(name));

    name = "dodecahedron";
    cout << name << endl;
    mesh.clear();
    submesh_id = model::create_dodecahedron(mesh, name, 10.0f);
    triangulate_central_vertices(mesh.submesh(submesh_id));
    model::write_obj(mesh, to_filename(name));

    for (const auto& face : mesh.faces())
    {
        auto normal = normalize(vertex_centroid(face).vector());
        assert(dot(normal, face.attributes().normal()) > 0.0f);
    }

    name = "icosahedron_vertex";
    cout << name << endl;
    mesh.clear();
    model::create_icosahedron(mesh, name, 10.0f);
    model::write_obj(mesh, to_filename(name));

    name = "icosahedron_face";
    cout << name << endl;
    mesh.clear();
    model::create_icosahedron(mesh, name, 10.0f, false, {{"smooth", "false"}});
    model::write_obj(mesh, to_filename(name));

    name = "icosahedron_vertex_spherical";
    cout << name << endl;
    mesh.clear();
    model::create_icosahedron(mesh, name, 10.0f, true, {{"smooth", "true"}});
    model::write_obj(mesh, to_filename(name));

    name = "icosahedron_face_spherical";
    cout << name << endl;
    mesh.clear();
    model::create_icosahedron(mesh, name, 10.0f, false, {{"smooth", "true"}});
    model::write_obj(mesh, to_filename(name));

    name = "d20";
    cout << name << endl;
    mesh.clear();
    model::create_icosahedron(mesh, name, 12.0f, false, {{"smooth", "false"}});
    model::write_obj(mesh, to_filename(name));

    name = "geodesic_sphere_1";
    cout << name << endl;
    mesh.clear();
    model::create_geodesic_sphere(mesh, name, 10.0f, 1, false);
    model::write_obj(mesh, to_filename(name));

    name = "geodesic_sphere_2";
    cout << name << endl;
    mesh.clear();
    model::create_geodesic_sphere(mesh, name, 10.0f, 2, false);
    model::write_obj(mesh, to_filename(name));

    name = "geodesic_sphere_3";
    cout << name << endl;
    mesh.clear();
    model::create_geodesic_sphere(mesh, name, 10.0f, 3, false);
    model::write_obj(mesh, to_filename(name));

    name = "geodesic_sphere_4";
    cout << name << endl;
    mesh.clear();
    model::create_geodesic_sphere(mesh, name, 10.0f, 4, false);
    model::write_obj(mesh, to_filename(name));

    name = "geodesic_sphere_5";
    cout << name << endl;
    mesh.clear();
    model::create_geodesic_sphere(mesh, name, 10.0f, 5, false);
    model::write_obj(mesh, to_filename(name));

    name = "geodesic_sphere_6";
    cout << name << endl;
    mesh.clear();
    model::create_geodesic_sphere(mesh, name, 10.0f, 6, false);
    model::write_obj(mesh, to_filename(name));

    name = "geodesic_sphere_6_spherical";
    cout << name << endl;
    mesh.clear();
    model::create_geodesic_sphere(mesh, name, 10.0f, 6, true, {{"smooth", "true"}});
    model::write_obj(mesh, to_filename(name));

    name = "cylinder_new";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 12, 10, 5.0f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cone_up_new";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 12, 6, 7.5f, 0.0f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cone_down_new";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 12, 6, 0.0f, 7.5f, -10.0f, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    return 0;
}
