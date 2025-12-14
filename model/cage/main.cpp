//------------------------------------------------------------------------------
// cage
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshBuilder.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/connection.hpp"
#include "quetzal/brep/mesh_util.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/common/string_util.hpp"
#include "quetzal/geometry/spatial_properties.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/model/SurfaceName.hpp"
#include "quetzal/model/helix_pyramid.hpp"
#include "quetzal/model/mesh_attributes.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include "quetzal/model/transforms.hpp"
#include "quetzal/model/vertices_attributes.hpp"
#include <iostream>

using namespace std;
using namespace quetzal;
using namespace DirectX;

namespace
{

	using value_type = float;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;
    using point_type = mesh_type::point_type;
    using vector_type = point_type::vector_type;

    static const filesystem::path dir_out = "out/";
    static const filesystem::path dir_ref = "reference/";
    static const string app_name = "cage";

    //--------------------------------------------------------------------------
    filesystem::path to_filename(const string& name)
    {
        return dir_out / (name + ".obj");
    }

    //--------------------------------------------------------------------------
    std::string surface_name(const std::string& base, size_t n)
    {
        return std::format("{}{}", base, n);
    }

    //--------------------------------------------------------------------------
    std::string next_surface_name(mesh_type& mesh, const std::string& base_name = model::SurfaceName::BodySection)
    {
        return surface_name(base_name, mesh.surface_store_count());
    }

    //--------------------------------------------------------------------------
    id_type create_base(mesh_type& mesh, const std::string& name, value_type r0, value_type r1)
    {
        value_type h0 = r0 * sqrt(3.0f) / 2.0f;
        value_type h1 = r1 + h0 / 3.0f;
        value_type z0 = sqrt(h1 * h1 - h0 * h0);
        value_type z1 = z0 * h0 / h1;
        value_type z2 = h0 * sqrt(3.0f) / 2.0f; // Arbitrary factor
        value_type r2 = h0 * (h1 - h0) / h1;
        value_type h2 = r2 * sqrt(3.0f) / 2.0f;

        std::vector<point_type> positions =
        {
            {h0, r0 / 2.0f, 0.0f},
            {0.0f, r0, 0.0f},
            {-h0, r0 / 2.0f, 0.0f},
            {-h0, -r0 / 2.0f, 0.0f},
            {0.0f, -r0, 0.0f},
            {h0, -r0 / 2.0f, 0.0f},
            {r2, 0.0f, z1},
            {r2 / 2.0f, h2, z1},
            {-r2 / 2.0f, h2, z1},
            {-r2, 0.0f, z1},
            {-r2 / 2.0f, -h2, z1},
            {r2 / 2.0f, -h2, z1},
            {0.0f, 0.0f, z2},
        };

        std::vector<point_type> face_positions =
        {
            // faces
            positions[0],
            positions[6],
            positions[5],

            positions[1],
            positions[7],
            positions[0],

            positions[2],
            positions[8],
            positions[1],

            positions[3],
            positions[9],
            positions[2],

            positions[4],
            positions[10],
            positions[3],

            positions[5],
            positions[11],
            positions[4],

            // sides
            positions[0],
            positions[7],
            positions[12],

            positions[12],
            positions[6],
            positions[0],

            positions[1],
            positions[8],
            positions[12],

            positions[12],
            positions[7],
            positions[1],

            positions[2],
            positions[9],
            positions[12],

            positions[12],
            positions[8],
            positions[2],

            positions[3],
            positions[10],
            positions[12],

            positions[12],
            positions[9],
            positions[3],

            positions[4],
            positions[11],
            positions[12],

            positions[12],
            positions[10],
            positions[4],

            positions[5],
            positions[6],
            positions[12],

            positions[12],
            positions[11],
            positions[5],

            // base
            positions[5],
            positions[4],
            positions[3],
            positions[2],
            positions[1],
            positions[0],
        };

        struct FaceDefinition
        {
            id_type start;
            size_t count;
        };

        std::vector<FaceDefinition> face_definitions =
        {
            {0, 3},
            {3, 3},
            {6, 3},
            {9, 3},
            {12, 3},
            {15, 3},
            {18, 3},
            {21, 3},
            {24, 3},
            {27, 3},
            {30, 3},
            {33, 3},
            {36, 3},
            {39, 3},
            {42, 3},
            {45, 3},
            {48, 3},
            {51, 3},
            {54, 6},
        };

        brep::MeshBuilder<mesh_type> builder;

        builder.set_mesh(mesh);
        id_type submesh_id = builder.submesh(name);

        for (const auto& face_def : face_definitions)
        {
            std::span<const point_type> points{&face_positions[face_def.start], face_def.count};
            builder.surface_face(submesh_id, next_surface_name(mesh), geometry::normal(points), points);
        }

        return submesh_id;
    }

