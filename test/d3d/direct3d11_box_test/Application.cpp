//------------------------------------------------------------------------------
// Application.cpp
//------------------------------------------------------------------------------

#include "Application.hpp"
#include "quetzal/direct3d11/Exception.hpp"
#include "quetzal/direct3d11/file_io.hpp"
#include "quetzal/direct3d11/math_util.hpp"
#include <DirectXColors.h>
#include <cassert>

using namespace std;

namespace
{

    string s_vertex_shader_filename("vertex_shader.cso");
    string s_pixel_shader_filename("pixel_shader.cso");

    vector<D3D11_INPUT_ELEMENT_DESC> s_vertex_layout
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    struct Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT4 color;
    };

    vector<Vertex> s_vertices =
    {
        {DirectX::XMFLOAT3(-1.f, -1.f, -1.f), static_cast<const DirectX::XMFLOAT4>(DirectX::Colors::White)},
        {DirectX::XMFLOAT3(-1.f, +1.f, -1.f), static_cast<const DirectX::XMFLOAT4>(DirectX::Colors::Black)},
        {DirectX::XMFLOAT3(+1.f, +1.f, -1.f), static_cast<const DirectX::XMFLOAT4>(DirectX::Colors::Red)},
        {DirectX::XMFLOAT3(+1.f, -1.f, -1.f), static_cast<const DirectX::XMFLOAT4>(DirectX::Colors::Green)},
        {DirectX::XMFLOAT3(-1.f, -1.f, +1.f), static_cast<const DirectX::XMFLOAT4>(DirectX::Colors::Blue)},
        {DirectX::XMFLOAT3(-1.f, +1.f, +1.f), static_cast<const DirectX::XMFLOAT4>(DirectX::Colors::Yellow)},
        {DirectX::XMFLOAT3(+1.f, +1.f, +1.f), static_cast<const DirectX::XMFLOAT4>(DirectX::Colors::Cyan)},
        {DirectX::XMFLOAT3(+1.f, -1.f, +1.f), static_cast<const DirectX::XMFLOAT4>(DirectX::Colors::Magenta)}
    };

    vector<UINT> s_indices =
    {
        // front face
        0, 1, 2,
        0, 2, 3,

        // back face
        4, 6, 5,
        4, 7, 6,

        // left face
        4, 5, 1,
        4, 1, 0,

        // right face
        3, 2, 6,
        3, 6, 7,

        // top face
        1, 5, 6,
        1, 6, 2,

        // bottom face
        4, 0, 3,
        4, 3, 7
    };

    struct ConstantData
    {
        DirectX::XMMATRIX m_matrixWVP;
    };

    ConstantData s_constant_data {};

} // namespace

//------------------------------------------------------------------------------
quetzal::direct3d11_box::Application::Application(HINSTANCE hinstance, const string& caption) :
    direct3d11::Application(hinstance, caption),
    m_pShaderVertex(),
    m_pShaderPixel(),
    m_pInputLayout(),
    m_pBufferVertex(),
    m_pBufferIndex(),
    m_pBufferConstant(),
    m_m4World(),
    m_m4View(),
    m_m4Projection(),
    m_theta(1.5f * MathHelper::Pi),
    m_phi(.25f * MathHelper::Pi),
    m_radius(5.f),
    m_posMousePrev{},
    m_button_pressed(),
    m_button_code()
{
    m_m4World = DirectX::XMMatrixIdentity();

    init_shaders();
    init_geometry();
    init_view();
    init_projection();
}

//------------------------------------------------------------------------------
void quetzal::direct3d11_box::Application::update(float dt)
{
    dt; // ...

    init_view();

    return;
}

