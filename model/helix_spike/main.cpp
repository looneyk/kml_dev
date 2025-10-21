//------------------------------------------------------------------------------
// helix_spike
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/direct3d11/geometry.hpp"
#include "quetzal/direct3d11/math_util.hpp"
#include "quetzal/direct3d11/obj_util.hpp"
#include <iostream>

using namespace std;
using namespace DirectX;
using namespace quetzal::direct3d11;

namespace
{

    //--------------------------------------------------------------------------
    void create_helix_spike(obj_mesh_type& mesh, float rBase, float dz, unsigned int nAzimuth, unsigned int nz, float rHelix, float helix_twist = DirectX::XM_2PI, float twist = DirectX::XM_PIDIV2, bool bConcave = false)
    {
        bool bDiagonalAlternate = twist < 0.0f ? bConcave : !bConcave;

        geometry_vertices_type vertices;
        geometry_indices_type indices;

        // Center vertex of base
        // Not necessarily {0, 0, 0} ...
        vertices.push_back({{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}});

        geometry_index_type nVertices = static_cast<geometry_index_type>(vertices.size());

        float dnz = dz / nz;
        float dnr = -rBase / nz;
        float dnphi = XM_2PI / nAzimuth;

        float dnr_helix = -rHelix / nz;
        float dnphi_helix = helix_twist / nz;

        for (unsigned int j = 0; j < nz; ++j)
        {
            float r = rBase + j * dnr;
            float z = j * dnz;

            for (unsigned int i = 0; i < nAzimuth; ++i)
            {
                float phi = i * dnphi;

                XMFLOAT3 position = {r * cos(phi), r * sin(phi), 0.0f}; // Start with triangle in xy-plane
                vertices.push_back({position, {}, {}});
            }

            nVertices = static_cast<geometry_index_type>(vertices.size());

            // Rotate vertices (twist)
            float phiLevel = twist * j / nz;
            XMMATRIX matrix = XMMatrixRotationNormal({0.0f, 0.0f, 1.0f}, phiLevel);

            // Orient along helix
            float r_helix = rHelix + j * dnr_helix;
            float phi_helix = j * dnphi_helix;
            XMVECTOR position_helix = {r_helix * cos(phi_helix), r_helix * sin(phi_helix), z};
            XMVECTOR direction_helix = XMVector3Normalize({r_helix * -sin(phi_helix), r_helix * cos(phi_helix), dz * DirectX::XM_2PI / helix_twist}); // * 2pi? ...

            // Rotate vertices (helix_twist)
            XMVECTOR axis = XMVector3Normalize(XMVector3Cross({0.0f, 0.0f, 1.0f}, direction_helix));
            float angle = math::rotation_angle_normal({0.0f, 0.0f, 1.0f}, direction_helix);
            matrix = XMMatrixMultiply(matrix, XMMatrixRotationNormal(axis, angle));

            // Translate to helix position
            matrix = XMMatrixMultiply(matrix, XMMatrixTranslationFromVector(position_helix));

            // Apply final transformation matrix
            XMStoreFloat3(&vertices[nVertices - 1].m_position, XMVector3TransformCoord(XMLoadFloat3(&vertices[nVertices - 1].m_position), matrix));
            XMStoreFloat3(&vertices[nVertices - 2].m_position, XMVector3TransformCoord(XMLoadFloat3(&vertices[nVertices - 2].m_position), matrix));
            XMStoreFloat3(&vertices[nVertices - 3].m_position, XMVector3TransformCoord(XMLoadFloat3(&vertices[nVertices - 3].m_position), matrix));

            // Flatten vertices with negative z
            // (Better to do this gradually, interpolate along helix from position to base, rather than this brute force approach)

            if (vertices[nVertices - 1].m_position.z < 0.0f)
            {
                vertices[nVertices - 1].m_position.z = 0.0f;
            }

            if (vertices[nVertices - 2].m_position.z < 0.0f)
            {
                vertices[nVertices - 2].m_position.z = 0.0f;
            }

            if (vertices[nVertices - 3].m_position.z < 0.0f)
            {
                vertices[nVertices - 3].m_position.z = 0.0f;
            }

            if (j == 0) // Base level
            {
                // Ensure that all z coordinates are 0 at base level
                vertices[nVertices - 1].m_position.z = 0.0f;
                vertices[nVertices - 2].m_position.z = 0.0f;
                vertices[nVertices - 3].m_position.z = 0.0f;

                // recalculate center point position ...
                // need more complex handling of this since base will be sliced at an angle by xy-plane ...

                indices.insert(indices.end(), {0, static_cast<geometry_index_type>(nVertices - 1), static_cast<geometry_index_type>(nVertices - 2)});
                indices.insert(indices.end(), {0, static_cast<geometry_index_type>(nVertices - 2), static_cast<geometry_index_type>(nVertices - 3)});
                indices.insert(indices.end(), {0, static_cast<geometry_index_type>(nVertices - 3), static_cast<geometry_index_type>(nVertices - 1)});
            }
            else
            {
                if (bDiagonalAlternate)
                {
                    indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 3), static_cast<geometry_index_type>(nVertices - 1), static_cast<geometry_index_type>(nVertices - 6)});
                    indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 3), static_cast<geometry_index_type>(nVertices - 6), static_cast<geometry_index_type>(nVertices - 5)});
                    indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 2), static_cast<geometry_index_type>(nVertices - 3), static_cast<geometry_index_type>(nVertices - 5)});
                    indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 2), static_cast<geometry_index_type>(nVertices - 5), static_cast<geometry_index_type>(nVertices - 4)});
                    indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 1), static_cast<geometry_index_type>(nVertices - 2), static_cast<geometry_index_type>(nVertices - 4)});
                    indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 1), static_cast<geometry_index_type>(nVertices - 4), static_cast<geometry_index_type>(nVertices - 6)});
                }
                else
                {
                    indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 3), static_cast<geometry_index_type>(nVertices - 4), static_cast<geometry_index_type>(nVertices - 6)});
                    indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 3), static_cast<geometry_index_type>(nVertices - 6), static_cast<geometry_index_type>(nVertices - 2)});
                    indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 2), static_cast<geometry_index_type>(nVertices - 6), static_cast<geometry_index_type>(nVertices - 5)});
                    indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 2), static_cast<geometry_index_type>(nVertices - 5), static_cast<geometry_index_type>(nVertices - 1)});
                    indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 1), static_cast<geometry_index_type>(nVertices - 5), static_cast<geometry_index_type>(nVertices - 4)});
                    indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 1), static_cast<geometry_index_type>(nVertices - 4), static_cast<geometry_index_type>(nVertices - 3)});
                }
            }
        }

        // Top level
        vertices.push_back({{0.0f, 0.0f, dz}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}});

        nVertices = static_cast<geometry_index_type>(vertices.size());

        indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 1), static_cast<geometry_index_type>(nVertices - 4), static_cast<geometry_index_type>(nVertices - 3)});
        indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 1), static_cast<geometry_index_type>(nVertices - 3), static_cast<geometry_index_type>(nVertices - 2)});
        indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 1), static_cast<geometry_index_type>(nVertices - 2), static_cast<geometry_index_type>(nVertices - 4)});

        // Create separate vertices and face normals for them

        prepare_face_vertices(vertices, indices);
        calculate_face_normals(vertices, indices);
        mesh.insert(vertices, indices);
        return;
    }

    //--------------------------------------------------------------------------
    // nAzimuth is number of sides
    // offset_factor 1.0 shifts triangles so that all have a vertex on the z-axis
    // instead of z-axis, use line from original base vertex position to {0, 0, dz}, maybe then they will stand up?
    void create_axial_helix_spike(obj_mesh_type& mesh, float rBase, float dz, unsigned int nAzimuth, unsigned int nz, float twist = DirectX::XM_PIDIV2, float offset_factor = 0.0f, bool bConcave = false)
    {
        bool bDiagonalAlternate = twist < 0.0f ? bConcave : !bConcave;

        geometry_vertices_type vertices;
        geometry_indices_type indices;

        // Center vertex of base
        // Not necessarily {0, 0, 0} ...
        vertices.push_back({{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}});

        geometry_index_type nVertices = static_cast<geometry_index_type>(vertices.size());

        float dnz = dz / nz;
        float dnr = -rBase / nz;
        float dnphi = XM_2PI / nAzimuth;

        for (unsigned int j = 0; j < nz; ++j)
        {
            float r = rBase + j * dnr;
            float z = j * dnz;

            for (unsigned int i = 0; i < nAzimuth; ++i)
            {
                float phi = i * dnphi;

                XMFLOAT3 position = {r * cos(phi), r * sin(phi), z};
                vertices.push_back({position, {}, {}});
            }

            nVertices = static_cast<geometry_index_type>(vertices.size());

            // Shift initial vertex position toward z-axis
            geometry_vertex_type& v0 = vertices[nVertices - 3];
            XMFLOAT3 offset = {-v0.m_position.x * offset_factor, -v0.m_position.y * offset_factor, 0.0f};

            vertices[nVertices - 1].m_position = vertices[nVertices - 1].m_position + offset;
            vertices[nVertices - 2].m_position = vertices[nVertices - 2].m_position + offset;
            vertices[nVertices - 3].m_position = vertices[nVertices - 3].m_position + offset;

            // Rotate vertices
            float phiLevel = twist * j / nz;
            XMMATRIX matrix = XMMatrixRotationNormal({0.0f, 0.0f, 1.0f}, phiLevel);

            XMStoreFloat3(&vertices[nVertices - 1].m_position, XMVector3TransformCoord(XMLoadFloat3(&vertices[nVertices - 1].m_position), matrix));
            XMStoreFloat3(&vertices[nVertices - 2].m_position, XMVector3TransformCoord(XMLoadFloat3(&vertices[nVertices - 2].m_position), matrix));
            XMStoreFloat3(&vertices[nVertices - 3].m_position, XMVector3TransformCoord(XMLoadFloat3(&vertices[nVertices - 3].m_position), matrix));

            if (j == 0) // Base level
            {
                // Given base vertex positions, recalculate center point position
                // ...

                indices.insert(indices.end(), {0, static_cast<geometry_index_type>(nVertices - 1), static_cast<geometry_index_type>(nVertices - 2)});
                indices.insert(indices.end(), {0, static_cast<geometry_index_type>(nVertices - 2), static_cast<geometry_index_type>(nVertices - 3)});
                indices.insert(indices.end(), {0, static_cast<geometry_index_type>(nVertices - 3), static_cast<geometry_index_type>(nVertices - 1)});
            }
            else
            {
                if (bDiagonalAlternate)
                {
                    indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 3), static_cast<geometry_index_type>(nVertices - 1), static_cast<geometry_index_type>(nVertices - 6)});
                    indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 3), static_cast<geometry_index_type>(nVertices - 6), static_cast<geometry_index_type>(nVertices - 5)});
                    indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 2), static_cast<geometry_index_type>(nVertices - 3), static_cast<geometry_index_type>(nVertices - 5)});
                    indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 2), static_cast<geometry_index_type>(nVertices - 5), static_cast<geometry_index_type>(nVertices - 4)});
                    indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 1), static_cast<geometry_index_type>(nVertices - 2), static_cast<geometry_index_type>(nVertices - 4)});
                    indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 1), static_cast<geometry_index_type>(nVertices - 4), static_cast<geometry_index_type>(nVertices - 6)});
                }
                else
                {
                    indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 3), static_cast<geometry_index_type>(nVertices - 4), static_cast<geometry_index_type>(nVertices - 6)});
                    indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 3), static_cast<geometry_index_type>(nVertices - 6), static_cast<geometry_index_type>(nVertices - 2)});
                    indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 2), static_cast<geometry_index_type>(nVertices - 6), static_cast<geometry_index_type>(nVertices - 5)});
                    indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 2), static_cast<geometry_index_type>(nVertices - 5), static_cast<geometry_index_type>(nVertices - 1)});
                    indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 1), static_cast<geometry_index_type>(nVertices - 5), static_cast<geometry_index_type>(nVertices - 4)});
                    indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 1), static_cast<geometry_index_type>(nVertices - 4), static_cast<geometry_index_type>(nVertices - 3)});
                }
            }
        }

        // Top level
        vertices.push_back({{0.0f, 0.0f, dz}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}});

        nVertices = static_cast<geometry_index_type>(vertices.size());

        indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 1), static_cast<geometry_index_type>(nVertices - 4), static_cast<geometry_index_type>(nVertices - 3)});
        indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 1), static_cast<geometry_index_type>(nVertices - 3), static_cast<geometry_index_type>(nVertices - 2)});
        indices.insert(indices.end(), {static_cast<geometry_index_type>(nVertices - 1), static_cast<geometry_index_type>(nVertices - 2), static_cast<geometry_index_type>(nVertices - 4)});

        // Create separate vertices and face normals for them

        prepare_face_vertices(vertices, indices);

        calculate_face_normals(vertices, indices);

        mesh.insert(vertices, indices);

        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
