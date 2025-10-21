//------------------------------------------------------------------------------
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/direct3d11/Mesh.hpp"
#include "quetzal/direct3d11/obj_util.hpp"
#include "quetzal/direct3d11/math_util.hpp"
#include <iostream>
#include <iomanip>
#include <cassert>

using namespace std;
using namespace DirectX;

namespace
{

    //--------------------------------------------------------------------------
    XMMATRIX texture_position_basis_matrix(XMFLOAT3 pos0, XMFLOAT3 pos1, XMFLOAT3 pos2, XMFLOAT2 tex0, XMFLOAT2 tex1, XMFLOAT2 tex2)
    {
        float x0 = pos0.x;
        float y0 = pos0.y;
        float z0 = pos0.z;
        float x1 = pos1.x;
        float y1 = pos1.y;
        float z1 = pos1.z;
        float x2 = pos2.x;
        float y2 = pos2.y;
        float z2 = pos2.z;

        float u0 = tex0.x;
        float v0 = tex0.y;
        float u1 = tex1.x;
        float v1 = tex1.y;
        float u2 = tex2.x;
        float v2 = tex2.y;

        float a = -(v1 * x0 - v2 * x0 - v0 * x1 + v2 * x1 + v0 * x2 - v1 * x2) / (u1 * v0 - u2 * v0 - u0 * v1 + u2 * v1 + u0 * v2 - u1 * v2);
        float b = -(-u1 * x0 + u2 * x0 + u0 * x1 - u2 * x1 - u0 * x2 + u1 * x2) / (u1 * v0 - u2 * v0 - u0 * v1 + u2 * v1 + u0 * v2 - u1 * v2);
        float c = -(-u2 * v1 * x0 + u1 * v2 * x0 + u2 * v0 * x1 - u0 * v2 * x1 - u1 * v0 * x2 + u0 * v1 * x2) / (u1 * v0 - u2 * v0 - u0 * v1 + u2 * v1 + u0 * v2 - u1 * v2);
        float d = -(v1 * y0 - v2 * y0 - v0 * y1 + v2 * y1 + v0 * y2 - v1 * y2) / (u1 * v0 - u2 * v0 - u0 * v1 + u2 * v1 + u0 * v2 - u1 * v2);
        float e = -(-u1 * y0 + u2 * y0 + u0 * y1 - u2 * y1 - u0 * y2 + u1 * y2) / (u1 * v0 - u2 * v0 - u0 * v1 + u2 * v1 + u0 * v2 - u1 * v2);
        float f = -(-u2 * v1 * y0 + u1 * v2 * y0 + u2 * v0 * y1 - u0 * v2 * y1 - u1 * v0 * y2 + u0 * v1 * y2) / (u1 * v0 - u2 * v0 - u0 * v1 + u2 * v1 + u0 * v2 - u1 * v2);
        float g = -(v1 * z0 - v2 * z0 - v0 * z1 + v2 * z1 + v0 * z2 - v1 * z2) / (u1 * v0 - u2 * v0 - u0 * v1 + u2 * v1 + u0 * v2 - u1 * v2);
        float h = -(-u1 * z0 + u2 * z0 + u0 * z1 - u2 * z1 - u0 * z2 + u1 * z2) / (u1 * v0 - u2 * v0 - u0 * v1 + u2 * v1 + u0 * v2 - u1 * v2);
        float i = -(-u2 * v1 * z0 + u1 * v2 * z0 + u2 * v0 * z1 - u0 * v2 * z1 - u1 * v0 * z2 + u0 * v1 * z2) / (u1 * v0 - u2 * v0 - u0 * v1 + u2 * v1 + u0 * v2 - u1 * v2);

        return {{a, d, g, 0.0f}, {b, e, h, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}, {c, f, i, 1.0f}};
    }

    //--------------------------------------------------------------------------
    ostream& operator<<(ostream& os, const XMFLOAT3& f3)
    {
        os << "[" << fixed << setprecision((2)) << f3.x << " " << fixed << setprecision((2)) << f3.y << " " << fixed << setprecision((2)) << f3.z << "]";
        return os;
    }