//------------------------------------------------------------------------------
void quetzal::direct3d11_box::Application::render()
{
    assert(m_pBufferConstant);

    ID3D11DeviceContext* pContext = m_device.context();

//    // Bind the render target view and depth/stencil view to the output pipeline
    ID3D11RenderTargetView* pRenderTargetView = m_device.render_target();
    pContext->OMSetRenderTargets(1, &pRenderTargetView, m_device.depth_stencil());

    pContext->ClearRenderTargetView(m_device.render_target(), reinterpret_cast<const float*>(&DirectX::Colors::Black));
    pContext->ClearDepthStencilView(m_device.depth_stencil(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

// these with actual drawing code? In any case, not here ...
//    pContext->IASetInputLayout(m_pInputLayout);
//    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    draw_box();

    m_device.present();
    return;
}

//------------------------------------------------------------------------------
void quetzal::direct3d11_box::Application::on_size(direct3d11::Window::size_type size)
{
    direct3d11::Application::on_size(size);
    init_projection();
    return;
}

//------------------------------------------------------------------------------
void quetzal::direct3d11_box::Application::on_key_down(Timestamp ts, input::key::Modifiers modifiers, input::key::Code code)
{
    ts;
    modifiers;

    if (code == input::key::Code::V_ESCAPE)
    {
        m_window.destroy();
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::direct3d11_box::Application::on_mouse_down(Timestamp ts, input::key::Modifiers modifiers, direct3d11::Window::pos_type position, input::key::Code code)
{
    ts;
    modifiers;

    m_button_pressed = true;
    m_button_code = code;
    m_posMousePrev = position;
    SetCapture(m_window.hwnd());
    return;
}

//------------------------------------------------------------------------------
void quetzal::direct3d11_box::Application::on_mouse_up(Timestamp ts, input::key::Modifiers modifiers, direct3d11::Window::pos_type position, input::key::Code code)
{
    ts;
    modifiers;
    position;
    code;

    m_button_pressed = false;
    ReleaseCapture();
    return;
}

//------------------------------------------------------------------------------
void quetzal::direct3d11_box::Application::on_mouse_move(Timestamp ts, input::key::Modifiers modifiers, direct3d11::Window::pos_type position)
{
    ts;
    modifiers;

    // just check button state here insead of caching code? ...
    if (m_button_code == input::key::Code::V_LBUTTON)
    {
        // Make each pixel correspond to a quarter of a degree.
        float dx = DirectX::XMConvertToRadians(.25f * static_cast<float>(position.x - m_posMousePrev.x));
        float dy = DirectX::XMConvertToRadians(.25f * static_cast<float>(position.y - m_posMousePrev.y));

        // Update angles based on input to orbit camera around box.
        m_theta += dx;
        m_phi += dy;

        // Restrict the angle m_phi
        m_phi = MathHelper::Clamp(m_phi, 0.1f, MathHelper::Pi - .1f);
    }
    else if (m_button_code == input::key::Code::V_RBUTTON)
    {
        // Make each pixel correspond to 0.005 unit in the scene.
        float dx = .005f * static_cast<float>(position.x - m_posMousePrev.x);
        float dy = .005f * static_cast<float>(position.y - m_posMousePrev.y);

        // Update the camera radius based on input
        m_radius += dx - dy;
        m_radius = MathHelper::Clamp(m_radius, 3.f, 15.f);
    }

    m_posMousePrev = position;
    return;
}

//------------------------------------------------------------------------------
void quetzal::direct3d11_box::Application::init_geometry()
{
    HRESULT hr = S_OK;

    ID3D11DeviceContext* pContext = m_device.context();

    // Load and create the vertex shader
    vector<char> shader_data = direct3d11::read(s_vertex_shader_filename);
    D3D11_CHECK(m_device.device()->CreateVertexShader(shader_data.data(), shader_data.size(), nullptr, &m_pShaderVertex.p()));

    // Create the vertex input layout
    D3D11_CHECK(m_device.device()->CreateInputLayout(s_vertex_layout.data(), s_vertex_layout.size(), shader_data.data(), shader_data.size(), &m_pInputLayout.p()));
    m_device.context()->IASetInputLayout(m_pInputLayout);

    // Load and create the pixel shader
    shader_data = direct3d11::read(s_pixel_shader_filename);
    D3D11_CHECK(m_device.device()->CreatePixelShader(shader_data.data(), shader_data.size(), nullptr, &m_pShaderPixel.p()));

    // Create vertex buffer
    D3D11_BUFFER_DESC vertex_buffer_desc {sizeof s_vertices[0] * s_vertices.size(), D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0};
    D3D11_SUBRESOURCE_DATA vertex_data {s_vertices.data()};
    D3D11_CHECK(m_device.device()->CreateBuffer(&vertex_buffer_desc, &vertex_data, &m_pBufferVertex.p()));

    UINT stride = sizeof (Vertex);
    UINT offset = 0;
    ID3D11Buffer* pBuffer = m_pBufferVertex.get();
    pContext->IASetVertexBuffers(0, 1, &pBuffer, &stride, &offset);

    // Create the index buffer
    D3D11_BUFFER_DESC index_buffer_desc {sizeof s_indices[0] * s_indices.size(), D3D11_USAGE_DEFAULT, D3D11_BIND_INDEX_BUFFER, 0, 0, 0};
    D3D11_SUBRESOURCE_DATA index_data {s_indices.data()};
    D3D11_CHECK(m_device.device()->CreateBuffer(&index_buffer_desc, &index_data, &m_pBufferIndex.p()));

    pContext->IASetIndexBuffer(m_pBufferIndex, DXGI_FORMAT_R32_UINT, 0);

    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Create the constant buffer
    D3D11_BUFFER_DESC constant_buffer_desc {sizeof s_constant_data, D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0};
//    D3D11_SUBRESOURCE_DATA constant_data {&s_constant_data};
    D3D11_CHECK(m_device.device()->CreateBuffer(&constant_buffer_desc, nullptr, &m_pBufferConstant.p()));

    return;
}

//------------------------------------------------------------------------------
void quetzal::direct3d11_box::Application::init_view()
{
    // Convert Spherical to Cartesian coordinates.
    float x = m_radius * sinf(m_phi) * cosf(m_theta);
    float z = m_radius * sinf(m_phi) * sinf(m_theta);
    float y = m_radius * cosf(m_phi);

    // Build the view matrix
    DirectX::XMVECTOR pos = DirectX::XMVectorSet(x, y, z, 1.0f);
    DirectX::XMVECTOR target = DirectX::XMVectorZero();
    DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    m_m4View = DirectX::XMMatrixLookAtLH(pos, target, up);
    return;
}

//------------------------------------------------------------------------------
void quetzal::direct3d11_box::Application::init_projection()
{
    m_m4Projection = DirectX::XMMatrixPerspectiveFovLH(.25f * MathHelper::Pi, m_window.aspect_ratio(), 1.f, 5000.f);
    return;
}

//------------------------------------------------------------------------------
void quetzal::direct3d11_box::Application::draw_box()
{
    ID3D11DeviceContext* pContext = m_device.context();

    // Set per object constants
    DirectX::XMMATRIX m = m_m4World * m_m4View * m_m4Projection;
    s_constant_data.m_matrixWVP = XMMatrixTranspose(m);
    pContext->UpdateSubresource(m_pBufferConstant, 0, nullptr, &s_constant_data, 0, 0);

    // Vertex shader
    pContext->VSSetShader(m_pShaderVertex, nullptr, 0);
    ID3D11Buffer* pBuffer = m_pBufferConstant.get();
    pContext->VSSetConstantBuffers(0, 1, &pBuffer);

    // Pixel shader
    pContext->PSSetShader(m_pShaderPixel, nullptr, 0);
    pContext->PSSetConstantBuffers(0, 1, &pBuffer);

    pContext->DrawIndexed(s_indices.size(), 0, 0);
    return;
}
