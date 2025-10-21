//------------------------------------------------------------------------------
// clip_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/mesh_util.hpp"
#include "quetzal/brep/triangulation.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/geometry/Plane.hpp"
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

    static const string dirOut = "out/";
    static const string dirReference = "reference/";

    //--------------------------------------------------------------------------
    string to_filename(const string& name)
    {
        return dirOut + name + ".obj";
    }

    //--------------------------------------------------------------------------
    void create_cube(mesh_type& mesh, const string& name, float side)
    {
        brep::id_type idSubmesh = mesh.create_submesh(name);

        float dx2 = 0.5f * side;
        float dy2 = 0.5f * side;
        float dz2 = 0.5f * side;

        vertices_attributes_type> avs =
        {
            {{-dx2, -dy2, -dz2}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
            {{+dx2, -dy2, -dz2}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
            {{+dx2, +dy2, -dz2}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
            {{-dx2, +dy2, -dz2}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
            {{-dx2, -dy2, +dz2}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
            {{+dx2, -dy2, +dz2}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
            {{+dx2, +dy2, +dz2}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
            {{-dx2, +dy2, +dz2}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        };

//v0 hs[0], hs[4], hs[17]
//v1 hs[3], hs[5], hs[8]
//v2 hs[2], hs[9], hs[12]
//v3 hs[1], hs[13], hs[16]
//v4 hs[7], hs[18], hs[20]
//v5 hs[6], hs[11], hs[21]
//v6 hs[10], hs[15], hs[22]
//v7 hs[14], hs[19], hs[23]

        vector<mesh_type::vertex_type>& vertices = mesh.vertices();
        vertices.emplace_back(mesh, 0, 0, avs[0]);
        vertices.emplace_back(mesh, 1, 3, avs[1]);
        vertices.emplace_back(mesh, 2, 2, avs[2]);
        vertices.emplace_back(mesh, 3, 1, avs[3]);
        vertices.emplace_back(mesh, 4, 7, avs[4]);
        vertices.emplace_back(mesh, 5, 6, avs[5]);
        vertices.emplace_back(mesh, 6, 10, avs[6]);
        vertices.emplace_back(mesh, 7, 14, avs[7]);

        for (size_t i = 0; i < 6; ++i)
        {
            brep::id_type idSurface = mesh.create_surface(idSubmesh, "side_" + to_string(i));
            mesh.create_face(idSurface, i * 4);
        }

        vector<mesh_type::halfedge_type>& halfedges = mesh.halfedges();
        halfedges.emplace_back(mesh, 0, 16, 1, 3, 0, 0);
        halfedges.emplace_back(mesh, 1, 12, 2, 0, 3, 0);
        halfedges.emplace_back(mesh, 2, 8, 3, 1, 2, 0);
        halfedges.emplace_back(mesh, 3, 4, 0, 2, 1, 0);
        halfedges.emplace_back(mesh, 4, 3, 5, 7, 0, 1);
        halfedges.emplace_back(mesh, 5, 11, 6, 4, 1, 1);
        halfedges.emplace_back(mesh, 6, 20, 7, 5, 5, 1);
        halfedges.emplace_back(mesh, 7, 17, 4, 6, 4, 1);
        halfedges.emplace_back(mesh, 8, 2, 9, 11, 1, 2);
        halfedges.emplace_back(mesh, 9, 15, 10, 8, 2, 2);
        halfedges.emplace_back(mesh, 10, 21, 11, 9, 6, 2);
        halfedges.emplace_back(mesh, 11, 5, 8, 10, 5, 2);
        halfedges.emplace_back(mesh, 12, 1, 13, 15, 2, 3);
        halfedges.emplace_back(mesh, 13, 19, 14, 12, 3, 3);
        halfedges.emplace_back(mesh, 14, 22, 15, 13, 7, 3);
        halfedges.emplace_back(mesh, 15, 9, 12, 14, 6, 3);
        halfedges.emplace_back(mesh, 16, 0, 17, 19, 3, 4);
        halfedges.emplace_back(mesh, 17, 7, 18, 16, 0, 4);
        halfedges.emplace_back(mesh, 18, 23, 19, 17, 4, 4);
        halfedges.emplace_back(mesh, 19, 13, 16, 18, 7, 4);
        halfedges.emplace_back(mesh, 20, 6, 21, 23, 4, 5);
        halfedges.emplace_back(mesh, 21, 10, 22, 20, 5, 5);
        halfedges.emplace_back(mesh, 22, 14, 23, 21, 6, 5);
        halfedges.emplace_back(mesh, 23, 18, 20, 22, 7, 5);

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
    mesh_type::point_type point;
    mesh_type::vector_type direction;

/*
    name = "cube";
    cout << name << endl;
    mesh.clear();
    create_cube(mesh, name, 10.0);
    cout << "\n" << name << " (quad)" << endl;
    brep::triangulate(mesh);
    cout << "\n" << name << "" << " (triangulated)" << endl;
    model::write_obj(mesh, to_filename(name));
*/

    name = "cube";
    cout << name << endl;
    mesh.clear();
    model::create_box(mesh, name, 10.0, 10.0, 10.0);
    brep::triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    // clip face
    point = {3.33f, 0.0f, 0.0f};
    direction = {1.0f, 0.0f, 0.0f};

    name = "cube_clip_face";
    cout << name << endl;
    mesh.clear();
	model::create_box(mesh, name, 10.0, 10.0, 10.0);
    clip(mesh, geometry::Plane(point, normalize(direction)), "slice");
    brep::triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "triangulated_cube_clip_face";
    cout << name << endl;
    mesh.clear();
	model::create_box(mesh, name, 10.0, 10.0, 10.0);
    brep::triangulate(mesh);
    clip(mesh, geometry::Plane(point, normalize(direction)), "slice");
    brep::triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    // clip edge
    point = {3.33f, 3.33f, 0.0f};
    direction = {1.0f, 1.0f, 0.0f};

    name = "cube_clip_edge";
    cout << name << endl;
    mesh.clear();
	model::create_box(mesh, name, 10.0, 10.0, 10.0);
    clip(mesh, geometry::Plane(point, normalize(direction)), "slice");
    brep::triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "triangulated_cube_clip_edge";
    cout << name << endl;
    mesh.clear();
	model::create_box(mesh, name, 10.0, 10.0, 10.0);
    brep::triangulate(mesh);
    clip(mesh, geometry::Plane(point, normalize(direction)), "slice");
    brep::triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    // clip vertex
    point = {3.33f, 3.33f, 3.33f};
    direction = {1.0f, 1.0f, 1.0f};

    name = "cube_clip_vertex";
    cout << name << endl;
    mesh.clear();
	model::create_box(mesh, name, 10.0, 10.0, 10.0);
    clip(mesh, geometry::Plane(point, normalize(direction)), "slice");
    brep::triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "triangulated_cube_clip_vertex";
    cout << name << endl;
    mesh.clear();
	model::create_box(mesh, name, 10.0, 10.0, 10.0);
    brep::triangulate(mesh);
    clip(mesh, geometry::Plane(point, normalize(direction)), "slice");
    brep::triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    // Edge cases, clip plane tangent to mesh

    // clip face tangent
    point = {10.0f, 0.0f, 0.0f};
    direction = {1.0f, 0.0f, 0.0f};

    name = "cube_clip_face_tangent";
    cout << name << endl;
    mesh.clear();
	model::create_box(mesh, name, 10.0, 10.0, 10.0);
    clip(mesh, geometry::Plane(point, normalize(direction)), "slice");
    brep::triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    // clip edge tangent
    point = {10.0f, 10.0f, 0.0f};
    direction = {1.0f, 1.0f, 0.0f};

    name = "cube_clip_edge_tangent";
    cout << name << endl;
    mesh.clear();
	model::create_box(mesh, name, 10.0, 10.0, 10.0);
    clip(mesh, geometry::Plane(point, normalize(direction)), "slice");
    brep::triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    // clip vertex tangent
    point = {10.0f, 10.0f, 10.0f};
    direction = {1.0f, 1.0f, 1.0f};

    name = "cube_clip_vertex_tangent";
    cout << name << endl;
    mesh.clear();
	model::create_box(mesh, name, 10.0, 10.0, 10.0);
    clip(mesh, geometry::Plane(point, normalize(direction)), "slice");
    brep::triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    mesh_type m;

    // octahedron the hard way
    name = "octahedron";
    cout << name << endl;
    mesh.clear();
	model::create_octahedron(m, name, 10.0);

	model::create_box(mesh, name, 30.0, 30.0, 30.0);

    for (const auto& face : m.faces())
    {
        point = centroid(face);
        direction = face.attributes().normal();
        assert(float_eq(normalize(point), direction));

        clip(mesh, geometry::Plane(point, direction), face.surface().name());
    }

    brep::triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    // icosahedron the hard way
    name = "icosahedron";
    cout << name << endl;
    mesh.clear();
    m.clear();
	model::create_icosahedron(m, name, 10.0);

	model::create_box(mesh, name, 30.0, 30.0, 30.0);

    for (const auto& face : m.faces())
    {
        point = centroid(face);
        direction = face.attributes().normal();
        assert(float_eq(normalize(point), direction));

        clip(mesh, geometry::Plane(point, direction), face.surface().name());
    }

    brep::triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    // pyramid sliced
    point = {0.0f, 0.0f, 0.0f};
    direction = {0.0f, 0.0f, -1.0f};

    name = "pyramid_sliced";
    cout << name << endl;
    mesh.clear();
    model::create_cylinder(mesh, name, 4, 1, 10.0f, 0.0f, 0.0f, 10.0f);
    clip(mesh, geometry::Plane(point, direction), "slice");
    brep::triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    // octahedron sliced
    point = {0.0f, 0.0f, 0.0f};
    direction = {0.0f, 0.0f, -1.0f};

    name = "octahedron_sliced";
    cout << name << endl;
    mesh.clear();
	model::create_octahedron(mesh, name, 10.0f);
    clip(mesh, geometry::Plane(point, direction), "slice");
    brep::triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    validator.report_results();
    return 0;
}