    //--------------------------------------------------------------------------
    ostream& operator<<(ostream& os, const XMFLOAT2& f2)
    {
        os << "[" << fixed << setprecision((2)) << f2.x << " " << fixed << setprecision((2)) << f2.y << " " << fixed << setprecision((2)) << 0.0f << "]";
        return os;
    }

    //--------------------------------------------------------------------------
    ostream& operator<<(ostream& os, const XMVECTOR& v)
    {
        os << "[" << fixed << setprecision((2)) << XMVectorGetX(v) << " " << fixed << setprecision((2)) << XMVectorGetY(v) << " " << fixed << setprecision((2)) << XMVectorGetZ(v) << " " << fixed << setprecision((2)) << XMVectorGetW(v) << "]";
        return os;
    }

    //--------------------------------------------------------------------------
    ostream& operator<<(ostream& os, const XMMATRIX& m)
    {
        os << "|" << fixed << setprecision((2)) << XMVectorGetX(m.r[0]) << " " << fixed << setprecision((2)) << XMVectorGetY(m.r[0]) << " " << fixed << setprecision((2)) << XMVectorGetZ(m.r[0]) << " " << fixed << setprecision((2)) << XMVectorGetW(m.r[0]) << "|" << endl;
        os << "|" << fixed << setprecision((2)) << XMVectorGetX(m.r[1]) << " " << fixed << setprecision((2)) << XMVectorGetY(m.r[1]) << " " << fixed << setprecision((2)) << XMVectorGetZ(m.r[1]) << " " << fixed << setprecision((2)) << XMVectorGetW(m.r[1]) << "|" << endl;
        os << "|" << fixed << setprecision((2)) << XMVectorGetX(m.r[2]) << " " << fixed << setprecision((2)) << XMVectorGetY(m.r[2]) << " " << fixed << setprecision((2)) << XMVectorGetZ(m.r[2]) << " " << fixed << setprecision((2)) << XMVectorGetW(m.r[2]) << "|" << endl;
        os << "|" << fixed << setprecision((2)) << XMVectorGetX(m.r[3]) << " " << fixed << setprecision((2)) << XMVectorGetY(m.r[3]) << " " << fixed << setprecision((2)) << XMVectorGetZ(m.r[3]) << " " << fixed << setprecision((2)) << XMVectorGetW(m.r[3]) << "|" << endl;
        return os;
    }

} // namespace        

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    string input_filename = "E:\\Users\\Q\\User\\Keith\\dev\\oneshot\\spikosahedron\\spikosahedron.obj";

    quetzal::direct3d11::Mesh mesh;
    bool b = read_obj(mesh, input_filename);
    assert(b);

    for (unsigned int i = 0; i < mesh.m_indices.size(); i += 3)
    {
        XMFLOAT3 pos0 = mesh.m_vertices[mesh.m_indices[i + 0]].m_position;
        XMFLOAT3 pos1 = mesh.m_vertices[mesh.m_indices[i + 1]].m_position;
        XMFLOAT3 pos2 = mesh.m_vertices[mesh.m_indices[i + 2]].m_position;

        XMFLOAT2 tex0 = mesh.m_vertices[mesh.m_indices[i + 0]].m_texture;
        XMFLOAT2 tex1 = mesh.m_vertices[mesh.m_indices[i + 1]].m_texture;
        XMFLOAT2 tex2 = mesh.m_vertices[mesh.m_indices[i + 2]].m_texture;

        XMMATRIX m = texture_position_basis_matrix(pos0, pos1, pos2, tex0, tex1, tex2);

        XMVECTOR v0 = XMVector3Transform({tex0.x, tex0.y, 0.0f, 1.0f}, m);
        XMVECTOR v1 = XMVector3Transform({tex1.x, tex1.y, 0.0f, 1.0f}, m);
        XMVECTOR v2 = XMVector3Transform({tex2.x, tex2.y, 0.0f, 1.0f}, m);

        cout << endl;
        cout << m;
        cout << tex0 << "\t" << tex1 << "\t" << tex2 << endl;
        cout << pos0 << "\t" << pos1 << "\t" << pos2 << endl;
        cout << v0 << "\t" << v1 << "\t" << v2 << endl;
    }

    return 0;
}