//------------------------------------------------------------------------------
// molds
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/connection.hpp"
#include "quetzal/brep/triangulation.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/geometry/Polygon.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include <filesystem>
#include <format>
#include <iostream>
#include <optional>
#include <string>
#include <cassert>

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = double;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_type = brep::Mesh<brep::MeshTraits<vector_traits>>;

    static const filesystem::path dir_out = "out/";
    static const filesystem::path dir_ref = "reference/";
    static const string app_name = "molds";
    static const double dz_base = 5.0;
    static const double dz_rim = 5.0;

    struct Pyramid
    {
        std::string m_name;
        size_t m_polyhedron_face_edge_count; // used to find centroid so that it is within the pyramid
        size_t m_base_edge_count;
        double m_base_radius; // base radius with unit vertical side
        double m_height; // height with unit vertical side
    };

    //--------------------------------------------------------------------------
    filesystem::path to_filename(const string& name)
    {
        return dir_out / (name + ".obj");
    }

    //--------------------------------------------------------------------------
    // don't I already have this somewhere? ...
    std::optional<std::array<std::string, 2>> split_surface_name(const std::string& surface_name)
    {
        if (surface_name.empty())
        {
            return std::nullopt;
        }

        size_t pos = surface_name.length() - 1;
        while (pos > 0 && std::isdigit(surface_name[pos]))
        {
            --pos;
        }

        if (pos == surface_name.length() - 1)
        {
            return std::nullopt;
        }

//        return std::array<std::string, 2>{surface_name.substr(0, pos), surface_name.substr(pos + 1)};
        return {{surface_name.substr(0, pos + 1), surface_name.substr(pos + 1)}};
    }

