//------------------------------------------------------------------------------
// sand_press
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/model/primitives.hpp"
//#include "quetzal/model/transforms.hpp"
#include "quetzal/model/mesh_attributes.hpp"
//#include "quetzal/model/math_util.hpp"
#include "quetzal/model/mesh_type.hpp"
#include "quetzal/model/obj_io.hpp"
#include <iostream>

using namespace std;
using namespace DirectX;
using namespace quetzal::model;

namespace
{

    //--------------------------------------------------------------------------
    void create_press(model_mesh_type& mesh, float r0, float r1, float zDepth, float zHandle)
    {
        model_mesh_type m;

        m.m_vertices.resize(24);

        for (int i = 0; i < 6; ++i)
        {
            float phi = XM_2PI * i / 6;
            float x = r1 * cos(phi);
            float y = r1 * sin(phi);

            m.m_vertices[i + 0] ={{x, y, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}};
            m.m_vertices[i + 12] ={{x, y, zDepth}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}};

            x = r0 * cos(phi);
            y = r0 * sin(phi);

            m.m_vertices[i + 6] ={{x, y, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}};
            m.m_vertices[i + 18] ={{x, y, zDepth}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}};
        }

        // Handles

        Vector3 position;
        position.set_x((2.0f * m.m_vertices[13].m_position.x() + m.m_vertices[14].m_position.x()) / 3.0f);
        position.set_y(m.m_vertices[13].m_position.y());
        position.set_z(zDepth + zHandle);
        m.m_vertices.push_back({position, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}});

        position.set_x((2.0f * m.m_vertices[14].m_position.x() + m.m_vertices[13].m_position.x()) / 3.0f);
        position.set_y(m.m_vertices[14].m_position.y());
        position.set_z(zDepth + zHandle);
        m.m_vertices.push_back({position, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}});

        position.set_x((2.0f * m.m_vertices[16].m_position.x() + m.m_vertices[17].m_position.x()) / 3.0f);
        position.set_y(m.m_vertices[16].m_position.y());
        position.set_z(zDepth + zHandle);
        m.m_vertices.push_back({position, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}});

        position.set_x((2.0f * m.m_vertices[17].m_position.x() + m.m_vertices[16].m_position.x()) / 3.0f);
        position.set_y(m.m_vertices[17].m_position.y());
        position.set_z(zDepth + zHandle);
        m.m_vertices.push_back({position, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}});

        position.set_x((2.0f * m.m_vertices[13].m_position.x() + m.m_vertices[14].m_position.x()) / 3.0f);
        position.set_y(m.m_vertices[19].m_position.y());
        position.set_z(zDepth + zHandle);
        m.m_vertices.push_back({position, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}});

        position.set_x((2.0f * m.m_vertices[14].m_position.x() + m.m_vertices[13].m_position.x()) / 3.0f);
        position.set_y(m.m_vertices[20].m_position.y());
        position.set_z(zDepth + zHandle);
        m.m_vertices.push_back({position, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}});

        position.set_x((2.0f * m.m_vertices[16].m_position.x() + m.m_vertices[17].m_position.x()) / 3.0f);
        position.set_y(m.m_vertices[22].m_position.y());
        position.set_z(zDepth + zHandle);
        m.m_vertices.push_back({position, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}});

        position.set_x((2.0f * m.m_vertices[17].m_position.x() + m.m_vertices[16].m_position.x()) / 3.0f);
        position.set_y(m.m_vertices[23].m_position.y());
        position.set_z(zDepth + zHandle);
        m.m_vertices.push_back({position, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}});

        // Bottom
        m.append_indices({0, 6, 7, 7, 1, 0});
        m.append_indices({1, 7, 8, 8, 2, 1});
        m.append_indices({2, 8, 9, 9, 3, 2});
        m.append_indices({3, 9, 10, 10, 4, 3});
        m.append_indices({4, 10, 11, 11, 5, 4});
        m.append_indices({5, 11, 6, 6, 0, 5});

        // Outer
        m.append_indices({0, 1, 13, 13, 12, 0});
        m.append_indices({1, 2, 14, 14, 13, 1});
        m.append_indices({2, 3, 15, 15, 14, 2});
        m.append_indices({3, 4, 16, 16, 15, 3});
        m.append_indices({4, 5, 17, 17, 16, 4});
        m.append_indices({5, 0, 12, 12, 17, 5});

        // Inner
        m.append_indices({7, 6, 18, 18, 19, 7});
        m.append_indices({8, 7, 19, 19, 20, 8});
        m.append_indices({9, 8, 20, 20, 21, 9});
        m.append_indices({10, 9, 21, 21, 22, 10});
        m.append_indices({11, 10, 22, 22, 23, 11});
        m.append_indices({6, 11, 23, 23, 18, 6});

        // Top
        m.append_indices({12, 13, 19, 19, 18, 12});
//        m.append_indices({13, 14, 20, 20, 19, 13});
        m.append_indices({14, 15, 21, 21, 20, 14});
        m.append_indices({15, 16, 22, 22, 21, 15});
//        m.append_indices({16, 17, 23, 23, 22, 16});
        m.append_indices({17, 12, 18, 18, 23, 17});

        // Handles
        m.append_indices({13, 14, 25, 25, 24, 13});
        m.append_indices({20, 19, 28, 28, 29, 20});
        m.append_indices({13, 24, 28, 28, 19, 13});
        m.append_indices({24, 25, 29, 29, 28, 24});
        m.append_indices({29, 25, 14, 14, 20, 29});

        m.append_indices({16, 17, 27, 27, 26, 16});
        m.append_indices({23, 22, 30, 30, 31, 23});
        m.append_indices({16, 26, 30, 30, 22, 16});
        m.append_indices({26, 27, 31, 31, 30, 26});
        m.append_indices({31, 27, 17, 17, 23, 31});

        calculate_face_normals(m);

        mesh.insert(m);

        assert(mesh.m_indices.size() % 3 == 0);
        assert(check_normals(mesh));

        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    model_mesh_type mesh;

    // Large press
    cout << "sand_press_1.obj" << endl;
//    mesh.clear();
//    create_press(mesh, 86.7f, 104.5f, 25.0f, 35.0f);
    create_press(mesh, 91.5f, 104.5f, 25.0f, 35.0f);
//    write_obj(mesh, "sand_press_1.obj");

    // Small press
    cout << "sand_press_2.obj" << endl;
//    mesh.clear();
//    float scale = 0.73f;
    create_press(mesh, 65.5f, 78.5f, 25.0f, 35.0f);
//    write_obj(mesh, "sand_press_2.obj");

    write_obj(mesh, "sand_press.obj");

    cout << "sand_press_3.obj" << endl;
    mesh.clear();
    create_press(mesh, 39.5f, 52.5f, 25.0f, 35.0f);
    write_obj(mesh, "sand_press_3.obj");

    return 0;
}
