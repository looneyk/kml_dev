//------------------------------------------------------------------------------
// crystal_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/mesh_util.hpp"
#include "quetzal/geometry/Properties.hpp"
#include "quetzal/math/math_util_dx.hpp" // ...
#include "quetzal/math/random.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include <string>
#include <iostream>
#include <cassert>

using namespace std;
using namespace quetzal;

namespace
{

	using value_type = float;
	using mesh_type = brep::Mesh<brep::MeshTraits<value_type>>;

	const std::string dirOut = "out/";

    math::Random<float> g_random;

    //--------------------------------------------------------------------------
    // slope = dz / dr
    template <typename M>
    void create_crystal(M& mesh, size_t nAzimuth, typename M::value_type radius, typename M::value_type dz, typename M::value_type slope)
    {
        model::create_prism(mesh, nAzimuth, 1, radius, radius, -dz / 2.0f, dz / 2.0f);

        auto matrix = math::translation(0.0f, 0.0f, dz / 2.0f);
        for (auto& v : mesh.vertices())
        {
            v.value().position() *= matrix;
        }

        typename M::vector3_type point = {0.0f, 0.0f, dz};

        size_t n = nAzimuth;

        for (size_t i = 0; i < n; ++i)
        {
            float o = g_random.gen(-0.25f, 0.25f);
            point.set_z(dz - 5.0f + g_random.uniform(-5.0f, 5.0f));

            typename M::value_type phi = math::PiTwo<float> * i / n + math::PiTwo<float> / 12.0f + math::PiTwo<float> * o / n;
            typename M::vector3_type direction = {slope * cos(phi), slope * sin(phi), 1.0f};

            brep::clip(mesh, geometry::Plane(point, math::normalize(direction)));
        }

        return;
    }

    //--------------------------------------------------------------------------
    // slope = dz / dr
    template <typename M>
    void create_crystal_mass(M& mesh, size_t nAzimuth, typename M::value_type radius, typename M::value_type dz, typename M::value_type slope)
    {
        std::array<typename M::value_type, 3> dzRadial{dz, 0.8f * dz, 0.5f * dz};
        std::array<typename M::value_type, 3> dthetaMax{0.2f, 0.4f, 0.6f};

        M m;

        for (size_t i = 0; i < 3; ++i)
        {
            size_t n = (i == 0 ? 1 : 6 * i);
            for (size_t j = 0; j < n; ++j)
            {
                m.clear();
                typename M::value_type dzCrystal = g_random.uniform(0.7f, 1.2f) * dzRadial[i];
                typename M::value_type rCrystal = g_random.uniform(0.8f, 1.2f) * radius;
                create_crystal(m, nAzimuth, rCrystal, dzCrystal, slope);

                typename M::vector3_type axis = math::normalize<float, 3>({g_random.uniform(-0.2f, 0.2f), g_random.uniform(0.8f, 1.2f), g_random.uniform(-0.2f, 0.2f)});
                typename M::value_type theta = g_random.uniform(0.0f, dthetaMax[i]);
                typename M::vector3_type position = {g_random.uniform(1.0f, 1.25f) * radius * i, g_random.uniform(-0.1f * radius, 0.1f * radius), g_random.uniform(-0.1f * radius, 0.1f * radius)};
                typename M::value_type phi = g_random.uniform(0.9f, 1.1f) * math::PiTwo<float> * j / n;

                auto matrixRotationY = math::rotation_axis(axis, theta);
                auto matrixTranslation = math::translation(position);
                auto matrixRotationZ = math::rotation_z3(phi);

                model::transform(m, matrixRotationY * matrixTranslation * matrixRotationZ);
                brep::clip(m, geometry::Plane<float>({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}));

                mesh.insert(m);
            }
        }

        m.clear();
        model::create_geodesic_sphere(m, 5.0f * radius, 5);
        brep::clip(m, geometry::Plane<float>({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}));
		model::transform(m, math::scaling(1.0f, 1.0f, 0.1f));
        triangulate(m);
        mesh.insert(m);

        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    string name;
    mesh_type mesh;

    name = "crystal";
    cout << "\n" << name << endl;
    mesh.clear();
    create_crystal(mesh, 6, 20.0f, 150.0f, 1.732f);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name + ".obj");

    name = "crystal_mass";
    cout << "\n" << name << endl;
    mesh.clear();
    create_crystal_mass(mesh, 6, 20.0f, 150.0f, 1.732f);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name + ".obj");

    return 0;
}
