//------------------------------------------------------------------------------
// urchin
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/mesh_util.hpp"
#include "quetzal/math/Matrix.hpp"
#include "quetzal/math/math_util_dx.hpp" // ...
#include "quetzal/model/helix_pyramid.hpp"
#include "quetzal/model/mesh_attributes.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include "quetzal/model/transforms.hpp"
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

    using matrix_type = math::Matrix<value_type>;

    //--------------------------------------------------------------------------
    void create_urchin(mesh_type& mesh, value_type r0, value_type r1, value_type z1, size_t nSubdivisions, size_t nAzimuthSpike, size_t nzSpikeMax, value_type twistSpikeMax, value_type tuskification = 0.0f)
    {
        assert(nAzimuthSpike > 0);
        assert(nzSpikeMax > 0);

        z1; // ...

        mesh_type m;
        model::create_geodesic_sphere(m, r0, nSubdivisions, false);
//        model::calculate_face_normals(m); // already done with smooth false ...

        value_type zBase = 0;

        size_t nFaces = m.faces().size();
        for (auto n = 0; n < nFaces; ++n)
        {
            // Body face position and orientation
            mesh_type::vector3_type position = centroid(m.face(n));
            mesh_type::vector3_type normal = geometry::normal(m.face(n));

            // Angle to rotate face so that it is perpendicular to z-axis
            value_type theta = math::angle_normal({0.0f, 0.0f, 1.0f}, normal);

            size_t nLevel = 0; // Controls detail and rotation
            value_type r = 0; // Length of spike

            if (math::float_eq(theta, math::Pi<value_type>)) // Bottom face
            {
                nLevel = 0;
                r = r0;
            }
            else if (theta > math::Pi<value_type> * 5.0f / 6.0f) // Unmodified flat face
            {
                nLevel = 1;
                r = r0;
            }
            else if (theta > math::PiTwo<value_type> / 3.0f) // Pyramidal spike
            {
                nLevel = 2;
                r = std::max(r0 * sin(math::Pi<value_type> - 3.0f * theta), math::norm(centroid(m.face(n))); // r0 to half of level 3 min height ...
            }
            else if (theta > math::Pi<value_type> / 3.0f)
            {
                nLevel = 3;
                r = r0 + (r1 - r0) * sin(math::Pi<value_type> - 1.5f * theta);
            }
            else // Top face
            {
                nLevel = 3;
                r = r1;
            }

            if (nLevel == 0) // Keep face without modification
            {
                zBase = m.face(n).halfedge().value().position().z();
                continue;
            }
            else if (nLevel == 1) // Keep face without modification
            {
                continue;
            }
            else if (nLevel == 2) // Replace face with pyramidal projection
            {
                triangulate_central_vertex(m, n, normal * r, false);
            }
            else // Append twisted spike
            {
                // Create spike

                size_t nz = static_cast<size_t>(nzSpikeMax * (1.0f - theta / math::Pi<value_type>)) + 1; // ...
                if (nz > nzSpikeMax) nz = nzSpikeMax;

                mesh_type::vector3_type vFace = m.face(n).halfedge().value().position() - position;
                value_type rBase = vFace.norm();

                // Reduce spike base radius so that it fits inside geodesic face
                if (nAzimuthSpike == 3)
                {
                    rBase *= 0.5f;
                }
                else
                {
                    rBase *= 0.4f;
                }

                value_type dzSpike = r - r0;
                value_type twist = twistSpikeMax * (1.0f - theta / math::Pi<value_type>);

                mesh_type mSpike;
                model::create_helix_cone(mSpike, nAzimuthSpike, nz, rBase, dzSpike, twist);

                // Base face is the last one
                mesh_type::vector3_type vBase = mSpike.faces().back().halfedge().value().position();
                assert(math::float_eq0(vBase.z()));

                if (tuskification != 0 && theta != 0)
                {
                    mesh_type::vector3_type axisRotationBase;
                    auto vFaceOriented = vFace;

                    if (theta != 0)
                    {
                        axisRotationBase = math::normalize(math::cross({0.0f, 0.0f, 1.0f}, normal)); // axis to rotate face perpendicular to z-axis
                        vFaceOriented *= math::rotation_normal(axisRotationBase, -theta); // Rotate face parallel to xy-plane
                    }

                    value_type phi = atan2(vFaceOriented.y(), vFaceOriented.x()) + atan2(vBase.y(), vBase.x());

                    model::tuskify(mSpike, axisRotationBase * math::rotation_z(-phi), tuskification, 0.0f, dzSpike);
                }

                // Rotate spike to align with normal and translate spike to face position
                matrix_type matrix = math::matrix_alignment({0.0f, 0.0f, 1.0f}, vBase, normal, vFace);
                model::transform(mSpike, matrix * math::translation(position), matrix);

                // Connect spike to body
                m.insert(mSpike);
                brep::attach(m, n, m.faces().back().id()); // get face id's from named surfaces ...
            }
        }

        triangulate(m);
        m.pack();

        // Flatten bottom faces and ensure that nothing projects below the bottom
        zBase = r0 * cos(math::Pi<value_type> * 3.0f / 4.0f);

        for (auto& vertex : m.vertices())
        {
            if (vertex.value().position().z() < zBase)
            {
                vertex.value().position().set_z(zBase);
            }
        }

        assert(check_normal_consistency(m));

        check_unit_normals(m);

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

    // Make sure that z1 is large enough that lower spikes don't extend below base

    name = "urchin_3.obj";
    cout << name << endl;
    mesh.clear();
    create_urchin(mesh, 29.0f, 58.0f, 31.1688f, 3, 3, 1, 0.0f); // Not swirled, not twisted, single section spike
    triangulate(mesh);
    write_obj(mesh, dirOut + name);

    // Smooth combinations
    // 25 -> 200 ...

    name = "smooth_urchin.obj";
    cout << name << endl;
    mesh.clear();
    create_urchin(mesh, 29.0f, 58.0f, 31.1688f, 3, 3, 25, 0.0f); // Smooth spike
    triangulate(mesh);
    write_obj(mesh, dirOut + name);

    name = "swirled_urchin.obj";
    cout << name << endl;
    mesh.clear();
    create_urchin(mesh, 29.0f, 58.0f, 31.1688f, 3, 3, 25, 0.0f); // Swirled, smooth spike
    triangulate(mesh);
    model::swirl(mesh, math::Pi<value_type>);
    write_obj(mesh, dirOut + name);

    name = "twisted_urchin.obj";
    cout << name << endl;
    mesh.clear();
    create_urchin(mesh, 29.0f, 58.0f, 31.1688f, 3, 3, 25, 0.5f); // Twisted, smooth spike
    triangulate(mesh);
    write_obj(mesh, dirOut + name);

    name = "twisted_urchin_2.obj";
    cout << name << endl;
    mesh.clear();
    create_urchin(mesh, 29.0f, 80.0f, 31.1688f, 3, 3, 25, 0.5f); // Twisted, smooth spike
    triangulate(mesh);
    write_obj(mesh, dirOut + name);

    // Use previous mesh
    name = "swirled_twisted_urchin.obj";
    cout << name << endl;
    model::swirl(mesh, math::Pi<value_type>); // Swirled, twisted, smooth spike
    write_obj(mesh, dirOut + name);

    // hexagonal spikes ...
    name = "urchin_6.obj";
    cout << name << endl;
    mesh.clear();
    create_urchin(mesh, 29.0f, 58.0f, 31.1688f, 3, 6, 1, 0.5f); // Twisted, smooth spike
    triangulate(mesh);
    write_obj(mesh, dirOut + name);

    // hexagonal spikes ...
    name = "twisted_urchin_6.obj";
    cout << name << endl;
    mesh.clear();
    create_urchin(mesh, 29.0f, 58.0f, 31.1688f, 3, 6, 20, 0.5f); // Twisted, smooth spike
    triangulate(mesh);
    write_obj(mesh, dirOut + name);
/*
    name = "twisted_urchin_6_2.obj";
    cout << name << endl;
    mesh.clear();
    create_urchin(mesh, 29.0f, 80.0f, 31.1688f, 3, 6, 200, 0.5f); // Twisted, smooth spike
    triangulate(mesh);
    write_obj(mesh, dirOut + name);
*/
/*
    name = "tuskified_urchin.obj";
    cout << name << endl;
    mesh.clear();
    create_urchin(mesh, 29.0f, 58.0f, 31.1688f, 3, 200, 0.0f, math::PiHalf<value_type<M>>); // Tuskified, smooth spike
    triangulate(mesh);
    write_obj(mesh, dirOut + name);

    // Use previous mesh
    name = "tuskified_swirled_urchin.obj";
    cout << name << endl;
    model::swirl(mesh, math::Pi<value_type<M>>); // Tuskified, swirled, smooth spike
    write_obj(mesh, name);

    name = "tuskified_twisted_urchin.obj";
    cout << name << endl;
    mesh.clear();
    create_urchin(mesh, 29.0f, 58.0f, 31.1688f, 3, 200, 0.5f, math::PiHalf<value_type<M>>); // Tuskified, twisted, smooth spike
    triangulate(mesh);
    write_obj(mesh, dirOut + name);

    // Use previous mesh
    name = "tuskified_swirled_twisted_urchin.obj";
    cout << name << endl;
    model::swirl(mesh, math::Pi<value_type<M>>); // Tuskified, swirled, twisted, smooth spike
    write_obj(mesh, name);
*/

    return 0;
}
