//------------------------------------------------------------------------------
// trinket_box
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/connection.hpp"
#include "quetzal/brep/geometry.hpp"
#include "quetzal/brep/triangulation.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include "quetzal/model/helix_pyramid.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include <iostream>
#include <string>
#include <cassert>

//#include "quetzal/brep/validation.hpp"

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
//    using value_type = double;
// rotation_axis_unit_xm ...
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;
    using matrix_type = math::Matrix<value_type>;

    static const filesystem::path dir_out = "out/";
    static const filesystem::path dir_ref = "reference/";
    static const string app_name = "trinket_box";

    //--------------------------------------------------------------------------
    filesystem::path to_filename(const string& name)
    {
        return dir_out / (name + ".obj");
    }

    //--------------------------------------------------------------------------
    value_type dodecahedron_insphere_radius(value_type r)
    {
        using T = value_type;
        return r * T(2) * sqrt((T(25) + T(11) * sqrt(T(5))) / T(10)) / (sqrt(T(3)) * (T(1) + sqrt(T(5))));
    }

    //--------------------------------------------------------------------------
    void create_spikodecahedron(mesh_type& mesh, const string& name, value_type r0, value_type r1, value_type revsSpike, size_t nzSpike)
    {
        assert(nzSpike > 0);

        mesh_type m;
        model::create_dodecahedron(m, name, r0);

        value_type dzSpike = r1 - dodecahedron_insphere_radius(r0); // Length of spike
        value_type zBase = m.face(0).halfedge().vertex_attributes().position().z();

        size_t nFaces = m.faces().size();
        for (size_t n = 1; n < nFaces - 1; ++n)
        {
            size_t nAzimuth = m.face(n).halfedges().size();

            string nameSpike = "spike" + to_string(n);
            string nameSpikeBase = nameSpike + "/end0";

            // Body face position and orientation
            mesh_type::point_type position = centroid(m.face(n));
            mesh_type::vector_type upFace = m.face(n).attributes().normal();
            mesh_type::vector_type vFace = m.face(n).halfedge().vertex_attributes().position() - position;
            mesh_type::vector_type rightFace = normalize(vFace);
            value_type rBase = vFace.norm();

            // Spike and orientation
            mesh_type mSpike;
            model::create_helix_pyramid(mSpike, nameSpike, nAzimuth, nzSpike, rBase, dzSpike, revsSpike);
            mesh_type::vector_type upSpike = {0.0f, 0.0f, 1.0f};
            mesh_type::vector_type rightSpike = normalize(to_vector(mSpike.surface(nameSpikeBase).faces().front().halfedge().vertex_attributes().position()));
            assert(math::float_eq0(rightSpike.z()));

            // Rotate spike to align with face normal and translate spike to face position
            matrix_type matrix = geometry::alignment(upSpike, rightSpike, upFace, rightFace) * geometry::translation(to_vector(position));
            model::transform(mSpike, matrix);

            // Connect spike to body
            m.insert(mSpike);

            brep::weld(m, n, m.surface(nameSpikeBase).faces().front().id()); // surface names ...
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

    //--------------------------------------------------------------------------
    void unify_vertices(mesh_type& mesh)
    {
        size_t nVertexInequalities = 0;

        mesh.reset();
        for (auto& vertex : mesh.vertices())
        {
            if (!vertex.marked())
            {
                bool bUnequal = false;
                auto position = vertex.attributes() .position();
                for (auto& halfedge : vertex.halfedges())
                {
                    if (!epsilon_eq(position, halfedge.vertex_attributes().position()))
                    {
                        cout << "vertices " << vertex.id() << " " << halfedge.vertex_id() << " failed precision check." << endl;
                    }

                    if (position != halfedge.vertex_attributes().position())
                    {
                        bUnequal = true;
                    }

                    halfedge.vertex_attributes().set_position(position);
                    halfedge.vertex().set_marked();
                }

                if (bUnequal)
                {
                    ++nVertexInequalities;
                }
            }
        }

        cout << nVertexInequalities << " vertex inequalities." << endl;

        return;
    }

    //--------------------------------------------------------------------------
    void check_vertices(mesh_type& mesh)
    {
        size_t nVertexInequalities = 0;
        size_t nTotalInequalities = 0;
        mesh.reset();
        for (auto& vertex : mesh.vertices())
        {
            if (!vertex.marked())
            {
                bool bUnequal = false;
                auto position = vertex.attributes() .position();
                for (auto& halfedge : vertex.halfedges())
                {
                    if (position != halfedge.vertex_attributes().position())
                    {
                        ++nTotalInequalities;
                        bUnequal = true;
                    }

                    halfedge.vertex().set_marked();
                }

                if (bUnequal)
                {
                    ++nVertexInequalities;
                }
            }
        }

        cout << nVertexInequalities << " vertex inequalities." << endl;
        cout << nTotalInequalities << " total inequalities." << endl;

        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    value_type r0 = 1.0f;
    value_type r1 = 1.1f;
    size_t nzSpike = 300; // Resolution
    value_type dzSpike = r1 - dodecahedron_insphere_radius(r0); // Length of spike
    value_type revsSpike = 0.25f;
//nzSpike = 1;
//revsSpike = 0.0f;

    FileValidator validator(dir_out, dir_ref, app_name);

    string name;
    mesh_type mesh;

    name = "trinket_box_open";
    cout << name << endl;
    mesh.clear();

    // Outer surface
    mesh_type m;
    create_spikodecahedron(m, "outer", 1.0f, 1.1f, revsSpike, nzSpike);
    auto polygonUpperOrig = to_polygon(m.surface("outer/body11").faces().front());
    polygonUpperOrig.sections() = std::vector<id_type>();
    model::transform(m, geometry::scaling(50.0f, 50.0f, 25.0f));
    auto polygonUpper = to_polygon(m.surface("outer/body11").faces().front());
    polygonUpper.sections() = std::vector<id_type>();
    mesh.insert(m);

    // Inner surface
    m.clear();
    model::create_dodecahedron(m, "inner", 1.0f);
    model::transform(m, geometry::scaling(45.0f, 45.0f, 20.0f));
    auto polygonLower = to_polygon(m.surface("inner/body11").faces().front());
    polygonLower.sections() = std::vector<id_type>();
    invert(m);
    mesh.insert(m);

    // Neck
    m.clear();
    model::create_cylinder_edges(m, "neck", polygonLower, polygonUpper, 3);

    size_t n = polygonLower.vertex_count();
    size_t nBodyVertices = 3 * n * 4;
    value_type z = (m.vertex(0).attributes().position().z() + m.vertex(nBodyVertices - n).attributes().position().z()) * value_type(0.5);
    for (size_t i = 0; i < n; ++i)
    {
        size_t j = 4 * n + i;

        typename mesh_type::point_type pointLower = m.vertex(i).attributes().position();
        pointLower.set_z(z);

        for (auto& halfedge : m.vertex(j).halfedges())
        {
            halfedge.vertex_attributes().set_position(pointLower);
        }

        size_t k = nBodyVertices - n + i;
        size_t l = nBodyVertices - 4 * n + i;

        typename mesh_type::point_type pointUpper = m.vertex(k).attributes().position();
        pointUpper.set_z(z);

        for (auto& halfedge : m.vertex(l).halfedges())
        {
            halfedge.vertex_attributes().set_position(pointUpper);
        }
    }

    invert(m);
    mesh.insert(m);

    // Connect outer and inner surfaces with neck
    weld(mesh, mesh.surface("outer/body11").faces().front().id(), mesh.surface("neck/end1").faces().front().id());
    weld(mesh, mesh.surface("inner/body11").faces().front().id(), mesh.surface("neck/end0").faces().front().id());

    triangulate(mesh);
//unify_vertices(mesh);
//check_vertices(mesh);
    model::write_obj(mesh, to_filename(name));
//validate(mesh);
//dump(mesh);

    name = "trinket_box_cover";
    cout << name << endl;
    mesh.clear();

    m.clear();
    model::create_helix_pyramid(m, "external", polygonUpperOrig, nzSpike, dzSpike, revsSpike);
    model::transform(m, geometry::scaling(50.0f, 50.0f, 25.0f));
    mesh.insert(m);

    m.clear();
    value_type dz = polygonUpper.vertex(0).z() - z;
//    model::create_cylinder(m, "internal", polygonUpper, 1, -dz, value_type(0.0), {{"azimuth_smooth", "false"}});
for (auto& v : polygonUpper.vertices())
{
    v.set_z(value_type(0));
}
polygonLower = polygonUpper;
for (auto& p : polygonLower.vertices())
{
    p *= value_type(0.995);
    p.set_z(-dz);
}

    model::create_cylinder_edges(m, "internal", polygonLower, polygonUpper, 1);
    mesh.insert(m);

    weld(mesh, mesh.surface("external/end0").faces().front().id(), mesh.surface("internal/end1").faces().front().id());
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));
//validate(mesh);
//check_surface_vertex_consistency_exact(mesh);

    validator.report_results();
    return 0;
}
