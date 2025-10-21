//------------------------------------------------------------------------------
// hex_level
// Application.cpp
//------------------------------------------------------------------------------

#include "Application.hpp"
#include "quetzal/direct3d11/Exception.hpp"
#include "quetzal/direct3d11/file_io.hpp"
#include "quetzal/direct3d11/math_util.hpp"
#include <cassert>

using namespace std;
using namespace DirectX;

namespace
{

    string s_level_pathname = ""; // ...

    string s_vertex_shader_filename("vertex_shader.cso");
    string s_pixel_shader_filename("pixel_shader.cso");

    vector<D3D11_INPUT_ELEMENT_DESC> s_vertex_layout
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    struct Vertex
    {
        XMFLOAT3 position;
        XMFLOAT3 normal;
        XMFLOAT2 texture;
    };

    vector<Vertex> s_vertices =
    {
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
        {{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},

        {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},

        {{-0.5f,  0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},

        {{ 0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}}
    };

    vector<UINT> s_indices =
    {
         0,  1,  2,  3,
        12, 13, 14, 15,
        20, 21, 22, 23,
         8,  9, 10, 11,
        16, 17, 18, 19,
         4,  5,  6,  7
    };

    // constant data separated, try this ...
    struct ConstantDataResize
    {
        XMMATRIX m_m4Projection;
    };

    struct ConstantDataFrame
    {
        XMMATRIX m_m4View;
    };

    struct ConstantDataObject
    {
        XMMATRIX m_m4World;
    };

    // stack variables ...
    ConstantDataResize s_constant_data_resize {};
    ConstantDataFrame s_constant_data_frame {};
    ConstantDataObject s_constant_data_object {};

    struct ConstantData
    {
        XMMATRIX m_m4World;
        XMMATRIX m_m4View;
        XMMATRIX m_m4Projection;
    };

    ConstantData s_constant_data {};

} // namespace

//------------------------------------------------------------------------------
quetzal::hex_level::Application::Application(HINSTANCE hinstance, const string& caption) :
    quetzal::direct3d11::Application(hinstance, caption),
    m_pVertexShader(),
    m_pPixelShader(),
    m_pInputLayout(),
    m_pBufferVertex(),
    m_pBufferIndex(),
    m_pBufferConstant(),
    m_pShaderResourceView(),
    m_pSamplerState(),
    m_m4World(),
    m_up(XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f)),
    m_camera(),
    m_speed(6.0f),
    m_posMouse(),
    m_tFrame(),
    m_t(),
    m_level(),
    m_statistics()
{
    load_level(s_level_pathname);

    init_geometry();
    m_camera.look_at(XMVectorSet(12.0f, 12.0f, -12.0f, 0.0f), XMVectorSet(0.0f, 0.0f, -2.0f, 0.0f), m_up),
    init_projection();
}

//------------------------------------------------------------------------------
void quetzal::hex_level::Application::update(float dt)
{
    check_input(dt);

    m_camera.update_view();

    m_statistics.update(dt);

    return;
}

//------------------------------------------------------------------------------
void quetzal::hex_level::Application::render()
{
    m_device.clear();

    draw_level();

//    m_statistics.text(); ...

    m_device.present();
    return;
}

//------------------------------------------------------------------------------
void quetzal::hex_level::Application::on_size(direct3d11::Window::size_type size)
{
    size;
    init_projection();
    return;
}

//------------------------------------------------------------------------------
void quetzal::hex_level::Application::on_key_down(Timestamp ts, input::key::Modifiers modifiers, input::key::Code code)
{
    ts;
    modifiers;

    switch (code)
    {
        case input::key::Code::V_ESCAPE:
            m_window.destroy();
            break;

/*
        case input::key::Code::V_F:
            if (fullscreen())
            {
                set_windowed();
            }
            else
            {
                set_fullscreen();
            }
            break;

        case input::key::Code::V_I:
            if (solid())
            {
                set_wireframe();
            }
            else
            {
                set_solid();
            }
            break;
*/
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::hex_level::Application::on_mouse_down(Timestamp ts, quetzal::input::key::Modifiers modifiers, quetzal::direct3d11::Window::pos_type position, input::key::Code code)
{
    ts;
    modifiers;
    code;

    m_posMouse.x = position.x;
    m_posMouse.y = position.y;

//    SetCapture(m_window.hwnd());
    return;
}

//------------------------------------------------------------------------------
void quetzal::hex_level::Application::on_mouse_up(Timestamp ts, quetzal::input::key::Modifiers modifiers, quetzal::direct3d11::Window::pos_type position, input::key::Code code)
{
    ts;
    modifiers;
    position;
    code;

//    ReleaseCapture();
    return;
}

//------------------------------------------------------------------------------
void quetzal::hex_level::Application::on_mouse_move(Timestamp ts, input::key::Modifiers modifiers, direct3d11::Window::pos_type position)
{
    ts;
    modifiers;

    float rx = XMConvertToRadians(0.25f * static_cast<float>(position.x - m_posMouse.x));
    float ry = XMConvertToRadians(0.25f * static_cast<float>(position.y - m_posMouse.y));

    // Camera movement
    m_camera.pan(rx);
//    m_camera.pan(rx, m_up);
    m_camera.tilt(ry);
    m_camera.update_view();

    m_posMouse.x = position.x;
    m_posMouse.y = position.y;
    return;
}

//------------------------------------------------------------------------------
void quetzal::hex_level::Application::init_geometry()
{
    HRESULT hr = S_OK;

    // Load and create the vertex shader
    vector<char> shader_data = direct3d11::read(s_vertex_shader_filename);
    D3D11_CHECK(m_device.device()->CreateVertexShader(shader_data.data(), shader_data.size(), nullptr, &m_pVertexShader.p()));

    // Create the vertex input layout
    D3D11_CHECK(m_device.device()->CreateInputLayout(s_vertex_layout.data(), s_vertex_layout.size(), shader_data.data(), shader_data.size(), &m_pInputLayout.p()));

    // Load and create the pixel shader
    shader_data = direct3d11::read(s_pixel_shader_filename);
    D3D11_CHECK(m_device.device()->CreatePixelShader(shader_data.data(), shader_data.size(), nullptr, &m_pPixelShader.p()));

    // Create vertex buffer
    D3D11_BUFFER_DESC vertex_buffer_desc {sizeof s_vertices[0] * s_vertices.size(), D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0};
    D3D11_SUBRESOURCE_DATA vertex_data {s_vertices.data()};
    D3D11_CHECK(m_device.device()->CreateBuffer(&vertex_buffer_desc, &vertex_data, &m_pBufferVertex.p()));

    // Create the index buffer
    D3D11_BUFFER_DESC index_buffer_desc {sizeof s_indices[0] * s_indices.size(), D3D11_USAGE_DEFAULT, D3D11_BIND_INDEX_BUFFER, 0, 0, 0};
    D3D11_SUBRESOURCE_DATA index_data {s_indices.data()};
    D3D11_CHECK(m_device.device()->CreateBuffer(&index_buffer_desc, &index_data, &m_pBufferIndex.p()));

    // Create the constant buffer
    D3D11_BUFFER_DESC constant_buffer_desc {sizeof s_constant_data, D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0};
    D3D11_CHECK(m_device.device()->CreateBuffer(&constant_buffer_desc, nullptr, &m_pBufferConstant.p()));
    return;
}

//------------------------------------------------------------------------------
void quetzal::hex_level::Application::init_projection()
{
    m_camera.set_lens(m_window.aspect_ratio(), 1.0f, 1000.0f);
    return;
}

//------------------------------------------------------------------------------
void quetzal::hex_level::Application::check_input(float dt)
{
    if (input::query_key_down(input::key::Code::V_W))
    {
        m_camera.set_position(XMVectorAdd(m_camera.position(), XMVectorScale(m_camera.forward(), dt * m_speed)));
    }

    if (input::query_key_down(input::key::Code::V_S))
    {
        m_camera.set_position(XMVectorAdd(m_camera.position(), XMVectorScale(m_camera.forward(), -dt * m_speed)));
    }

    if (input::query_key_down(input::key::Code::V_A))
    {
        m_camera.set_position(XMVectorAdd(m_camera.position(), XMVectorScale(m_camera.right(), -dt * m_speed)));
    }

    if (input::query_key_down(input::key::Code::V_D))
    {
        m_camera.set_position(XMVectorAdd(m_camera.position(), XMVectorScale(m_camera.right(), dt * m_speed)));
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::hex_level::Application::load_level(const string& filename)
{
    filename; // ...
    return;
}

//------------------------------------------------------------------------------
void quetzal::hex_level::Application::draw_level()
{
    ID3D11DeviceContext* pContext = m_device.context();
    assert(pContext);

    pContext->IASetInputLayout(m_pInputLayout);
    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    UINT stride = sizeof (Vertex);
    UINT offset = 0;
    ID3D11Buffer* pBuffers[] = {m_pBufferVertex.get()};
    pContext->IASetVertexBuffers(0, 1, pBuffers, &stride, &offset);
    pContext->IASetIndexBuffer(m_pBufferIndex, DXGI_FORMAT_R32_UINT, 0);

    for (auto& cell : m_level.cells())
    {
        draw_cell(cell);
    }

//    m_statistics.set_vertices(s_vertices.size());
//    m_statistics.set_triangles(cubearray.width() * cubearray.height() * s_indices.size() / 3);
m_statistics.set_vertices(28 * 24);
m_statistics.set_triangles(28 * 12);

    return;
}

//------------------------------------------------------------------------------
void quetzal::hex_level::Application::draw_cell(const Cell& cell)
{
    ID3D11DeviceContext* pContext = m_device.context();
    assert(pContext);

/*
    for (int i = 0; i < 6; ++i)
    {
        // Set constant data
        s_constant_data.m_m4World = XMMatrixTranspose(m_m4World);
        s_constant_data.m_m4View = XMMatrixTranspose(m_m4View);
        s_constant_data.m_m4Projection = XMMatrixTranspose(m_m4Projection);
        pContext->UpdateSubresource(m_pBufferConstant, 0, nullptr, &s_constant_data, 0, 0);

        // Vertex shader
        pContext->VSSetShader(m_pVertexShader, nullptr, 0);
        ID3D11Buffer* pBuffer = m_pBufferConstant.get();
        pContext->VSSetConstantBuffers(0, 1, &pBuffer);

        // Pixel shader
        pContext->PSSetShader(m_pPixelShader, nullptr, 0);
        pContext->PSSetConstantBuffers(0, 1, &pBuffer);

        pContext->DrawIndexed(4, i * 4, 0);
    }
*/

    // Vertex shader
    pContext->VSSetShader(m_pVertexShader, nullptr, 0);
    ID3D11Buffer* pBuffers[] = {m_pBufferConstant.get()};
    pContext->VSSetConstantBuffers(0, 1, pBuffers);

    // Pixel shader
    pContext->PSSetShader(m_pPixelShader, nullptr, 0);
    pContext->PSSetConstantBuffers(0, 1, pBuffers); // ...

    // Texture
    ID3D11ShaderResourceView* pResourceViews[] = {m_pShaderResourceView.get()};
    pContext->PSSetShaderResources(0, 1, pResourceViews);
    ID3D11SamplerState* pSamplerStates[] = {m_pSamplerState.get()};  
    pContext->PSSetSamplers(0, 1, pSamplerStates);

    for (auto& wall : cell.walls())
    {
        draw_wall(wall);
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::hex_level::Application::draw_wall(const Wall& wall)
{
    assert(wall.cell() != nullptr);
    assert(wall.cell()->level() != nullptr);

    ID3D11DeviceContext* pContext = m_device.context();
    assert(pContext);

    // rotate normal into position first ...

    Position position(wall.position());
    position += wall.cell()->position();
    position += wall.cell()->level()->position();
    m_m4World = XMMatrixTranslation(position.x(), position.y(), position.z());

    ConstantData constant_data {XMMatrixTranspose(m_m4World), XMMatrixTranspose(m_camera.view()), XMMatrixTranspose(m_camera.projection())};
    pContext->UpdateSubresource(m_pBufferConstant, 0, nullptr, &constant_data, 0, 0);

    pContext->DrawIndexed(4, i * 4, 0);

    return;
}
