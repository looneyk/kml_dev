//------------------------------------------------------------------------------
// spikodesic
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/clip.hpp"
#include "quetzal/brep/connection.hpp"
#include "quetzal/brep/validation.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/geometry/intersect.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/math/Matrix.hpp"
#include "quetzal/model/helix_pyramid.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include "quetzal/model/transforms.hpp"
#include <algorithm>
#include <array>
#include <string>
#include <iostream>
#include <cassert>

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;
    using matrix_type = math::Matrix<value_type>;

    static const mesh_type::vector_type down {0.0f, 0.0f, -1.0f};

    static const string dirOut = "out/";
    static const string dirReference = "reference/";

    //--------------------------------------------------------------------------
    string to_filename(const string& name)
    {
        return dirOut + name + ".obj";
    }

    //--------------------------------------------------------------------------
    bool adjacent_faces(const mesh_type& mesh, id_type idFaceA, id_type idFaceB)
    {
        for (const auto& halfedge : mesh.face(idFaceA).halfedges())
        {
            if (halfedge.partner().face_id() == idFaceB)
            {
                return true;
            }
        }

        return false;
    }

    //--------------------------------------------------------------------------
    id_type select_base_face(const mesh_type& mesh)
    {
        for (const auto& face : mesh.faces())
        {
            assert(face.halfedges().size() == 3);

            auto position0 = face.halfedge().vertex_attributes().position();
            auto position1 = face.halfedge().next().vertex_attributes().position();
            auto position2 = face.halfedge().prev().vertex_attributes().position();

            auto normal = normalize(cross(position1 - position0, position2 - position0));
            assert(math::float_eq(normal, face.attributes().normal()));

            mesh_type::value_type length0 = (position1 - position0).norm();
            mesh_type::value_type length1 = (position2 - position1).norm();
            mesh_type::value_type length2 = (position0 - position2).norm();

            // check that equilateral works for all nSubdivisions ...
            bool bEquilateral = math::float_eq0(length1 - length0) && math::float_eq0(length2 - length1);
            if (bEquilateral)
            {
                return face.id();
            }
        }

        return nullid;
    }

    //--------------------------------------------------------------------------
    id_type orient_vertical(mesh_type& mesh)
    {
        id_type idFace0 = select_base_face(mesh);
        auto face = mesh.face(idFace0);

        matrix_type matrix = geometry::alignment(face.attributes().normal(), down);
        model::transform(mesh, matrix);

        value_type zBase = centroid(face).z();

        for (auto& halfedge : face.halfedges())
        {
            auto position = halfedge.vertex_attributes().position();
            position.set_z(zBase);

// this won't work until select_base_face is completed, but it seems that normals might not be calculated right ...
//            model::unify_vertex_position(halfedge.vertex(), position);
        }

        return idFace0;
    }

    //--------------------------------------------------------------------------
    void create_spikodesic(mesh_type& mesh, const string& name, value_type r0, size_t nSubdivisions, std::function<value_type(value_type)> radius, size_t nzSpikeMax, value_type revsSpikeMax)
    {
        assert(nzSpikeMax > 0);

        mesh_type m;
        model::create_geodesic_sphere(m, name, r0, nSubdivisions, false, false);

        id_type idFace0 = orient_vertical(m);
        value_type zBase = m.face(idFace0).halfedges().front().vertex_attributes().position().z();

        // Base plane
        geometry::Plane<mesh_type::vector_traits> plane({0.0f, 0.0f, zBase}, {0.0f, 0.0f, -1.0f});

        size_t nFaces = m.faces().size();
        for (size_t n = 0; n < nFaces; ++n)
        {
            if (n == idFace0) // Bottom face
            {
                continue;
            }

            auto face = m.face(n);
            mesh_type::vector_type upFace = face.attributes().normal();

            if (adjacent_faces(m, idFace0, n)) // First level, replace face with pyramidal projection to be flush with base
            {
                geometry::Line<mesh_type::vector_traits> line(centroid(face), face.attributes().normal());
                auto inter = intersection(line, plane);
                assert(inter.locus() == geometry::Locus::Point);
                auto p = inter.point();
                assert(math::float_eq(p.z(), zBase));
                p.set_z(zBase);

                // maybe just set nz to 1 here and use clip at end? ...
                // maybe do that for any apex below zBase? ...

                triangulate_central_vertex(m, n, p, false);
                continue;
            }

            mesh_type::point_type position = centroid(face);
            mesh_type::vector_type rightFace = normalize(face.halfedge().vertex_attributes().position() - position);
            mesh_type::vector_type upSpike = {0.0f, 0.0f, 1.0f};
            mesh_type::vector_type rightSpike = {1.0f, 0.0f, 0.0f};

            geometry::Polygon<mesh_type::vector_traits> polygon = to_polygon(face);
            matrix_type matrix = geometry::translation(-position.vector()) * geometry::alignment(upFace, rightFace, upSpike, rightSpike);
            polygon.transform(matrix);

            for (auto& v : polygon.vertices())
            {
                assert(math::float_eq0(v.z()));
                v.set_z(0.0f);
            }

            value_type theta = math::angle_between_unit(down, face.attributes().normal());
            assert(theta >= 0.0f && theta <= math::Pi<value_type>);
            value_type t = theta / math::Pi<value_type>;

            // Spike parameters
            size_t nz = static_cast<size_t>(clamp(floor(nzSpikeMax * t) + 1.0f, 1.0f, value_type(nzSpikeMax)));
            value_type r = radius(t);
            value_type dz = r - r0;
            assert(dz >= 0.0f);
            value_type revs = revsSpikeMax * t;

            string nameSpike = "spike" + to_string(n);
            string nameSpikeBase = nameSpike + "/end0";

            // Spike and orientation
            mesh_type mSpike;
            model::create_helix_cone(mSpike, nameSpike, polygon, nz, dz, revs);

            // Rotate spike to align with face normal and translate spike to face position
//            matrix_type matrix = alignment2(upSpike, rightSpike, upFace, rightFace) * translation(position);
            matrix = geometry::alignment(upSpike, rightSpike, upFace, rightFace) * geometry::translation(position.vector());
            model::transform(mSpike, matrix);

            // this may actually be more efficient, but a single clip at the end should work too ...
            if ((centroid(face) + upFace * dz).z() < zBase)
            {
                brep::clip(mSpike, plane);
            }

            // Connect spike to body
            m.insert(mSpike);

            // Accuracy suffers without this ...
            // But disabling this can be used to test alignment transform accuracy ...
// weld should just do this ...
//            id_type idHalfedgeA = face.halfedge_id();
//            id_type idHalfedgeB = m.surface(nameSpikeBase).faces().front().halfedge().prev_id();
//            model::unify_vertex_pairs(m, idHalfedgeA, idHalfedgeB);

            brep::weld(m, face.surface().extended_name(), nameSpikeBase);
        }

// should work here ...
//        clip(m, plane);

        m.pack();
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

    value_type r0 = 30.0f;
    size_t nSubdivisions = 2;

    auto radius = [&](value_type t) -> value_type
    {
        return r0 * (1.0f + 1.5f * t); // spiral ...
    };

    name = "spikodesic";
    cout << name << endl;
    mesh.clear();
    create_spikodesic(mesh, name, r0, nSubdivisions, radius, 1, 0.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "swirled_spikodesic";
    cout << name << endl;
    mesh.clear();
//    create_spikodesic(mesh, name, r0, nSubdivisions, radius, 100, 0.0f);
create_spikodesic(mesh, name, r0, nSubdivisions, radius, 10, 0.0f);
    model::swirl(mesh, math::Pi<value_type>);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "twisted_spikodesic";
    cout << name << endl;
    mesh.clear();
//    create_spikodesic(mesh, name, r0, nSubdivisions, radius, 100, 0.5f);
create_spikodesic(mesh, name, r0, nSubdivisions, radius, 10, 0.5f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    // Use previous mesh
    name = "swirled_twisted_spikodesic";
    cout << name << endl;
	model::swirl(mesh, math::Pi<value_type>);
    model::write_obj(mesh, to_filename(name));

    validator.report_results();
    return 0;
}
