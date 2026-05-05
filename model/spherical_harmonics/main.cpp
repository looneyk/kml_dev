//------------------------------------------------------------------------------
// spherical_harmonics
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/triangulation.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/math/math_util.hpp"
#include "quetzal/math/spherical_harmonics.hpp"
#include "quetzal/model/mesh_attributes.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include <format>
#include <iostream>
#include <string>
#include <cassert>

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;

    static const filesystem::path dir_out = "out/";
    static const filesystem::path dir_ref = "reference/";
    static const string app_name = "spherical_harmonics";

    static const int degree_max = 10;

    //--------------------------------------------------------------------------
    filesystem::path to_filename(const string& name)
    {
        return dir_out / (name + ".obj");
    }

    //--------------------------------------------------------------------------
    template<typename T>
    T elevation_to_polar(T elevation)
    {
        return T(0.5) - elevation;
    }

    //--------------------------------------------------------------------------
    template<typename M>
    id_type create_spherical_harmonic1(M& mesh, const std::string& name, size_t nAzimuth, size_t nElevation, value_type scale, std::function<double(double)> fr)
    {
        id_type submesh_id = model::create_sphere(mesh, name, nAzimuth, nElevation, [fr](value_type te) -> value_type { return static_cast<value_type>(abs(fr(elevation_to_polar(double(te)) * math::Pi<double>))); });
        model::transform(mesh.submesh(submesh_id), geometry::scaling(scale));
        return submesh_id;
    }

    //--------------------------------------------------------------------------
    template<typename M>
    id_type create_spherical_harmonic2(M& mesh, const std::string& name, size_t nAzimuth, size_t nElevation, value_type scale, std::function<double(double, double)> fr)
    {
        id_type submesh_id = model::create_sphere(mesh, name, nAzimuth, nElevation, [fr](value_type ta, value_type te) -> value_type { return static_cast<value_type>(abs(fr(elevation_to_polar(double(te)) * math::Pi<double>, double(ta) * math::PiTwo<double>))); }); // squared instead of abs? ...
        model::transform(mesh.submesh(submesh_id), geometry::scaling(scale));
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

    // Create all spherical harmonics using create_sphere with a 2 parameter radius function

    for (int l = 0; l <= degree_max; ++l)
    {
//        for (int m = -l; m <= l; ++m)
int m = 0;
        {
            name = std::format("spherical_harmonic_{}_{}", l, m);
            cout << name << endl;
            mesh.clear();
            submesh_id = create_spherical_harmonic2(mesh, name, 24, 100, 10.0f, math::real_spherical_harmonic_function(l, m));
            triangulate(mesh.submesh(submesh_id));
            model::write_obj(mesh, to_filename(name));
        }
    }

    // Create spherical harmonics with m = 0 using create_sphere with a 1 parameter radius function

    for (int l = 0; l <= 3; ++l)
    {
        name = std::format("spherical_harmonic_{}_0_", l);
        cout << name << endl;
        mesh.clear();
        submesh_id = create_spherical_harmonic1(mesh, name, 24, 100, 10.0f, [l] (double theta) { return math::real_spherical_harmonic_function(l, 0)(theta, 0.0); });
        triangulate(mesh.submesh(submesh_id));
        model::write_obj(mesh, to_filename(name));
    }

    validator.report_results();
    return 0;
}
