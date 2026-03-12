//------------------------------------------------------------------------------
// spherical_harmonics
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/mesh_util.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/model/mesh_attributes.hpp"
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
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;

    static const filesystem::path dir_out = "out/";
    static const filesystem::path dir_ref = "reference/";
    static const string app_name = "spherical_harmonics";

    //--------------------------------------------------------------------------
    filesystem::path to_filename(const string& name)
    {
        return dir_out / (name + ".obj");
    }

    //--------------------------------------------------------------------------
    // degree l, order m
    std::function<value_type(value_type)> spherical_harmonic_function(int degree, int order)
    {
        assert(abs(order) <= degree);

        switch (degree)
        {
            case 0:
                return [](value_type) -> value_type { return 0.5f * sqrt(1.0f / math::Pi<value_type>); };

            case 1:
                if (order == 0)
                {
                    return [](value_type t) -> value_type { return 0.5f * sqrt(3.0f / math::Pi<value_type>) * abs(cos(math::Pi<value_type> * t)); };
                }
                // -1, 1 ...
                break;

            case 2:
                if (order == 0)
                {
                    return [](value_type t) -> value_type { value_type c = cos(math::Pi<value_type> * t); return 0.25f * sqrt(5.0f / math::Pi<value_type>) * abs(3.0f * c * c - 1.0f); };
                }
                // -2, -1, 1, 2 ...
                break;

            case 3:
                if (order == 0)
                {
                    return [](value_type t) -> value_type { value_type c = cos(math::Pi<value_type> * t); return 0.25f * sqrt(7.0f / math::Pi<value_type>) * abs(5.0f * c * c * c - 3.0f * c); };
                }
                // -3, -2, -1, 1, 2, 3 ...
                break;
        }

        assert(false);
        return [](value_type) -> value_type { return 0.0f; };
    }

    //--------------------------------------------------------------------------
    template<typename M>
    id_type create_spherical_harmonic(M& mesh, const std::string& name, size_t nAzimuth, size_t nElevation, value_type scale, std::function<value_type(value_type)> fr)
    {
        assert(nAzimuth > 2);

        id_type submesh_id = model::create_sphere(mesh, name, nAzimuth, nElevation, [fr](value_type t) -> value_type { return fr(1.0f - t); });
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

    name = "spherical_harmonic_0_0";
    cout << name << endl;
    mesh.clear();
    submesh_id = create_spherical_harmonic(mesh, name, 24, 100, 10.0f, spherical_harmonic_function(0, 0));
    triangulate(mesh.submesh(submesh_id));
    model::write_obj(mesh, to_filename(name));

    name = "spherical_harmonic_1_0";
    cout << name << endl;
    mesh.clear();
    submesh_id = create_spherical_harmonic(mesh, name, 24, 100, 10.0f, spherical_harmonic_function(1, 0));
    triangulate(mesh.submesh(submesh_id));
    model::write_obj(mesh, to_filename(name));

    name = "spherical_harmonic_2_0";
    cout << name << endl;
    mesh.clear();
    submesh_id = create_spherical_harmonic(mesh, name, 24, 100, 10.0f, spherical_harmonic_function(2, 0));
    triangulate(mesh.submesh(submesh_id));
    model::write_obj(mesh, to_filename(name));

    name = "spherical_harmonic_3_0";
    cout << name << endl;
    mesh.clear();
    submesh_id = create_spherical_harmonic(mesh, name, 24, 100, 10.0f, spherical_harmonic_function(3, 0));
    triangulate(mesh.submesh(submesh_id));
    model::write_obj(mesh, to_filename(name));

    validator.report_results();
    return 0;
}