    //--------------------------------------------------------------------------
    id_type create_cage(mesh_type& mesh, const std::string& name, value_type r0, value_type r1, unsigned int nBars, unsigned int nz, value_type twist, value_type tuskification, bool bAdjustTuskification = true)
    {
        value_type h0 = r0 * sqrt(3.0f) / 2.0f;
        value_type h1 = r1 + h0 / 3.0f;
        value_type z0 = sqrt(h1 * h1 - h0 * h0);

        id_type submesh_id = create_base(mesh, name, r0, r1);

        if (tuskification != 0.0f && bAdjustTuskification)
        {
            tuskification -= atan2(h0, z0);
        }

        for (size_t i = 0; i < nBars; ++i)
        {
            size_t nAzimuth = mesh.face(i).halfedge_count();
            point_type center = geometry::centroid(mesh.face(i));
            vector_type normal = geometry::normal(mesh.face(i)); // upFace

            // Axis and angle between face normal and positive z-axis
            vector_type axis = normalize(cross({0.0f, 0.0f, 1.0f}, normal));
            value_type theta = math::PiHalf<value_type> - asin(normal.z());
            assert(theta >= 0.0f);

            // Spike base radius
            vector_type rightFace = mesh.face(i).halfedge().vertex_attributes().position() - center;
            value_type rBase = rightFace.norm();

            // Spike length
            value_type dzSpike = 2.0f * r1 - r0; // ...

            // Create spike
            mesh_type mSpike;
            std::string spike_name = surface_name(model::SurfaceName::BodySection, i);
            model::create_helix_pyramid(mSpike, spike_name, nAzimuth, nz, rBase, dzSpike, twist);

            // Rotate spike to align with normal

            rightFace *= geometry::rotation_axis_unit(axis, -theta); // Face right vector rotated using same matrix that would rotate face parallel to xy-plane
            assert(math::float_eq0(rightFace.z()));
            id_type idSpikeBase = mSpike.halfedge_count() - nAzimuth;
            point_type rightBase = mSpike.halfedge(idSpikeBase).vertex_attributes().position();
            value_type phi = atan2(rightFace.y(), rightFace.x()) + atan2(rightBase.y(), rightBase.x());

            auto matrixDirection = geometry::rotation_axis_unit(vector_type::z_direction(), phi); // Rotate base around z-axis to align with face
            matrixDirection *= geometry::rotation_axis_unit(axis, theta); // Rotate base to be parallel with face
            auto matrixPosition = matrixDirection * geometry::translation(center.vector()); // Translate spike to face center

            if (tuskification != 0.0f)
            {
                model::tuskify(mSpike, axis * geometry::rotation_axis_unit(vector_type::z_direction(), -phi), tuskification, 0.0f, dzSpike);
            }

            // Apply final transformation matrix to spike
//			model::transform(mSpike, matrixPosition, matrixDirection);
			model::transform(mSpike.submesh(0), matrixPosition, matrixDirection);

            idSpikeBase += mesh.halfedge_count();
            mesh.insert(mSpike);
            brep::weld(mesh, i, mesh.surface(std::format("{}/{}", spike_name, model::SurfaceName::End0)).faces().front().id());
        }

        mesh.pack();
        mesh.check();

        return submesh_id;
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
    mesh_type m;
    id_type submesh_id = nullid;

    name = "base_spike_6";
    std::cout << name << std::endl;
    mesh.clear();
    create_cage(mesh, name, 30.0f, 60.0f, 6, 1, 0.0f, 0.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "tuskified_cage_6";
    std::cout << name << std::endl;
    mesh.clear();
    create_cage(mesh, name, 30.0f, 60.0f, 6, 250, 0.0f, math::Pi<value_type>);
//create_cage(mesh, name, 30.0f, 60.0f, 6, 5, 0.0f, math::Pi<value_type>);
//check_convex(mesh);
    triangulate_convex(mesh);
    model::write_obj(mesh, to_filename(name));
//return 0;

    name = "tuskified_swirled_cage_6";
    std::cout << name << std::endl;
    mesh.clear();
    create_cage(mesh, name, 30.0f, 60.0f, 6, 250, 0.0f, math::Pi<value_type>);
    triangulate_convex(mesh);
	model::swirl(mesh, math::Pi<value_type>);
    model::write_obj(mesh, to_filename(name));

    name = "tuskified_swirled_cage_6_model";
    std::cout << name << std::endl;
    mesh.clear();
    create_cage(mesh, name, 30.0f, 60.0f, 6, 20, 0.0f, math::Pi<value_type>);
    triangulate_convex(mesh);
	model::swirl(mesh, math::Pi<value_type>);
    model::write_obj(mesh, to_filename(name));

    name = "tuskified_cage_6_sphere";
    std::cout << name << std::endl;
    mesh.clear();
    create_cage(mesh, name, 30.0f, 60.0f, 6, 250, 0.0f, math::Pi<value_type>);
    triangulate_convex(mesh);
    /*
    m.clear();
	model::create_geodesic_sphere(m, "orb", 26.0f, 6);
    model::transform(m, geometry::translation(0.0f, 0.0f, 55.0f));
    mesh.insert(m);
    */
	submesh_id = model::create_geodesic_sphere(mesh, "orb", 26.0f, 6);
    model::transform(mesh.submesh(submesh_id), geometry::translation(0.0f, 0.0f, 55.0f));
    model::write_obj(mesh, to_filename(name));

    name = "tuskified_swirled_cage_6_sphere";
    std::cout << name << std::endl;
    mesh.clear();
    create_cage(mesh, name, 30.0f, 60.0f, 6, 250, 0.0f, math::Pi<value_type>);
    triangulate_convex(mesh);
	model::swirl(mesh, math::Pi<value_type>);
    m.clear();
	model::create_geodesic_sphere(m, "orb", 26.0f, 6);
    model::transform(m, geometry::translation(0.0f, 0.0f, 55.0f));
    mesh.insert(m);
    model::write_obj(mesh, to_filename(name));

    name = "tuskified_twisted_cage_6";
    std::cout << name << std::endl;
    mesh.clear();
    create_cage(mesh, name, 30.0f, 60.0f, 6, 250, math::Pi<value_type>, math::Pi<value_type>);
    triangulate_convex(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "tuskified_swirled_twisted_cage_6";
    std::cout << name << std::endl;
    mesh.clear();
    create_cage(mesh, name, 30.0f, 60.0f, 6, 250, math::Pi<value_type>, math::Pi<value_type>);
    triangulate_convex(mesh);
	model::swirl(mesh, math::Pi<value_type>);
    model::write_obj(mesh, to_filename(name));

    name = "tuskified_swirled_twisted_cage_6_model";
    std::cout << name << std::endl;
    mesh.clear();
    create_cage(mesh, name, 30.0f, 60.0f, 6, 20, math::Pi<value_type>, math::Pi<value_type>);
    triangulate_convex(mesh);
	model::swirl(mesh, math::Pi<value_type>);
    model::write_obj(mesh, to_filename(name));

    name = "tuskified_twisted_cage_6_sphere";
    std::cout << name << std::endl;
    mesh.clear();
    create_cage(mesh, name, 30.0f, 60.0f, 6, 250, math::Pi<value_type>, math::Pi<value_type>);
    triangulate_convex(mesh);
    m.clear();
	model::create_geodesic_sphere(m, "orb", 26.0f, 6);
    model::transform(m, geometry::translation(0.0f, 0.0f, 55.0f));
    mesh.insert(m);
    model::write_obj(mesh, to_filename(name));

    name = "tuskified_swirled_twisted_cage_6_sphere";
    std::cout << name << std::endl;
    mesh.clear();
    create_cage(mesh, name, 30.0f, 60.0f, 6, 300, math::Pi<value_type>, math::Pi<value_type>);
    triangulate_convex(mesh);

    m.clear();
	model::create_cylinder(m, "rod", 48, 30, 5.0f, 5.0f, -15.0f, 15.0f);
    model::transform(m, geometry::translation(0.0f, 0.0f, 16.0f));
    triangulate_convex(m);
    mesh.insert(m);

	model::swirl(mesh, math::Pi<value_type>);
//swirler(mesh, math::Pi<value_type>, math::PiHalf<value_type>, 60.0f);
    m.clear();
	model::create_geodesic_sphere(m, "orb", 25.0f, 6);
    model::transform(m, geometry::translation(0.0f, 0.0f, 55.0f));
    mesh.insert(m);

//    m.clear();
//    create_cylinder(m, 30, 15.0f, 5.0f, 10.0f);
//    model::transform, m(math::translation(0.0f, 0.0f, 26.0f));
//    mesh.insert(m);
//
//    m.clear();
//    create_cylinder(m, 6, 1, 2.0f, 2.0f, 5.0f);
//    model::transform, m(math::translation(10.0f, 0.0f, 26.0f));
//    mesh.insert(m);
//
//    m.clear();
//    create_cylinder(m, 6, 1, 2.0f, 2.0f, 5.0f);
//    model::transform, m(math::translation(-10.0f, 0.0f, 26.0f));
//    mesh.insert(m);

    model::write_obj(mesh, to_filename(name));

    return validator.report_results();
}
