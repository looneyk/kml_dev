//------------------------------------------------------------------------------
// texture_test
// Application.cpp
//------------------------------------------------------------------------------

#include "Application.hpp"
#include "quetzal/direct3d11/Vertex.hpp"
#include "quetzal/direct3d11/Exception.hpp"
#include "quetzal/direct3d11/file_io.hpp"
#include "quetzal/direct3d11/math_util.hpp"
#include "quetzal/direct3d11/obj_util.hpp"
#include <algorithm>
#include <cassert>

using namespace std;
using namespace DirectX;
using namespace quetzal::direct3d11;

namespace
{

    string s_vertex_shader_filename("vertex_shader.cso");
    string s_pixel_shader_filename("pixel_shader.cso");
//    string s_texture_filename("textures/WoodCrate02.dds");
    string s_texture_filename("textures/stones.dds");

    vector<quetzal::direct3d11::VertexPositionNormalTexture> s_vertices =
    {
        {{{-0.5f, -0.5f, -0.5f}}, {{0.0f, 0.0f, 1.0f}}, {{0.0f, 0.0f}}},
        {{{-0.5f,  0.5f, -0.5f}}, {{0.0f, 0.0f, 1.0f}}, {{1.0f, 0.0f}}},
        {{{ 0.5f, -0.5f, -0.5f}}, {{0.0f, 0.0f, 1.0f}}, {{0.0f, 1.0f}}},
        {{{ 0.5f,  0.5f, -0.5f}}, {{0.0f, 0.0f, 1.0f}}, {{1.0f, 1.0f}}},

        {{{-0.5f, -0.5f, -0.5f}}, {{0.0f, 1.0f, 0.0f}}, {{0.0f, 0.0f}}},
        {{{ 0.5f, -0.5f, -0.5f}}, {{0.0f, 1.0f, 0.0f}}, {{1.0f, 0.0f}}},
        {{{-0.5f, -0.5f,  0.5f}}, {{0.0f, 1.0f, 0.0f}}, {{0.0f, 1.0f}}},
        {{{ 0.5f, -0.5f,  0.5f}}, {{0.0f, 1.0f, 0.0f}}, {{1.0f, 1.0f}}},

        {{{-0.5f, -0.5f, -0.5f}}, {{1.0f, 0.0f, 0.0f}}, {{0.0f, 0.0f}}},
        {{{-0.5f, -0.5f,  0.5f}}, {{1.0f, 0.0f, 0.0f}}, {{1.0f, 0.0f}}},
        {{{-0.5f,  0.5f, -0.5f}}, {{1.0f, 0.0f, 0.0f}}, {{0.0f, 1.0f}}},
        {{{-0.5f,  0.5f,  0.5f}}, {{1.0f, 0.0f, 0.0f}}, {{1.0f, 1.0f}}},

        {{{-0.5f, -0.5f,  0.5f}}, {{0.0f, 0.0f, -1.0f}}, {{0.0f, 0.0f}}},
        {{{ 0.5f, -0.5f,  0.5f}}, {{0.0f, 0.0f, -1.0f}}, {{1.0f, 0.0f}}},
        {{{-0.5f,  0.5f,  0.5f}}, {{0.0f, 0.0f, -1.0f}}, {{0.0f, 1.0f}}},
        {{{ 0.5f,  0.5f,  0.5f}}, {{0.0f, 0.0f, -1.0f}}, {{1.0f, 1.0f}}},

        {{{-0.5f,  0.5f, -0.5f}}, {{0.0f, -1.0f, 0.0f}}, {{0.0f, 0.0f}}},
        {{{-0.5f,  0.5f,  0.5f}}, {{0.0f, -1.0f, 0.0f}}, {{1.0f, 0.0f}}},
        {{{ 0.5f,  0.5f, -0.5f}}, {{0.0f, -1.0f, 0.0f}}, {{0.0f, 1.0f}}},
        {{{ 0.5f,  0.5f,  0.5f}}, {{0.0f, -1.0f, 0.0f}}, {{1.0f, 1.0f}}},

        {{{ 0.5f, -0.5f, -0.5f}}, {{-1.0f, 0.0f, 0.0f}}, {{0.0f, 0.0f}}},
        {{{ 0.5f,  0.5f, -0.5f}}, {{-1.0f, 0.0f, 0.0f}}, {{1.0f, 0.0f}}},
        {{{ 0.5f, -0.5f,  0.5f}}, {{-1.0f, 0.0f, 0.0f}}, {{0.0f, 1.0f}}},
        {{{ 0.5f,  0.5f,  0.5f}}, {{-1.0f, 0.0f, 0.0f}}, {{1.0f, 1.0f}}}
    };

//    vector<uint16_t> s_indices =
//    {
//         0,  1,  2,  3,
//        12, 13, 14, 15,
//        20, 21, 22, 23,
//         8,  9, 10, 11,
//        16, 17, 18, 19,
//         4,  5,  6,  7
//    };