/*
    //--------------------------------------------------------------------------
    id_type create_tetrahedral_vertex_pyramid(mesh_type& mesh, const string& name)
    {
        double s = 1.0;
        double r0 = s / sqrt(3.0);
        double r1 = 0.0;
        double z0 = 0.0;
        double z1 = s * sqrt(6.0) / 3.0;
        return model::create_cylinder_edges(mesh, name, 3, 1, r0, r1, z0, z1);
    }

    //--------------------------------------------------------------------------
    id_type create_hexahedral_vertex_pyramid(mesh_type& mesh, const string& name)
    {
        double s = 1.0;
        double r0 = s * sqrt(6.0) / 3.0;
        double r1 = 0.0;
        double z0 = 0.0;
        double z1 = s / sqrt(3.0);
        return model::create_cylinder_edges(mesh, name, 3, 1, r0, r1, z0, z1);
    }

    //--------------------------------------------------------------------------
    id_type create_octahedral_vertex_pyramid(mesh_type& mesh, const string& name)
    {
        double s = 1.0;
        double r0 = s / sqrt(2);
        double r1 = 0.0;
        double z0 = 0.0;
        double z1 = s / sqrt(2.0);
        return model::create_cylinder_edges(mesh, name, 4, 1, r0, r1, z0, z1);
    }

    //--------------------------------------------------------------------------
    id_type create_dodecahedral_vertex_pyramid(mesh_type& mesh, const string& name)
    {
        double s = 1.0;
        double r0 = s * sqrt((3.0 + sqrt(5.0)) / 6.0);
        double r1 = 0.0;
        double z0 = 0.0;
        double z1 = s * (sqrt(15.0) - sqrt(3.0)) / 6.0;
        return model::create_cylinder_edges(mesh, name, 3, 1, r0, r1, z0, z1);
    }

    //--------------------------------------------------------------------------
    id_type create_icosahedral_vertex_pyramid(mesh_type& mesh, const string& name)
    {
        double s = 1.0;
        double r0 = s * sqrt(50.0 + 10.0 * sqrt(5.0)) / 10.0;
        double r1 = 0.0;
        double z0 = 0.0;
        double z1 = s * sqrt((5.0 - sqrt(5.0)) / 10.0);
        return model::create_cylinder_edges(mesh, name, 5, 1, r0, r1, z0, z1);
    }
*/
    //--------------------------------------------------------------------------
    double radians(double angle)
    {
        return math::Pi<double> * angle / 180.0;
    }

    //--------------------------------------------------------------------------
    double joint_radius(double alpha, double thickness)
    {
        return thickness / (1.0 + tan(alpha / 2.0));
    }

    //--------------------------------------------------------------------------
    double circumradius(size_t edge_count, double side_length)
    {
        return side_length / (2.0 * sin(math::Pi<double> / edge_count));
    }

    //--------------------------------------------------------------------------
    double inradius(size_t edge_count, double side_length)
    {
        return side_length / (2.0 * tan(math::Pi<double> / edge_count));
    }

    //--------------------------------------------------------------------------
    double to_circumradius(size_t edge_count, double inradius)
    {
        return inradius / cos(math::Pi<double> / edge_count);
    }

    //--------------------------------------------------------------------------
    double to_inradius(size_t edge_count, double circumradius)
    {
        return circumradius * cos(math::Pi<double> / edge_count);
    }

    //--------------------------------------------------------------------------
    double pyramid_scale(const Pyramid& pyramid, double scale)
    {
        double base_inradius = to_inradius(pyramid.m_base_edge_count, pyramid.m_base_radius);
        double base_edge_halflength = sqrt(pyramid.m_base_radius * pyramid.m_base_radius - base_inradius * base_inradius);
        double side_face_height = sqrt(1.0 - base_edge_halflength * base_edge_halflength);
        double min_side_face_height = circumradius(pyramid.m_polyhedron_face_edge_count, 1.0); // polyhedron face circumradius

        if (min_side_face_height > side_face_height)
        {
            scale *= min_side_face_height / side_face_height;
        }

        return scale;
    }

    //--------------------------------------------------------------------------
    id_type create_platonic_mold(mesh_type& mesh, const std::string& name, const Pyramid& pyramid, double edge_length)
    {
        double scale = pyramid_scale(pyramid, edge_length + 11.0); // Adjust for balance and add worst case edge gap for 3 mm glass thickness at each end
        double pyramid_height_scaled = scale * pyramid.m_height;
        double z1 = pyramid_height_scaled + dz_base;

        std::string pyramid_name = name + "_pyramid";
        id_type pyramid_submesh_id = model::create_cylinder_edges(mesh, pyramid_name, pyramid.m_base_edge_count, 1, scale * pyramid.m_base_radius, 0.0, z1, z1 - pyramid_height_scaled);
        // Rename pyramid body surfaces so they don't interfere with outer surfaces
        mesh.submesh(pyramid_submesh_id).rename_surfaces(
            [&pyramid](const std::string& name)
            {
                auto [base, count] = *split_surface_name(name);
                return base == "body" ? base + std::to_string(std::stoi(count) + pyramid.m_base_edge_count) : name;
            });

        double polygon_radius = to_circumradius(pyramid.m_base_edge_count, to_inradius(pyramid.m_base_edge_count, pyramid.m_base_radius) * scale + dz_rim);
        const auto polygon = geometry::regular_polygon<vector_traits>(pyramid.m_base_edge_count, polygon_radius);
        id_type submesh_id = model::create_cylinder_edges(mesh, name, polygon, 1, 0.0, z1);
        brep::attach(mesh, mesh.extended_surface_name(submesh_id, "end1"), mesh.extended_surface_name(pyramid_submesh_id, "end0"));
        mesh.rename_submesh(pyramid_name, name); // attach should do submesh renaming automatically ...

        return submesh_id;
    }

    //--------------------------------------------------------------------------
    id_type create_angle_mold(mesh_type& mesh, const std::string& name, double theta, double width, double length)
    {
        double theta2 = math::PiHalf<double> - theta / 2.0;
        double x0 = 0.0;
        double x1 = width * cos(theta2);
        double x2 = x1 + 5.0;
        double y0 = length / 2.0;
        double z0 = 5.0;
        double z1 = z0 + width * sin(theta2);

        geometry::Polygon<vector_traits> polygon
        {
            {x0, y0, z0},
            {-x1, y0, z1},
            {-x2, y0, z1},
            {-x2, y0, 0.0},
            {x2, y0, 0.0},
            {x2, y0, z1},
            {x1, y0, z1},
        };

        return model::create_extrusion(mesh, name, polygon, {0.0, length, 0.0});
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    FileValidator validator(dir_out, dir_ref, app_name);

    mesh_type mesh;

    // read edge length from command line
    double edge_length = 75.0;

    std::array<Pyramid, 5> pyramids
    {{
        {
            "tetrahedron",
            3,
            3,
            1.0 / sqrt(3.0),
            sqrt(6.0) / 3.0,
        },
        {
            "hexahedron",
            4,
            3,
            sqrt(6.0) / 3.0,
            1.0 / sqrt(3.0),
        },
        {
            "octahedron",
            3,
            4,
            1.0 / sqrt(2.0),
            1.0 / sqrt(2.0),
        },
        {
            "dodecahedron",
            5,
            3,
            sqrt((3.0 + sqrt(5.0)) / 6.0),
            (sqrt(15.0) - sqrt(3.0)) / 6.0,
        },
        {
            "icosahedron",
            3,
            5,
            sqrt(50.0 + 10.0 * sqrt(5.0)) / 10.0,
            sqrt((5.0 - sqrt(5.0)) / 10.0),
        },
     }};

    for (const auto& pyramid : pyramids)
    {
        string name = std::format("{}_mold", pyramid.m_name);
        cout << name << endl;
        mesh.clear();
        id_type submesh_id = create_platonic_mold(mesh, name, pyramid, edge_length);
        auto& submesh = mesh.submesh(submesh_id);
        triangulate(submesh);
        model::write_submesh_obj(submesh, to_filename(name));
    }

    std::array<std::tuple<std::string, double>, 7> polygon_angles =
    {{
        {"degenerate_closed", 0.0},
        {"triangle", 60.0},
        {"square", 90.0},
        {"pentagon", 108.0},
        {"hexagon", 120.0},
        {"octagon", 135.0},
        {"degenerate_open", 180.0},
    }};

    for (const auto [name, angle] : polygon_angles)
    {
        auto theta = radians(angle);
        auto radius = joint_radius(theta, 3.0);
        cout << "joint radius" << "\t" << name << "\t" << angle << "\t" << theta << "\t" << radius << "\t" << (2.0 * radius) << endl;
    }

    auto regular_polygon_angles = polygon_angles | std::views::drop(1) | std::views::take(polygon_angles.size() - 2);

    for (const auto [polygon_name, angle] : regular_polygon_angles)
    {
        string name = std::format("{}_mold", polygon_name);
        cout << name << endl;
        mesh.clear();
        auto theta = radians(angle);
        id_type submesh_id = create_angle_mold(mesh, name, theta, 80.0, 40.0);
        auto& submesh = mesh.submesh(submesh_id);
        triangulate(submesh);
        model::write_submesh_obj(submesh, to_filename(name));
    }

    return validator.report_results();
}
