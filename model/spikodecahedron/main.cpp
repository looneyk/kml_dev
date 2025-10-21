//------------------------------------------------------------------------------
// spikodecahedron
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/mesh_util.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/math/CartesianTraits.hpp"
#include "quetzal/math/Matrix.hpp"
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
    using vector_traits = math::CartesianTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;
    using matrix_type = math::Matrix<value_type>;

    const size_t s_nLevels = 4;

    // Control revs on first (low, 1) and higher levels
    const bool s_bTwistLow = true;
    const bool s_bTwistHigh = true;

    static const string dirOut = "out/";
    static const string dirReference = "reference/";

    //--------------------------------------------------------------------------
    string to_filename(const string& name)
    {
        return dirOut + name + ".obj";
    }

} // namespace

namespace
{

    //--------------------------------------------------------------------------
    void create_spikodecahedron(mesh_type& mesh, const string& name, value_type r0, value_type r1, value_type z1, value_type revsSpikeMin, value_type revsSpikeMax, size_t nzSpikeMax)
    {
        assert(nzSpikeMax > 0);

        mesh_type m;
        model::create_dodecahedron(m, name, r0);

        value_type zBase = 0;

        size_t nFaces = m.faces().size();
        for (size_t n = 0; n < nFaces; ++n)
        {
            mesh_type::vector_type upSpike = {0.0f, 0.0f, 1.0f};
            mesh_type::vector_type upFace = m.face(n).attributes().normal();

            // Angle to rotate face so that it is perpendicular to z-axis
            value_type theta = math::angle_unit(upSpike, upFace);

            size_t nLevel = 0; // Controls detail and rotation
            value_type r = 0; // Length of spike

            if (math::float_eq(theta, math::Pi<value_type>)) // Bottom face
            {
                nLevel = 0;
                r = r0;
            }
            else if (math::float_ne(theta, value_type(0)))
            {
                nLevel = (math::PiHalf<value_type> - theta) > 0 ? 2 : 1;

                value_type theta0 = math::PiHalf<value_type> - asin(upFace.z());
                value_type theta2 = asin(z1 * sin(theta0) / r1);
                value_type theta1 = math::Pi<value_type> - (theta0 + theta2);
                r = r1 * sin(theta1) / sin(theta0);
            }
            else // Top face
            {
                nLevel = s_nLevels - 1;
                r = r1 + z1;
            }

            if (nLevel == 0) // Bottom face, just record base z coordinate
            {
                zBase = m.face(n).halfedge().vertex_attributes().position().z();
                continue;
            }

            if ((!s_bTwistLow && nLevel == 1) || (!s_bTwistHigh && nLevel > 1) || (revsSpikeMin == 0.0f && revsSpikeMax == 0.0f) || nzSpikeMax  == 1) // Replace face with pyramidal projection
            {
                triangulate_central_vertex(m, n, upFace * r, false);
                continue;
            }

            size_t nAzimuth = m.face(n).halfedges().size();
            size_t nz = nzSpikeMax * nLevel / (s_nLevels - 1);
            value_type dzSpike = r - r0;
            value_type revs = revsSpikeMin + (revsSpikeMax - revsSpikeMin) * nLevel / (s_nLevels - 1); // lerp? ...

            string nameSpike = "spike" + to_string(n);
            string nameSpikeBase = nameSpike + "/end0";

            // Body face position and orientation
            mesh_type::point_type position = centroid(m.face(n));
            mesh_type::vector_type vFace = m.face(n).halfedge().vertex_attributes().position() - position;
            mesh_type::vector_type rightFace = normalize(vFace);
            value_type rBase = vFace.norm();

            // Spike and orientation
            mesh_type mSpike;
            model::create_helix_cone(mSpike, nameSpike, nAzimuth, nz, rBase, dzSpike, revs);
            mesh_type::vector_type rightSpike = normalize(mSpike.surface(nameSpikeBase).faces().front().halfedge().vertex_attributes().position());
            assert(math::float_eq0(rightSpike.z()));

            // Rotate spike to align with face normal and translate spike to face position
            matrix_type matrix = alignment(upSpike, rightSpike, upFace, rightFace) * translation(position);
            model::transform(mSpike, matrix);

            // Connect spike to body
            m.insert(mSpike);

//            brep::id_type idHalfedgeA = m.face(n).halfedge_id();
//            brep::id_type idHalfedgeB = m.surface(nameSpikeBase).faces().front().halfedge().prev_id();

//            model::unify_vertex_pairs(m, idHalfedgeA, idHalfedgeB);
//            brep::weld(m, idHalfedgeA, idHalfedgeB);
            brep::weld(m, n, m.surface(nameSpikeBase).faces().front().id()); // surface names? less overhead if we know we have a single face surface ...
        }

        m.pack();

        // Ensure that nothing projects below the bottom face
        for (auto& vertex : m.vertices())
        {
            if (vertex.attributes().position().z() < zBase)
            {
                vertex.attributes().position().set_z(zBase);
            }
        }

        mesh.insert(m);
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

    name = "spiked_dodecahedron.obj";
    cout << name << endl;
    mesh.clear();
    create_spikodecahedron(mesh, "spikodecahedon", 29.0f, 58.0f, 31.1688f, 0.0f, 0.0f, 1);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name);

    name = "twisted_dodecahedron.obj";
    cout << name << endl;
    mesh.clear();
    create_spikodecahedron(mesh, "spikodecahedon", 29.0f, 58.0f, 31.1688f, 0.0f, 0.5f, 100);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name);

    name = "twisted_dodecahedron_ball.obj";
    cout << name << endl;
    mesh.clear();
    create_spikodecahedron(mesh, "spikodecahedon", 30.0f, 36.0f, 0.0f, 0.25f, 0.25f, 100);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name);

    name = "twisted_dodecahedron_cactus.obj";
    cout << name << endl;
    mesh.clear();
    create_spikodecahedron(mesh, "spikodecahedon", 45.0f, 54.0f, 0.0f, 0.25f, 0.25f, 100);
    model::transform(mesh, math::scaling(1.0f, 1.0f, 0.666f));
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name);

    // glass may be better off not twisted at all ...
    name = "twisted_dodecahedron_glass.obj";
    cout << name << endl;
    mesh.clear();
    create_spikodecahedron(mesh, "spikodecahedon", 29.0f, 58.0f, 31.1688f, 0.0f, 0.25f, 3);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name);

    validator.report_results();
    return 0;
}