    vector<uint16_t> s_indices =
    {
         0,  1,  2,
         2,  1,  3,
        12, 13, 14,
        14, 13, 15,
        20, 21, 22,
        22, 21, 23,
         8,  9, 10,
        10,  9, 11,
        16, 17, 18,
        18, 17, 19,
         4,  5,  6,
         6,  5,  7
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
quetzal::texture_test::Application::Application() :
    direct3d11::Application(),
    m_pVertexShader(),
    m_pPixelShader(),
    m_pInputLayout(),
    m_pBufferVertex(),
    m_pBufferIndex(),
    m_pBufferConstant(),
//    m_texture(m_renderer),
    m_pShaderResourceView(),
    m_nIndices(0),
    m_m4World(),
    m_up(XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f)),
    m_camera(),
    m_speed(100.0f),
    m_tFrame(),
    m_t(),
    m_statistics()
{
	m_window.create_floating("Texture Test", {100, 100}, {800, 600});
	m_renderer.create();

	init_geometry();
//    m_camera.look(XMVectorSet(1.5f, 1.5f, -1.5f, 0.0f), XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), m_up),
    m_camera.look(XMVectorSet(300.0f, 300.0f, -100.0f, 0.0f), XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), m_up),
    init_projection();

	m_window.show();
}

//------------------------------------------------------------------------------
void quetzal::texture_test::Application::update(float dt)
{
    check_input(dt);

    m_camera.update_view();

    m_statistics.update(dt);

    return;
}

//------------------------------------------------------------------------------
void quetzal::texture_test::Application::render()
{
    assert(m_pBufferConstant);

    ID3D11DeviceContext* pContext = m_renderer.context();
    assert(pContext);

    m_renderer.clear();

    pContext->IASetInputLayout(m_pInputLayout);
//    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    UINT stride = sizeof (direct3d11::VertexPositionNormalTexture);
    UINT offset = 0;
    ID3D11Buffer* pBuffers[] = {m_pBufferVertex.get()};
    pContext->IASetVertexBuffers(0, 1, pBuffers, &stride, &offset);
    pContext->IASetIndexBuffer(m_pBufferIndex, DXGI_FORMAT_R16_UINT, 0);

    draw_cube(0.0f, 0.0f, 0.0f);

//    m_statistics.text(); ...

	m_renderer.present();
    return;
}

//------------------------------------------------------------------------------
void quetzal::texture_test::Application::on_size(framework::Window::size_type size)
{
    size;
    init_projection();
    return;
}

//------------------------------------------------------------------------------
void quetzal::texture_test::Application::on_key_down(Timestamp ts, input::key::Modifiers modifiers, input::key::Code code)
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
void quetzal::texture_test::Application::on_mouse_move(Timestamp ts, input::key::Modifiers modifiers, framework::Window::position_type position)
{
    ts;
    modifiers;

    float rx = XMConvertToRadians(0.25f * static_cast<float>(position.x));
    float ry = XMConvertToRadians(0.25f * static_cast<float>(position.y));

    // Camera movement
    m_camera.pan(rx);
//    m_camera.pan(rx, m_up);
    m_camera.tilt(ry);
    m_camera.update_view();

    return;
}

