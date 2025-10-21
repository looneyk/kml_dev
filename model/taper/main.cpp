//------------------------------------------------------------------------------
// taper
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshConstructor.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/clip.hpp"
#include "quetzal/brep/connection.hpp"
#include "quetzal/brep/mesh_util.hpp"
#include "quetzal/brep/triangulation.hpp"
#include "quetzal/common/FileValidator.hpp"
#include "quetzal/geometry/Plane.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/Polygon.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/model/mesh_attributes.hpp"
#include "quetzal/model/obj_io.hpp"
#include "quetzal/model/primitives.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <cmath>
#include <cassert>

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
//    using value_type = double;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using mesh_traits = brep::MeshTraits<vector_traits>;
    using mesh_type = brep::Mesh<mesh_traits>;
    using vertex_attributes_type = mesh_type::vertex_attributes_type;
    using face_attributes_type = mesh_type::face_attributes_type;
    using surface_attributes_type = mesh_type::surface_attributes_type;
    using point_type = mesh_type::point_type;
    using vector_type = point_type::vector_type;
    using plane_type = geometry::Plane<vector_traits>;
    using polygon_type = geometry::Polygon<vector_traits>;

    static const string dirOut = "out/";
    static const string dirReference = "reference/";

    //--------------------------------------------------------------------------
    string to_filename(const string& name)
    {
        return dirOut + name + ".obj";
    }

    //--------------------------------------------------------------------------
    template<typename M>
    id_type create_taper(M& mesh, const std::string& name, size_t nAzimuth, size_t nz, value_type radius, value_type zLower, value_type zUpper)
    {
        id_type idSubmesh = model::create_cylinder(mesh, name, nAzimuth, nz, radius, zLower, zUpper);

        size_t n = nAzimuth;
        for (size_t j = 1; j < 3; ++j)
        {
            value_type theta = math::PiHalf<value_type> * j / 3.0f;
            value_type cos_theta = cos(theta);
            value_type sin_theta = sin(theta);
            value_type phi_offset = math::PiTwo<value_type> / n;

            n /= 2;
            for (size_t i = 0; i < n; ++i)
            {
                value_type phi = phi_offset + math::PiTwo<value_type> * i / n;
                vector_type normal = {cos(phi) * cos_theta, sin(phi) * cos_theta, sin_theta};
                normal.normalize();
                point_type point = normal * radius;
                plane_type plane(point, normal);
                ostringstream oss;
                oss << "side_" << j << "_" << i;
                brep::clip(mesh, plane, oss.str());
            }
        }

        return idSubmesh;
    }

    //--------------------------------------------------------------------------
    template<typename M>
    id_type create_blades(M& mesh, const std::string& name, size_t nAzimuth, value_type rLower, value_type rUpper, value_type zLower, value_type zUpper)
    {
        brep::MeshConstructor mc(mesh);

        id_type idSubmesh = mesh.create_submesh(name);

        polygon_type lower(nAzimuth);
        polygon_type upper(nAzimuth);
        for (size_t i = 0; i < nAzimuth; ++i)
        {
            value_type phi = math::PiTwo<value_type> * i / nAzimuth;
            value_type cos_phi = cos(phi);
            value_type sin_phi = sin(phi);
            lower.set_vertex(i, {rLower * cos_phi, rLower * sin_phi, zLower});
            upper.set_vertex(i, {rUpper * cos_phi, rUpper * sin_phi, zUpper});
        }

        for (size_t i = 0; i < nAzimuth; ++i)
        {
            size_t iLeft = i;
            size_t iRight = (i + 1) % nAzimuth;
            point_type finLower = 1.5f * (to_vector(lower.vertex(iLeft)) + to_vector(lower.vertex(iRight)));
            vector_type normalLeft = cross(finLower - lower.vertex(iLeft), upper.vertex(iLeft) - lower.vertex(iLeft));
            vector_type normalRight = cross(upper.vertex(iRight) - lower.vertex(iRight), finLower - lower.vertex(iRight));
            vector_type direction = normalize(cross(normalLeft, normalRight));
            point_type finUpper = finLower + direction * (zUpper - zLower) * 2.0f;
            vector_type normalLower = cross(lower.vertex(iRight) - lower.vertex(iLeft), finLower - lower.vertex(iLeft));
            vector_type normalUpper = cross(upper.vertex(iLeft) - upper.vertex(iRight), finUpper - upper.vertex(iRight));

            auto surface_name = [name, i](const std::string& s) { return name + "_" + s + to_string(i); };

            mc.surface_face(idSubmesh, surface_name("left"), normalLeft, {upper.vertex(iLeft), lower.vertex(iLeft), finLower, finUpper});
            mc.surface_face(idSubmesh, surface_name("right"), normalRight, {lower.vertex(iRight), upper.vertex(iRight), finUpper, finLower});
            mc.surface_face(idSubmesh, surface_name("lower"), normalLower, {lower.vertex(iLeft), lower.vertex(iRight), finLower});
            mc.surface_face(idSubmesh, surface_name("upper"), normalUpper, {upper.vertex(iRight), upper.vertex(iLeft), finUpper});
        }

        mc.close();

        auto slope = (zUpper - zLower) / (rUpper - rLower);
        auto z0 = zLower - slope * rLower;
        auto zPinnacle = z0;
        auto rBase = rLower + rLower - rUpper;
        auto zBase = z0 + slope * rBase;

        mesh_type m;
        model::create_prism(m, "top", 6, 1, rUpper, 0.0f, zUpper, zPinnacle);
        mesh.insert(m);
        weld (mesh, name + "/s1", "top/end0");

        m.clear();
        model::create_prism(m, "bottom", 6, 1, rBase, rLower, zBase, zLower);
        mesh.insert(m);
        weld (mesh, name + "/s0", "bottom/end1");

//        model::unify_vertex_positions(mesh);

        return idSubmesh;
    }

    //--------------------------------------------------------------------------
    template<typename M>
    id_type create_fins(M& mesh, const std::string& name, size_t nAzimuth, value_type radius, value_type zApex, value_type zLower, value_type zUpper)
    {
        brep::MeshConstructor mc(mesh);

        id_type idSubmesh = mesh.create_submesh(name);

        //value_type factor_in = 0.5f;

        value_type rBase = radius;
        value_type rLower = radius * (zApex + zLower) / zApex;
        value_type rUpper = radius * (zApex - zUpper) / zApex;
        value_type rBaseIn = rBase * 0.5f;
        value_type rLowerIn = rLower * 0.5f;
        value_type rUpperIn = rUpper * 0.5f;

        //value_type zLowerIn = zLower;
        //value_type zUpperIn = zUpper;

        polygon_type base(nAzimuth);
        polygon_type base_in(nAzimuth);
        polygon_type lower(nAzimuth);
        polygon_type lower_in(nAzimuth);
        polygon_type upper(nAzimuth);
        polygon_type upper_in(nAzimuth);

        value_type dphi = math::PiTwo<value_type> / (2.0f * nAzimuth);

        for (size_t i = 0; i < nAzimuth; ++i)
        {
            value_type phi = math::PiTwo<value_type> * i / nAzimuth;
            value_type cos_phi = cos(phi);
            value_type sin_phi = sin(phi);
            base.set_vertex(i, {rBase * cos_phi, rBase * sin_phi, 0.0f});
            lower.set_vertex(i, {rLower * cos_phi, rLower * sin_phi, zLower});
            upper.set_vertex(i, {rUpper * cos_phi, rUpper * sin_phi, zUpper});

            phi += dphi;
            cos_phi = cos(phi);
            sin_phi = sin(phi);
            base_in.set_vertex(i, {rBaseIn * cos_phi, rBaseIn * sin_phi, 0.0f});
            lower_in.set_vertex(i, {rLowerIn * cos_phi, rLowerIn * sin_phi, zLower});
            upper_in.set_vertex(i, {rUpperIn * cos_phi, rUpperIn * sin_phi, zUpper});
        }

        point_type point_lower{0.0f, 0.0f, zLower};
        point_type point_upper{0.0f, 0.0f, zUpper};

        vector_type normalLower{0.0f, 0.0f, -1.0f};
        vector_type normalUpper{0.0f, 0.0f, 1.0f};

        id_type idSurfaceLower = mesh.create_surface(idSubmesh, "lower", {normalLower});
        id_type idSurfaceUpper = zUpper == zApex ? nullid : mesh.create_surface(idSubmesh, "upper", {normalLower});

        vector_type normal{0.0f, 0.0f, 1.0f}; // calculate this for each ...

        auto surface_name = [](const std::string& prefix, size_t i) { return prefix + to_string(i); };

        for (size_t i = 0; i < nAzimuth; ++i)
        {
            size_t iLeft = (i + nAzimuth - 1) % nAzimuth;;
            size_t iRight = i;

            mc.face(idSurfaceLower, normalLower, {point_lower, lower.vertex(i), lower_in.vertex(iLeft)});
            mc.face(idSurfaceLower, normalLower, {lower.vertex(i), point_lower, lower_in.vertex(iRight)});

            // these are not planar, decide how to triangulate, or determine the differential scaling that would, I am assuming, render them planar ...
            // weird, it does seem to be planar? ...
            mc.surface_face(idSubmesh, surface_name("left_lower_", i), normal, {base_in.vertex(iLeft), lower_in.vertex(iLeft), lower.vertex(i), base.vertex(i)});
            mc.surface_face(idSubmesh, surface_name("right_lower_", i), normal, {lower_in.vertex(iRight), base_in.vertex(iRight), base.vertex(i), lower.vertex(i)});

            if (zUpper == zApex)
            {
                mc.surface_face(idSubmesh, surface_name("left_upper_", i), normal, {point_upper, base_in.vertex(iLeft), base.vertex(i)});
                mc.surface_face(idSubmesh, surface_name("right_upper_", i), normal, {base_in.vertex(iRight), point_upper, base.vertex(i)});
            }
            else
            {
                mc.surface_face(idSubmesh, surface_name("left_upper_", i), normal, {upper_in.vertex(iLeft), base_in.vertex(iLeft), base.vertex(i), upper.vertex(i)});
                mc.surface_face(idSubmesh, surface_name("right_upper_", i), normal, {base_in.vertex(iRight), upper_in.vertex(iRight), upper.vertex(i), base.vertex(i)});

                mc.face(idSurfaceUpper, normalUpper, {upper.vertex(i), point_upper, upper_in.vertex(iLeft)});
                mc.face(idSurfaceUpper, normalUpper, {point_upper, upper.vertex(i), upper_in.vertex(iRight)});
            }
        }

        mc.close();
        return idSubmesh;
    }

    //--------------------------------------------------------------------------
    template<typename M>
    id_type create_fins2(M& mesh, const std::string& name, size_t nAzimuth, value_type radius, value_type zApex, value_type zLower, value_type zUpper)
    {
        id_type idSubmesh = create_fins(mesh, name, nAzimuth, radius, zApex, zLower, zUpper);

        mesh_type m;
        brep::MeshConstructor mc(m);

        auto r = radius / 2.0f;
        auto r2 = r / 2.0f;
        auto h = r * sqrt(3.0f) / 2.0f;

        point_type p0 = {r, -r2, 0.0f};
        point_type p1 = {r, r2, 0.0f};
        point_type p2 = {-r2, r2, 0.0f};
        point_type p3 = {-r2, -r2, 0.0f};
        point_type p4 = {1.5f * r, 0.0f, 1.5f * h};
        point_type p5 = {-1.5f * r, 0.0f, 1.5f * h};

        vector_type normal{0.0f, 0.0f, 1.0f}; // calculate this for each ...

        id_type idExtension = mc.submesh(name);
        mc.surface_face(idExtension, "xa", normal, {p0, p3, p2, p1});
        mc.surface_face(idExtension, "xb", normal, {p0, p4, p5, p3});
        mc.surface_face(idExtension, "xc", normal, {p2, p5, p4, p1});
        mc.surface_face(idExtension, "xd", normal, {p0, p1, p4});
        mc.surface_face(idExtension, "xe", normal, {p2, p3, p5});

        mc.close();

        for (size_t i = 0; i < nAzimuth; ++i)
        {
            auto mx = m;
            for (auto& surface : mx.surfaces())
            {
                mx.rename_surface(surface.id(), surface.name() + "_" + to_string(i));
            }

            auto phi = math::PiTwo<value_type> * i / nAzimuth;
            auto theta = atan2(zUpper, radius);
            auto matrix = geometry::scaling(1.0f, 0.6f, 0.6f) * geometry::rotation_y(theta) * geometry::translation(radius * 0.5f, 0.0f, zUpper * 0.4f) * geometry::rotation_z3(phi);

            model::transform(mx, matrix);
            mesh.insert(mx);
        }

        return idSubmesh;
    }

    //--------------------------------------------------------------------------
    template<typename M>
    id_type create_cube(M& mesh, const std::string& name, value_type d)
    {
        brep::MeshConstructor mc(mesh);

        id_type idSubmesh = mesh.create_submesh(name);

        point_type p0{0.0f, 0.0f, 0.0f};
        point_type p1{d, 0.0f, 0.0f};
        point_type p2{d, d, 0.0f};
        point_type p3{0.0f, d, 0.0f};
        point_type p4{0.0f, 0.0f, d};
        point_type p5{d, 0.0f, d};
        point_type p6{d, d, d};
        point_type p7{0.0f, d, d};

        mc.surface_face(idSubmesh, "right", {1.0f, 0.0f, 0.0f}, {p0, p1, p5, p4});
        mc.surface_face(idSubmesh, "back", {0.0f, 1.0f, 0.0f}, {p1, p2, p6, p5});
        mc.surface_face(idSubmesh, "left", {-1.0f, 0.0f, 0.0f}, {p2, p3, p7, p6});
        mc.surface_face(idSubmesh, "front", {0.0f, -1.0f, 0.0f}, {p3, p0, p4, p7});
        mc.surface_face(idSubmesh, "bottom", {0.0f, 0.0f, -1.0f}, {p0, p3, p2, p1});
        mc.surface_face(idSubmesh, "top", {0.0f, 0.0f, 1.0f}, {p4, p5, p6, p7});

        mc.close();

        return idSubmesh;
    }

    //--------------------------------------------------------------------------
    template<typename M>
    id_type create_cube_open(M& mesh, const std::string& name, value_type d)
    {
        brep::MeshConstructor mc(mesh);

        id_type idSubmesh = mesh.create_submesh(name);

        point_type p0{0.0f, 0.0f, 0.0f};
        point_type p1{d, 0.0f, 0.0f};
        point_type p2{d, d, 0.0f};
        point_type p3{0.0f, d, 0.0f};
        point_type p4{0.0f, 0.0f, d};
        point_type p5{d, 0.0f, d};
        point_type p6{d, d, d};
        point_type p7{0.0f, d, d};

        mc.surface_face(idSubmesh, "right", {1.0f, 0.0f, 0.0f}, {p0, p1, p5, p4});
        mc.surface_face(idSubmesh, "back", {0.0f, 1.0f, 0.0f}, {p1, p2, p6, p5});
        mc.surface_face(idSubmesh, "left", {-1.0f, 0.0f, 0.0f}, {p2, p3, p7, p6});
        mc.surface_face(idSubmesh, "front", {0.0f, -1.0f, 0.0f}, {p3, p0, p4, p7});

        mc.close();

        return idSubmesh;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[]
{
    argc;
    argv;

    FileValidator validator(dirOut, dirReference);

    string name;
    mesh_type mesh;

    name = "taper";
    cout << name << endl;
    mesh.clear();
    create_taper(mesh, name, 24, 1, 40.0f, -40.0f, 80.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cube";
    cout << name << endl;
    mesh.clear();
    create_cube(mesh, name, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "cube_open";
    cout << name << endl;
    mesh.clear();
    create_cube_open(mesh, name, 10.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "blades";
    cout << name << endl;
    mesh.clear();
    create_blades(mesh, name, 6, 20.0f, 10.0f, -10.0f, 40.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "fins";
    cout << name << endl;
    mesh.clear();
    create_fins(mesh, name, 6, 40.0f, 90.0f, -30.0f, 90.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    name = "fins2";
    cout << name << endl;
    mesh.clear();
    create_fins2(mesh, name, 6, 40.0f, 90.0f, -30.0f, 90.0f);
    triangulate(mesh);
    model::write_obj(mesh, to_filename(name));

    validator.report_results();
    return 0;
}
