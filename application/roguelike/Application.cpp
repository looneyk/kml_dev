//------------------------------------------------------------------------------
// roguelike
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

    struct ConstantData // Per object
    {
        XMMATRIX m_m4WorldViewProjection;
        XMMATRIX m_m4World;
    };

    ConstantData s_constant_data {};

} // namespace

//------------------------------------------------------------------------------
quetzal::roguelike::Application::Application(HINSTANCE hinstance, const string& caption) :
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
    m_camera(),
    m_up(XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f)),
    m_fov(direct3d11::math::Pi / 3.0f),
    m_speed(0.5f),
    m_tFrame(),
    m_t(),
    m_statistics()
{
    init_display();

    init_geometry();
    init_view();
    init_projection();
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Application::update(float dt)
{
    check_input(dt);

    m_camera.update_view();

    m_t += dt;

    // ...

    m_statistics.update(dt);

    return;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Application::render()
{
    assert(m_pBufferConstant);

    ID3D11DeviceContext* pContext = m_device.context();
    assert(pContext);

    // Bind the render target view and depth/stencil view to the output pipeline
// move to device? ...
    ID3D11RenderTargetView* pRenderTargetView = m_device.render_target();
    pContext->OMSetRenderTargets(1, &pRenderTargetView, m_device.depth_stencil());

    m_device.clear();

    pContext->IASetInputLayout(m_pInputLayout);
    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    UINT stride = sizeof (Vertex);
    UINT offset = 0;
    ID3D11Buffer* pBuffers[] = {m_pBufferVertex.get()};
    pContext->IASetVertexBuffers(0, 1, pBuffers, &stride, &offset);
    pContext->IASetIndexBuffer(m_pBufferIndex, DXGI_FORMAT_R32_UINT, 0);

    draw_level();

//    m_statistics.text(); ...

    m_device.present();
    return;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Application::on_size(direct3d11::Window::size_type size)
{
    size;
    init_projection();
    return;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Application::on_key_down(Timestamp ts, input::key::Modifiers modifiers, input::key::Code code)
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

        case input::key::Code::V_C:
            m_bCycle = !m_bCycle;
            break;

        case input::key::Code::V_M:
            m_bSmooth = !m_bSmooth;
            break;
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Application::on_mouse_down(Timestamp ts, quetzal::input::key::Modifiers modifiers, quetzal::direct3d11::Window::pos_type position, input::key::Code code)
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
void quetzal::roguelike::Application::on_mouse_up(Timestamp ts, quetzal::input::key::Modifiers modifiers, quetzal::direct3d11::Window::pos_type position, input::key::Code code)
{
    ts;
    modifiers;
    position;
    code;

//    ReleaseCapture();
    return;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Application::on_mouse_move(Timestamp ts, input::key::Modifiers modifiers, direct3d11::Window::pos_type position)
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
void quetzal::roguelike::Application::init_geometry()
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


// ...
    // Load the raw texture data from disk and construct a subresource description that references it.
    auto texture_data = direct3d11::read("texturedata.bin");
    D3D11_SUBRESOURCE_DATA texture_subresource_data = {};
    texture_subresource_data.pSysMem = texture_data.data();
    texture_subresource_data.SysMemPitch = 1024; // Specify the size of a row in bytes, known a priori about the texture data.
    texture_subresource_data.SysMemSlicePitch = 0; // As this is not a texture array or 3D texture, this parameter is ignored.

    // Create a texture description from information known a priori about the data.
    // Generalized texture loading code can be found in the Resource Loading sample.
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = 256;
    textureDesc.Height = 256;
    textureDesc.MipLevels = 1; // Most textures contain more than one MIP level.  For simplicity, this sample uses only one.
    textureDesc.ArraySize = 1; // As this will not be a texture array, this parameter is ignored.
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1; // Don't use multi-sampling.
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE; // Allow the texture to be bound as a shader resource.
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    com_ptr<ID3D11Texture2D> pTexture2D;
    D3D11_CHECK(m_device.device()->CreateTexture2D(&textureDesc, &texture_subresource_data, &pTexture2D.p()));

    // Once the texture is created, we must create a shader resource view of it so that shaders may use it.
    // In general, the view description will match the texture description.
    D3D11_SHADER_RESOURCE_VIEW_DESC textureViewDesc = {};
    textureViewDesc.Format = textureDesc.Format;
    textureViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    textureViewDesc.Texture2D.MostDetailedMip = 0;
    textureViewDesc.Texture2D.MipLevels = textureDesc.MipLevels;

    D3D11_CHECK(m_device.device()->CreateShaderResourceView(pTexture2D, &textureViewDesc, &m_pShaderResourceView.p()));

    D3D11_SAMPLER_DESC samplerDesc = {}; // Once the texture view is created, create a sampler. This defines how the color for a particular texture coordinate is determined using the relevant texture data.
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    // Specify how texture coordinates outside of the range 0..1 are resolved.
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    // Use no special MIP clamping or bias.
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    samplerDesc.MaxAnisotropy = 0; // The sampler does not use anisotropic filtering, so this parameter is ignored.
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER; // Don't use a comparison function.
    // samplerDesc.BorderColor not used

    D3D11_CHECK(m_device.device()->CreateSamplerState(&samplerDesc, &m_pSamplerState.p()));

	return;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Application::init_projection()
{
    m_camera.set_lens(m_window.aspect_ratio(), 1.0f, 1000.0f);
    return;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Application::check_input(float dt)
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
void quetzal::roguelike::Application::draw_level()
{
    for (each cell)
    {
        Cell& cell = ...;

        draw_cell(cell, ...);
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Application::draw_cell(const Cell& cell, float x, float y, float z)
{
    for (each wall)
    {
        Wall& wall = ...;

        calculate transformation matrix
        set vertices
        set indices
        set texture

        draw_wall(wall, ...);
    }

    for (contents)
    {
        draw_item(...);
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::roguelike::Application::draw_wall(const Wall& wall, XMMatrix m4World)
{
    ID3D11DeviceContext* pContext = m_device.context();

    m_m4World = XMMatrixTranslation(x, y, z);

/*
    for (int i = 0; i < 6; ++i)
    {
        // Set constant data
        s_constant_data.m_m4WorldViewProjection = XMMatrixTranspose(m_m4World * m_camera.view_projection());
        s_constant_data.m_m4World = XMMatrixTranspose(m_m4World);
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

    for (int i = 0; i < 6; ++i)
    {
        ConstantData constant_data {XMMatrixTranspose(m_m4World), XMMatrixTranspose(m_camera.view()), XMMatrixTranspose(m_camera.projection())};
        pContext->UpdateSubresource(m_pBufferConstant, 0, nullptr, &constant_data, 0, 0);

        pContext->DrawIndexed(4, i * 4, 0);
    }

    return;
}