//------------------------------------------------------------------------------
void quetzal::texture_test::Application::init_geometry()
{
    HRESULT hr = S_OK;

    vector<quetzal::direct3d11::VertexPositionNormalTexture>* pVertices = &s_vertices;
    vector<uint16_t>* pIndices = &s_indices;

    // load model ...
    direct3d11::Mesh mesh;
    bool b = read_obj(mesh, "models/spikosahedron.obj");
    vector<quetzal::direct3d11::VertexPositionNormalTexture> vertices(mesh.m_vertices.size());
    vector<uint16_t> indices(mesh.m_indices.size());

    transform(mesh.m_vertices.begin(), mesh.m_vertices.end(), vertices.begin(), [](const VertexPositionNormalTextureTangent& vin) -> VertexPositionNormalTexture {return VertexPositionNormalTexture(vin.m_position, vin.m_normal, vin.m_texture);});
    transform(mesh.m_indices.begin(), mesh.m_indices.end(), indices.begin(), [](unsigned int i) -> uint16_t {return static_cast<uint16_t>(i);});

    pVertices = &vertices;
    pIndices = &indices;

    m_nIndices = pIndices->size();

    // Load and create the vertex shader
    vector<char> shader_data = direct3d11::read(s_vertex_shader_filename);
    D3D11_CHECK(m_renderer.device()->CreateVertexShader(shader_data.data(), shader_data.size(), nullptr, &m_pVertexShader.p()));

    // Create the vertex input layout
    D3D11_CHECK(direct3d11::VertexPositionNormalTexture::create_input_layout(m_renderer, {shader_data.data(), shader_data.size()}, m_pInputLayout));

    // Load and create the pixel shader
    shader_data = direct3d11::read(s_pixel_shader_filename);
    D3D11_CHECK(m_renderer.device()->CreatePixelShader(shader_data.data(), shader_data.size(), nullptr, &m_pPixelShader.p()));

    // Create vertex buffer
    D3D11_BUFFER_DESC vertex_buffer_desc {sizeof (*pVertices)[0] * pVertices->size(), D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0};
    D3D11_SUBRESOURCE_DATA vertex_data {pVertices->data()};
    D3D11_CHECK(m_renderer.device()->CreateBuffer(&vertex_buffer_desc, &vertex_data, &m_pBufferVertex.p()));

    // Create the index buffer
    D3D11_BUFFER_DESC index_buffer_desc {sizeof (*pIndices)[0] * pIndices->size(), D3D11_USAGE_DEFAULT, D3D11_BIND_INDEX_BUFFER, 0, 0, 0};
    D3D11_SUBRESOURCE_DATA index_data {pIndices->data()};
    D3D11_CHECK(m_renderer.device()->CreateBuffer(&index_buffer_desc, &index_data, &m_pBufferIndex.p()));

    // Create the constant buffer
    D3D11_BUFFER_DESC constant_buffer_desc {sizeof s_constant_data, D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0};
    D3D11_CHECK(m_renderer.device()->CreateBuffer(&constant_buffer_desc, nullptr, &m_pBufferConstant.p()));

//    m_texture.open(s_texture_filename);
    m_pShaderResourceView = m_renderer.texture_manager().open(s_texture_filename);

    return;
}

//------------------------------------------------------------------------------
void quetzal::texture_test::Application::init_projection()
{
    m_camera.set_lens(static_cast<float>(m_window.client_size().aspect_ratio()), 1.0f, 1000.0f);
    return;
}

//------------------------------------------------------------------------------
void quetzal::texture_test::Application::check_input(float dt)
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
void quetzal::texture_test::Application::draw_cube(float x, float y, float z)
{
    ID3D11DeviceContext* pContext = m_renderer.context();

    // Shaders
    pContext->VSSetShader(m_pVertexShader, nullptr, 0);
    pContext->PSSetShader(m_pPixelShader, nullptr, 0);

    // Texture
//    ID3D11ShaderResourceView* pShaderResourceViews[] = {m_texture.shader_resource_view().get()};
    ID3D11ShaderResourceView* pShaderResourceViews[] = {m_pShaderResourceView};
    pContext->PSSetShaderResources(0, 1, pShaderResourceViews);

    ID3D11SamplerState* pSamplerStates[] =
    {
        m_renderer.common_states().SamplerLinearWrap(),
        m_renderer.common_states().SamplerLinearWrap()
    };  
    pContext->PSSetSamplers(0, 2, pSamplerStates);

    // Constant
    m_m4World = XMMatrixTranslation(x, y, z);
    ConstantData constant_data {XMMatrixTranspose(m_m4World), XMMatrixTranspose(m_camera.view()), XMMatrixTranspose(m_camera.projection())};
    pContext->UpdateSubresource(m_pBufferConstant, 0, nullptr, &constant_data, 0, 0);
    ID3D11Buffer* pBuffers[] = {m_pBufferConstant.get()};
    pContext->VSSetConstantBuffers(0, 1, pBuffers);

//    for (int i = 0; i < 6; ++i)
//    {
//        pContext->DrawIndexed(4, i * 4, 0);
//    }
    pContext->DrawIndexed(m_nIndices, 0, 0);

    return;
}