//    if (argc < 2)
//    {
//        cout << "Missing output filename" << endl;
//        return 1;
//    }
//
//    string output_filename = argv[1];
    argc;
    argv;

    obj_mesh_type mesh;

    create_axial_helix_spike(mesh, 15.0f, 60.0f, 3, 5, 0.0f, 0.0f);
    write_obj(mesh, "axial_helix_spike_0_0_0.obj");

    mesh.clear();

    create_axial_helix_spike(mesh, 15.0f, 60.0f, 3, 5, 0.0f, 0.0f, true);
    write_obj(mesh, "axial_helix_spike_0_0_1.obj");

    mesh.clear();

    create_axial_helix_spike(mesh, 15.0f, 60.0f, 3, 5, DirectX::XM_PIDIV2, 0.0f);
    write_obj(mesh, "axial_helix_spike_pi2_0_0.obj");

    mesh.clear();

    create_axial_helix_spike(mesh, 15.0f, 60.0f, 3, 5, DirectX::XM_PIDIV2, 0.0f, true);
    write_obj(mesh, "axial_helix_spike_pi2_0_1.obj");

    mesh.clear();

    create_axial_helix_spike(mesh, 15.0f, 60.0f, 3, 5, DirectX::XM_PI, 0.0f);
    write_obj(mesh, "axial_helix_spike_pi_0_0.obj");

    mesh.clear();

    create_axial_helix_spike(mesh, 15.0f, 60.0f, 3, 5, DirectX::XM_PI, 0.0f, true);
    write_obj(mesh, "axial_helix_spike_pi_0_1.obj");

    mesh.clear();

    create_axial_helix_spike(mesh, 15.0f, 60.0f, 3, 5, DirectX::XM_2PI, 0.0f);
    write_obj(mesh, "axial_helix_spike_2pi_0_0.obj");

    mesh.clear();

    create_axial_helix_spike(mesh, 15.0f, 60.0f, 3, 5, DirectX::XM_2PI, 0.0f, true);
    write_obj(mesh, "axial_helix_spike_2pi_0_1.obj");

    mesh.clear();

    create_axial_helix_spike(mesh, 15.0f, 60.0f, 3, 5, DirectX::XM_PIDIV2, 1.0f);
    write_obj(mesh, "axial_helix_spike_pi2_1_0.obj");

    mesh.clear();

    create_axial_helix_spike(mesh, 15.0f, 60.0f, 3, 5, DirectX::XM_PIDIV2, 1.0f, true);
    write_obj(mesh, "axial_helix_spike_pi2_1_1.obj");

    mesh.clear();

    create_axial_helix_spike(mesh, 15.0f, 60.0f, 3, 5, DirectX::XM_PI, 1.0f);
    write_obj(mesh, "axial_helix_spike_pi_1_0.obj");

    mesh.clear();

    create_axial_helix_spike(mesh, 15.0f, 60.0f, 3, 5, DirectX::XM_PI, 1.0f, true);
    write_obj(mesh, "axial_helix_spike_pi_1_1.obj");

    mesh.clear();

    create_axial_helix_spike(mesh, 15.0f, 60.0f, 3, 5, DirectX::XM_2PI, 1.0f);
    write_obj(mesh, "axial_helix_spike_2pi_1_0.obj");

    mesh.clear();

    create_axial_helix_spike(mesh, 15.0f, 60.0f, 3, 5, DirectX::XM_2PI, 1.0f, true);
    write_obj(mesh, "axial_helix_spike_2pi_1_1.obj");

    mesh.clear();

    create_axial_helix_spike(mesh, 15.0f, 60.0f, 3, 5, DirectX::XM_PIDIV2, 2.0f);
    write_obj(mesh, "axial_helix_spike_pi2_2_0.obj");

    mesh.clear();

    create_axial_helix_spike(mesh, 15.0f, 60.0f, 3, 5, DirectX::XM_PIDIV2, 2.0f, true);
    write_obj(mesh, "axial_helix_spike_pi2_2_1.obj");

    mesh.clear();

    create_axial_helix_spike(mesh, 15.0f, 60.0f, 3, 5, DirectX::XM_PI, 2.0f);
    write_obj(mesh, "axial_helix_spike_pi_2_0.obj");

    mesh.clear();

    create_axial_helix_spike(mesh, 15.0f, 60.0f, 3, 5, DirectX::XM_PI, 2.0f, true);
    write_obj(mesh, "axial_helix_spike_pi_2_1.obj");

    mesh.clear();

    create_axial_helix_spike(mesh, 15.0f, 60.0f, 3, 5, DirectX::XM_2PI, 2.0f);
    write_obj(mesh, "axial_helix_spike_2pi_2_0.obj");

    mesh.clear();

    create_axial_helix_spike(mesh, 15.0f, 60.0f, 3, 5, DirectX::XM_2PI, 2.0f, true);
    write_obj(mesh, "axial_helix_spike_2pi_2_1.obj");

    mesh.clear();

    create_axial_helix_spike(mesh, 15.0f, 60.0f, 3, 50, DirectX::XM_PI, 0.0f);
    write_obj(mesh, "axial_helix_spike_pi_0_0_x.obj");

    mesh.clear();

    create_axial_helix_spike(mesh, 15.0f, 60.0f, 3, 50, DirectX::XM_PI, 1.0f);
    write_obj(mesh, "axial_helix_spike_pi_1_0_x.obj");

    mesh.clear();

    create_axial_helix_spike(mesh, 15.0f, 60.0f, 3, 10, DirectX::XM_PI, 1.5f);
    write_obj(mesh, "axial_helix_spike_pi_1x_0_x.obj");

    mesh.clear();

    create_helix_spike(mesh, 15.0f, 60.0f, 3, 10, 10.0f);
    write_obj(mesh, "helix_spike.obj");

    mesh.clear();

    create_helix_spike(mesh, 15.0f, 60.0f, 3, 50, 10.0f, 2.0f * DirectX::XM_2PI, DirectX::XM_2PI);
    write_obj(mesh, "helix_spike.obj");

    return 0;
}
