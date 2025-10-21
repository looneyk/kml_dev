//------------------------------------------------------------------------------
// glass_form_tetra
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/direct3d11/Generator.hpp"
#include "quetzal/direct3d11/Vertex.hpp"
#include "quetzal/direct3d11/obj_util.hpp"
#include "quetzal/direct3d11/math_util.hpp"
#include <iostream>

using namespace std;
using namespace DirectX;
using namespace quetzal::direct3d11;

namespace
{

    //--------------------------------------------------------------------------
    void add_face(Mesh& mesh, XMVECTOR v0, XMVECTOR v1, XMVECTOR v2)
    {
        XMVECTOR normal = XMVector3Normalize(XMVector3Cross(v1 - v0, v2 - v1));
        XMFLOAT3 f3normal = to_xmfloat3(normal);

        unsigned int i0 = mesh.m_vertices.size();

        mesh.m_vertices.push_back({to_xmfloat3(v0), f3normal, {}, {}});
        mesh.m_vertices.push_back({to_xmfloat3(v1), f3normal, {}, {}});
        mesh.m_vertices.push_back({to_xmfloat3(v2), f3normal, {}, {}});

        mesh.m_indices.push_back(i0 + 0);
        mesh.m_indices.push_back(i0 + 1);
        mesh.m_indices.push_back(i0 + 2);

        return;
    }

    //--------------------------------------------------------------------------
    void create_glass_form_tetra(Mesh& mesh, float edge, float truncation)
    {
        float radius = edge * sqrt(6.0f) / 4.0f;

        // Create full size tetrahedron
        Mesh mReference;
        create_tetrahedron(mReference, radius);

        // Adjust original vertex positions and add internal set

        assert((mReference.m_indices.size() % 3) == 0);
        unsigned int n = mReference.m_indices.size(); // Original tetrahedron vertices

        // Add common internal vertices
        XMVECTOR pos0 = XMLoadFloat3(&mReference.m_vertices[mReference.m_indices[0]].m_position);
        XMVECTOR pos1 = XMLoadFloat3(&mReference.m_vertices[mReference.m_indices[2]].m_position);
        XMVECTOR pos2 = XMLoadFloat3(&mReference.m_vertices[mReference.m_indices[1]].m_position);
        XMVECTOR pos3 = XMLoadFloat3(&mReference.m_vertices[mReference.m_indices[5]].m_position);

        float inner_scale = (1.0f - truncation);
        XMFLOAT3 f3pos0 = to_xmfloat3(pos0 * inner_scale);
        XMFLOAT3 f3pos1 = to_xmfloat3(pos1 * inner_scale);
        XMFLOAT3 f3pos2 = to_xmfloat3(pos2 * inner_scale);
        XMFLOAT3 f3pos3 = to_xmfloat3(pos3 * inner_scale);

        mReference.m_vertices.push_back({f3pos0, {}, {}, {}});
        mReference.m_vertices.push_back({f3pos2, {}, {}, {}});
        mReference.m_vertices.push_back({f3pos1, {}, {}, {}});

        mReference.m_vertices.push_back({f3pos0, {}, {}, {}});
        mReference.m_vertices.push_back({f3pos1, {}, {}, {}});
        mReference.m_vertices.push_back({f3pos3, {}, {}, {}});

        mReference.m_vertices.push_back({f3pos1, {}, {}, {}});
        mReference.m_vertices.push_back({f3pos2, {}, {}, {}});
        mReference.m_vertices.push_back({f3pos3, {}, {}, {}});

        mReference.m_vertices.push_back({f3pos2, {}, {}, {}});
        mReference.m_vertices.push_back({f3pos0, {}, {}, {}});
        mReference.m_vertices.push_back({f3pos3, {}, {}, {}});

        for (unsigned int i = 0; i < n; ++i)
        {
            mReference.m_indices.push_back(n + i);
        }

        // Shrink each face
        for (unsigned int i = 0; i < n; i += 3)
        {
            XMVECTOR pos0 = XMLoadFloat3(&mReference.m_vertices[mReference.m_indices[i + 0]].m_position);
            XMVECTOR pos1 = XMLoadFloat3(&mReference.m_vertices[mReference.m_indices[i + 1]].m_position);
            XMVECTOR pos2 = XMLoadFloat3(&mReference.m_vertices[mReference.m_indices[i + 2]].m_position);

            XMVECTOR center = (pos0 + pos1 + pos2) / 3.0f;

            pos0 -= (pos0 - center) * truncation;
            pos1 -= (pos1 - center) * truncation;
            pos2 -= (pos2 - center) * truncation;

            mReference.m_vertices[mReference.m_indices[i + 0]].m_position = to_xmfloat3(pos0);
            mReference.m_vertices[mReference.m_indices[i + 1]].m_position = to_xmfloat3(pos1);
            mReference.m_vertices[mReference.m_indices[i + 2]].m_position = to_xmfloat3(pos2);
        }

        // Add final vertices and faces based on reference positions

        Mesh m;

        for (unsigned int i = 0; i < n; i += 3)
        {
            // Outer
            XMVECTOR pos0 = XMLoadFloat3(&mReference.m_vertices[mReference.m_indices[i + 0]].m_position);
            XMVECTOR pos1 = XMLoadFloat3(&mReference.m_vertices[mReference.m_indices[i + 1]].m_position);
            XMVECTOR pos2 = XMLoadFloat3(&mReference.m_vertices[mReference.m_indices[i + 2]].m_position);

            // Inner
            XMVECTOR pos3 = XMLoadFloat3(&mReference.m_vertices[mReference.m_indices[n + i + 0]].m_position);
            XMVECTOR pos4 = XMLoadFloat3(&mReference.m_vertices[mReference.m_indices[n + i + 1]].m_position);
            XMVECTOR pos5 = XMLoadFloat3(&mReference.m_vertices[mReference.m_indices[n + i + 2]].m_position);

            add_face(m, pos0, pos1, pos2);
            add_face(m, pos1, pos0, pos3);
            add_face(m, pos3, pos4, pos1);
            add_face(m, pos2, pos1, pos4);
            add_face(m, pos4, pos5, pos2);
            add_face(m, pos0, pos2, pos5);
            add_face(m, pos5, pos3, pos0);
        }

        mesh.insert(m);

        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    string output_filename;

    if (argc < 1)
    {
        output_filename = argv[1];
    }
    else
    {
//        bool b = select_output_file(base_dir, patterns, output_filename);
output_filename = "glass_form_tetra_3.obj";
bool b = true;
        if (!b)
        {
            return 0;
        }
    }

    Mesh mesh;

    create_glass_form_tetra(mesh, 3.0f * 25.4f, .3f);
    write_obj(mesh, output_filename);

    return 0;
}
