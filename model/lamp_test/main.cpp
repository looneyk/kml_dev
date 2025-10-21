//------------------------------------------------------------------------------
// lamp_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/mesh_util.hpp"
#include "quetzal/brep/triangulation.hpp"
#include "quetzal/math/math_util_dx.hpp"
#include "quetzal/model/mesh_attributes.hpp"
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

    //--------------------------------------------------------------------------
    template<typename V>
    void check_texcoord(V& vertex)
    {
        const auto& texcoord = vertex.value().texcoord();
        if (texcoord.x() != value_type(0) && texcoord.x() != value_type(1))
        {
            cout << "Bad: texcoord x " << texcoord.x() << " face " << vertex.halfedge().face_id() << " halfedge " << vertex.halfedge_id() << endl;
        }
        if (texcoord.y() != value_type(0) && texcoord.y() != value_type(1))
        {
            cout << "Bad: texcoord y " << texcoord.y() << " face " << vertex.halfedge().face_id() << " halfedge " << vertex.halfedge_id() << endl;
        }

        return;
    }

    //--------------------------------------------------------------------------
    template<typename M>
    void check_texcoords(M& mesh)
    {
        for (const auto& face : mesh.faces())
        {
            for (const auto& halfedge : face.halfedges())
            {
                check_texcoord(halfedge.vertex());
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    template<typename M>
    void create_lamp(M& mesh, size_t nAzimuth, value_type wPanel, value_type dx, value_type dz, value_type tzLower)
    {
        assert(nAzimuth > 2);

        value_type x0 = wPanel / (2.0f * tan(math::Pi<value_type> / nAzimuth));
        value_type x1 = x0 + dx;
        value_type xSide = wPanel / (2.0f * tan(math::Pi<value_type> / (2.0f * nAzimuth)));
        value_type y0 = -wPanel / 2.0f;
        value_type y1 = -y0;
        value_type z0 = 0.0f;
        value_type z1 = tzLower * dz;
        value_type z2 = dz;

        if (dx < value_type(0))
        {
            x1 = x0;
            x0 = x1 - dx;
        }

        geometry::Polygon<typename M::traits_type::vector_traits> panel(4);
        mesh_type::vector_type direction;
        mesh_type::vector_type normal;
        value_type thickness = 3.0f;

        // Upper panel
        panel.vertices()[0] = {x0, y0, z2};
        panel.vertices()[1] = {x1, y0, z1};
        panel.vertices()[2] = {x1, y1, z1};
        panel.vertices()[3] = {x0, y1, z2};
        direction = {z2 - z1, 0.0f, dx};
        normal = normalize(direction);
        normal = dx < value_type(0) ? -normal : normal;
        M mUpper;
        model::create_extrusion(mUpper, panel, normal, normal * thickness);

        // Lower panel
        panel.vertices()[0] = {x0, y1, z0};
        panel.vertices()[1] = {x1, y1, z1};
        panel.vertices()[2] = {x1, y0, z1};
        panel.vertices()[3] = {x0, y0, z0};
        direction = {z1 - z0, 0.0f, -dx};
        normal = normalize(direction);
        normal = dx < value_type(0) ? -normal : normal;
        M mLower;
        model::create_extrusion(mLower, panel, normal, normal * thickness);

        // Side panel
        panel.vertices()[0] = {xSide, y0, z0};
        panel.vertices()[1] = {xSide, y1, z0};
        panel.vertices()[2] = {xSide, y1, z2};
        panel.vertices()[3] = {xSide, y0, z2};
        direction = {1.0f, 0.0f, 0.0f};
        normal = normalize(direction);
        M mSide;
        model::create_extrusion(mSide, panel, normal, normal * thickness);

        math::Matrix<value_type> matrix;

        for (size_t i = 0; i < nAzimuth; ++i)
        {
            value_type phi = math::PiTwo<value_type> * value_type(i) / value_type(nAzimuth);

            M m = mUpper;
            matrix = math::rotation_z3(phi);
            model::transform(m, matrix);
            mesh.insert(m);

            m = mLower;
            model::transform(m, matrix);
            mesh.insert(m);

            m = mSide;
            matrix = math::rotation_z3(phi + math::Pi<value_type> / value_type(nAzimuth));
            model::transform(m, matrix);
            mesh.insert(m);
        }

        check_texcoords(mesh);

        mesh.check();
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

    name = "lamp_4_outward.obj";
    cout << name << endl;
    mesh.clear();
    create_lamp(mesh, 4, 30.0f, 50.0f, 190.0f, 0.5f);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name);

    name = "lamp_4_outward_wide.obj";
    cout << name << endl;
    mesh.clear();
    create_lamp(mesh, 4, 40.0f, 50.0f, 190.0f, 0.5f);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name);

    name = "lamp_4_outward_third.obj";
    cout << name << endl;
    mesh.clear();
    create_lamp(mesh, 4, 30.0f, 50.0f, 190.0f, 0.333f);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name);

    name = "lamp_4_outward_third_wide.obj";
    cout << name << endl;
    mesh.clear();
    create_lamp(mesh, 4, 40.0f, 50.0f, 190.0f, 0.333f);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name);

    name = "lamp_6_outward.obj";
    cout << name << endl;
    mesh.clear();
    create_lamp(mesh, 6, 30.0f, 50.0f, 190.0f, 0.5f);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name);

    name = "lamp_6_outward_wide.obj";
    cout << name << endl;
    mesh.clear();
    create_lamp(mesh, 6, 40.0f, 50.0f, 190.0f, 0.5f);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name);

    name = "lamp_6_outward_third.obj";
    cout << name << endl;
    mesh.clear();
    create_lamp(mesh, 6, 30.0f, 50.0f, 190.0f, 0.333f);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name);

    name = "lamp_6_outward_third_wide.obj";
    cout << name << endl;
    mesh.clear();
    create_lamp(mesh, 6, 40.0f, 50.0f, 190.0f, 0.333f);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name);

    name = "lamp_4_inward.obj";
    cout << name << endl;
    mesh.clear();
    create_lamp(mesh, 4, 30.0f, -50.0f, 190.0f, 0.5f);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name);

    name = "lamp_4_inward_wide.obj";
    cout << name << endl;
    mesh.clear();
    create_lamp(mesh, 4, 40.0f, -50.0f, 190.0f, 0.5f);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name);

    name = "lamp_4_inward_third.obj";
    cout << name << endl;
    mesh.clear();
    create_lamp(mesh, 4, 30.0f, -50.0f, 190.0f, 0.333f);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name);

    name = "lamp_4_inward_third_wide.obj";
    cout << name << endl;
    mesh.clear();
    create_lamp(mesh, 4, 40.0f, -50.0f, 190.0f, 0.333f);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name);

    name = "lamp_6_inward.obj";
    cout << name << endl;
    mesh.clear();
    create_lamp(mesh, 6, 30.0f, -50.0f, 190.0f, 0.5f);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name);

    name = "lamp_6_inward_wide.obj";
    cout << name << endl;
    mesh.clear();
    create_lamp(mesh, 6, 40.0f, -50.0f, 190.0f, 0.5f);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name);

    name = "lamp_6_inward_third.obj";
    cout << name << endl;
    mesh.clear();
    create_lamp(mesh, 6, 30.0f, -50.0f, 190.0f, 0.333f);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name);

    name = "lamp_6_inward_third_wide.obj";
    cout << name << endl;
    mesh.clear();
    create_lamp(mesh, 6, 40.0f, -50.0f, 190.0f, 0.333f);
    triangulate(mesh);
    model::write_obj(mesh, dirOut + name);

    return 0;
}
