//------------------------------------------------------------------------------
//
// Copyright (c) 2013 Keith Michael Looney. All Rights Reserved.
//
//------------------------------------------------------------------------------
//
// Cubes
// application.cpp
//
//------------------------------------------------------------------------------

#include <cmath>
#include <cassert>
#include "application.hpp"
#include "quetzal/dx/Vertex.hpp"
#include "quetzal/dx/dx_util.hpp"

typedef quetzal::device::Key Key;
typedef quetzal::device::KeyModifier KeyModifier;

namespace
{

    float color[16][3] =
    {
        {1.0f, 0.0f, 0.0f},
        {1.0f, 0.27f, 0.0f},
        {1.0f, 0.43f, 0.0f},
        {1.0f, 0.6f, 0.0f},
        {1.0f, 0.8f, 0.0f},
        {0.9f, 0.9f, 0.0f},
        {0.7f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.9f, 0.5f},
        {0.0f, 0.8f, 0.8f},
        {0.0f, 0.5f, 0.9f},
        {0.2f, 0.2f, 1.0f},
        {0.4f, 0.0f, 1.0f},
        {0.6f, 0.0f, 1.0f},
        {0.9f, 0.0f, 0.9f},
        {1.0f, 0.0f, 0.5f}
    };
/*
        {1.0f, 0.0f, 0.0f},
        {1.0f, 0.2f, 0.0f},
        {1.0f, 0.4f, 0.0f},
        {1.0f, 0.6f, 0.0f},
        {1.0f, 0.8f, 0.0f},
        {1.0f, 1.0f, 0.0f},
        {0.5f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.5f},
        {0.0f, 1.0f, 1.0f},
        {0.0f, 0.5f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.4f, 0.0f, 1.0f},
        {0.6f, 0.0f, 1.0f},
        {1.0f, 0.0f, 1.0f},
        {1.0f, 0.0f, 0.5f}
*/

} // namespace

//------------------------------------------------------------------------------
quetzal::cubes::application::application(HINSTANCE hInstance, std::wstring strCaption, D3DDEVTYPE devType, DWORD requestedVP) :
    quetzal::dx::Application(hInstance, strCaption, devType, requestedVP),
    m_statistics(),
    m_nVertices(0),
    m_nIndices(0),
//    m_display(),
    m_bCycle(false),
    m_nColors(sizeof color / sizeof color[0][0] / 3),
    m_nColor(11),
    m_smooth(false),
    m_tFrameDuration(0.1f),
    m_pVertexBuffer(),
    m_pIndexBuffer(),
    m_camera(12.0f, 12.0f, -12.0f),
    m_direction(-0.57735027f, -0.57735027f, 0.43301270f),
    m_speed(0.5f),
    m_scale(1.0f),
    m_tFrame(0.0f),
    m_t(0.0f),

    m_pEffect(),
    m_hTechnique(),
    m_hWVP()
{
    InitVertexDeclarations(m_pD3DDevice);

    m_statistics.init(m_pD3DDevice);

    init_display();

    init();
}

//------------------------------------------------------------------------------
quetzal::cubes::application::~application()
{
    final();
}

//------------------------------------------------------------------------------
bool quetzal::cubes::application::checkDeviceCaps()
{
    return true;
}

//------------------------------------------------------------------------------
void quetzal::cubes::application::onLostDevice()
{
    HRESULT hr = S_OK;

    m_statistics.onLostDevice();
    DX_CHECK(m_pEffect->OnLostDevice());

//final(); // ...

    return;
}

//------------------------------------------------------------------------------
void quetzal::cubes::application::onResetDevice()
{
    HRESULT hr = S_OK;

//init(); // ...

    m_statistics.onResetDevice();
    DX_CHECK(m_pEffect->OnResetDevice());

    initProjectionMatrix();

    return;
}

//------------------------------------------------------------------------------
void quetzal::cubes::application::update(Timer::Timestamp dt)
{
    check_input(dt);

    initViewMatrix();

    m_t += dt;
    while (m_t >= m_tFrame + m_tFrameDuration)
    {
        if (m_bCycle)
        {
            m_nColor = (m_nColor + 1) % m_nColors;
        }

        m_tFrame += m_tFrameDuration;
    }

    if (m_bCycle) // and only if color actually changed ...
    {
        if (m_smooth)
        {
            init_display(m_nColor, static_cast<float>((m_t - m_tFrame) / m_tFrameDuration));
        }
        else
        {
            init_display();
        }
        init_geometry(); // does not need to do all of that ...
    }

//    const CubeArray& cubearray = m_display.cubearray();
//
//    m_statistics.setVertices(m_nVertices);
//    m_statistics.setTriangles(cubearray.width() * cubearray.height() * m_nIndices / 3);
m_statistics.setVertices(28 * 24);
m_statistics.setTriangles(28 * 12);
    m_statistics.update(dt);

    return;
}

//------------------------------------------------------------------------------
void quetzal::cubes::application::draw()
{
    assert(m_pD3DDevice != nullptr);

    HRESULT hr = S_OK;

    DX_CHECK(m_pD3DDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00000000, 1.0f, 0));

    DX_CHECK(m_pD3DDevice->BeginScene());

    DX_CHECK(m_pEffect->SetTechnique(m_hTechnique));

    UINT nPasses = 0;
    DX_CHECK(m_pEffect->Begin(&nPasses, 0));

    for (UINT i = 0; i < nPasses; ++i)
    {
        DX_CHECK(m_pEffect->BeginPass(i));

        draw_pyramid();

        DX_CHECK(m_pEffect->EndPass());
    }

    DX_CHECK(m_pEffect->End());

    m_statistics.display();

    DX_CHECK(m_pD3DDevice->EndScene());

    DX_CHECK(m_pD3DDevice->Present(0, 0, 0, 0));

    return;
}

//------------------------------------------------------------------------------
bool quetzal::cubes::application::input_key_down(Timer::Timestamp ts, KeyModifier::Modifiers modifiers, int keycode)
{
    ts;
    modifiers;

    switch (keycode)
    {
        case Key::V_ESCAPE:
            close();
            break;

        case Key::V_F:
            if (fullscreen())
            {
                set_windowed();
            }
            else
            {
                set_fullscreen();
            }
            break;

        case Key::V_I:
            if (solid())
            {
                set_wireframe();
            }
            else
            {
                set_solid();
            }
            break;

        case Key::V_C:
            m_bCycle = !m_bCycle;
            break;

        case Key::V_M:
            m_smooth = !m_smooth;
            break;

        default:
            m_keycache.set_key_down(keycode);
            break;
    }

    return true;
}

//------------------------------------------------------------------------------
bool quetzal::cubes::application::input_key_up(Timer::Timestamp ts, KeyModifier::Modifiers modifiers, int keycode)
{
    ts;
    modifiers;
    keycode;

    return true;
}

//------------------------------------------------------------------------------
bool quetzal::cubes::application::input_pointer_move(Timer::Timestamp ts, KeyModifier::Modifiers modifiers, dx::vector_type position)
{
    ts;
    modifiers;

    // Camera movement

    float roty = position.x / 100.0f;
    float rotx = position.y / 100.0f;
    if (roty == 0.0f && rotx == 0.0f)
    {
        return true;
    }

//m_ofs << "ri mouse " << roty << ", " << rotx << std::endl;
    D3DXMATRIX matrixRotation;
    D3DXMatrixRotationY(&matrixRotation, roty);
    D3DXMATRIX matrixTemp;
    D3DXMatrixRotationX(&matrixTemp, rotx);
    D3DXMatrixMultiply(&matrixRotation, &matrixRotation, &matrixTemp);
    D3DXVec3TransformCoord(&m_direction, &m_direction, &matrixRotation);
    D3DXVec3Normalize(&m_direction, &m_direction); // Should not be necessary except for rounding ...

    return true;
}

//------------------------------------------------------------------------------
void quetzal::cubes::application::init()
{
    init_geometry();
    init_effects();
    initProjectionMatrix();

    return;
}

//------------------------------------------------------------------------------
void quetzal::cubes::application::final()
{
    m_pVertexBuffer = 0;
    m_pIndexBuffer = 0;
    m_pEffect = 0;

    return;
}

//------------------------------------------------------------------------------
void quetzal::cubes::application::init_geometry()
{
    assert(m_pD3DDevice != nullptr);

    const float e = 1.0f / 512.0f;
    const D3DCOLOR color1 = D3DCOLOR_XRGB(static_cast<int>((m_color.r + e) * 255), static_cast<int>((m_color.g + e) * 255), static_cast<int>((m_color.b + e) * 255));
    const D3DCOLOR color2 = D3DCOLOR_XRGB(static_cast<int>((m_color.r / 2 + e) * 255), static_cast<int>((m_color.g / 2 + e) * 255), static_cast<int>((m_color.b / 2 + e) * 255));
    const D3DCOLOR color4 = D3DCOLOR_XRGB(static_cast<int>((m_color.r / 4 + e) * 255), static_cast<int>((m_color.g / 4 + e) * 255), static_cast<int>((m_color.b / 4 + e) * 255));

    VertexCol vertices[] =
    {
        {{-0.5f, -0.5f, -0.5f}, color1},
        {{-0.5f,  0.5f, -0.5f}, color1},
        {{ 0.5f, -0.5f, -0.5f}, color1},
        {{ 0.5f,  0.5f, -0.5f}, color1},

        {{-0.5f, -0.5f, -0.5f}, color4},
        {{ 0.5f, -0.5f, -0.5f}, color4},
        {{-0.5f, -0.5f,  0.5f}, color4},
        {{ 0.5f, -0.5f,  0.5f}, color4},

        {{-0.5f, -0.5f, -0.5f}, color2},
        {{-0.5f, -0.5f,  0.5f}, color2},
        {{-0.5f,  0.5f, -0.5f}, color2},
        {{-0.5f,  0.5f,  0.5f}, color2},

        {{-0.5f, -0.5f,  0.5f}, color1},
        {{ 0.5f, -0.5f,  0.5f}, color1},
        {{-0.5f,  0.5f,  0.5f}, color1},
        {{ 0.5f,  0.5f,  0.5f}, color1},

        {{-0.5f,  0.5f, -0.5f}, color4},
        {{-0.5f,  0.5f,  0.5f}, color4},
        {{ 0.5f,  0.5f, -0.5f}, color4},
        {{ 0.5f,  0.5f,  0.5f}, color4},

        {{ 0.5f, -0.5f, -0.5f}, color2},
        {{ 0.5f,  0.5f, -0.5f}, color2},
        {{ 0.5f, -0.5f,  0.5f}, color2},
        {{ 0.5f,  0.5f,  0.5f}, color2}
    };

    m_nVertices = sizeof vertices / sizeof vertices[0];

    WORD indices[] =
    {
         0,  1,  2,  3,
         4,  5,  6,  7,
         8,  9, 10, 11,
        12, 13, 14, 15,
        16, 17, 18, 19,
        20, 21, 22, 23
    };

    m_nIndices = sizeof indices / sizeof indices[0];

    // Create the vertex buffer.
    m_pD3DDevice->CreateVertexBuffer(m_nVertices * sizeof (VertexCol), 0, D3DFVF_XYZ | D3DFVF_DIFFUSE, D3DPOOL_MANAGED, &m_pVertexBuffer.p(), nullptr);

    // Copy the vertices into the buffer.
    void* pVertices;
    m_pVertexBuffer->Lock(0, sizeof vertices, (void**)&pVertices, 0);
    memcpy(pVertices, vertices, sizeof vertices);
    m_pVertexBuffer->Unlock();

    // Create the index buffer.
    m_pD3DDevice->CreateIndexBuffer(m_nIndices * sizeof (WORD), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, &m_pIndexBuffer.p(), nullptr);

    // Copy the indices into the buffer.
    void* pIndices;
    m_pIndexBuffer->Lock(0, sizeof indices, (void**)&pIndices, 0);
    memcpy(pIndices, indices, sizeof indices);
    m_pIndexBuffer->Unlock();

    return;
}

//------------------------------------------------------------------------------
void quetzal::cubes::application::init_effects()
{
    ID3DXBuffer* pBufferError = 0;
    HRESULT hr = D3DXCreateEffectFromFile(m_pD3DDevice, L"color.fx", 0, 0, D3DXSHADER_DEBUG, 0, &m_pEffect.p(), &pBufferError);
    if (FAILED(hr))
    {
        if (pBufferError != nullptr)
        {
            MessageBox(0, reinterpret_cast<wchar_t*>(pBufferError->GetBufferPointer()), 0, 0);
        }
    }

    m_hTechnique = m_pEffect->GetTechniqueByName("ColorTech");
    m_hWVP = m_pEffect->GetParameterByName(0, "gWVP");

    return;
}

//------------------------------------------------------------------------------
void quetzal::cubes::application::initProjectionMatrix()
{
//    float w = (float)m_d3dPP.BackBufferWidth;
//    float h = (float)m_d3dPP.BackBufferHeight;
//    D3DXMatrixPerspectiveFovLH(&m_matrixProjection, D3DX_PI * 0.25f, w / h, 1.0f, 5000.0f);

    float w = 20.0f * m_scale;
    float h = 15.0f * m_scale;
    D3DXMatrixOrthoLH(&m_matrixProjection, w, h, 1.0f, 10000.0f);

    return;
}

//------------------------------------------------------------------------------
void quetzal::cubes::application::initViewMatrix()
{
    D3DXVECTOR3 target = m_camera + m_direction;
    D3DXVECTOR3 up(0.0f, 0.0f, -1.0f);
    D3DXMatrixLookAtLH(&m_matrixView, &m_camera, &target, &up);

    return;
}

//------------------------------------------------------------------------------
void quetzal::cubes::application::check_input(Timer::Timestamp dt)
{
    m_keyboard.poll();

    // debug this? seems OK ...

    if (m_keyboard.key_down(Key::V_W))
    {
        m_scale -= m_speed * static_cast<float>(dt);
        initProjectionMatrix();
    }

    if (m_keycache.key_down(Key::V_W))
    {
        m_scale -= m_speed * static_cast<float>(dt);
        initProjectionMatrix();
    }

    if (m_keyboard.key_down(Key::V_S))
    {
        m_scale += m_speed * static_cast<float>(dt);
        initProjectionMatrix();
    }

    if (m_keycache.key_down(Key::V_S))
    {
        m_scale += m_speed * static_cast<float>(dt);
        initProjectionMatrix();
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::cubes::application::init_display()
{
//    CubeArray& cubearray = m_display.cubearray();

    assert(m_nColor < m_nColors);

    m_color.r = color[m_nColor][0];
    m_color.g = color[m_nColor][1];
    m_color.b = color[m_nColor][2];

//    for (int y = 0; y < h; ++y)
//    {
//        for (int x = 0; x < w; ++x)
//        {
//            Cube& cube = cubearray.cube(x, y);
//
//            cube.rgba(color[y][0], color[y][1], color[y][2], 1.0f);
//            cube.brightness(1.0f);
//        }
//    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::cubes::application::init_display(int nColor0, float fraction1)
{
    assert(nColor0 < m_nColors);
    assert(fraction1 >= 0.0f && fraction1 <= 1.0f);

    int nColor1 = (nColor0 + 1) % m_nColors;
    assert(nColor1 < m_nColors);

    float fraction0 = 1.0f - fraction1;
    assert(fraction0 >= 0.0f && fraction0 <= 1.0f);

    m_color.r = color[nColor0][0] * fraction0 + color[nColor1][0] * fraction1;
    m_color.g = color[nColor0][1] * fraction0 + color[nColor1][1] * fraction1;
    m_color.b = color[nColor0][2] * fraction0 + color[nColor1][2] * fraction1;

    return;
}

//------------------------------------------------------------------------------
void quetzal::cubes::application::draw_pyramid()
{
//    const CubeArray& cubearray = m_display.cubearray();
    const int nRows = 7;

    for (int i = 0; i < nRows; ++i)
    {
        float z = (i - nRows + 0.5f); // 0.5 = cube dimension / 2

        for (int j = 0; j <= i; ++j)
        {
            float x = i - j + 0.5f;
            float y = j + 0.5f;

//            const Cube& cube = cubearray.cube(nCube++);

//            draw_cube(cube, x, y, z);
            draw_cube(x, y, z);
        }
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::cubes::application::draw_cube(/*const Cube& cube, */float x, float y, float z)
{
    assert(m_pD3DDevice != nullptr);

    HRESULT hr = S_OK;

    DX_CHECK(m_pD3DDevice->SetStreamSource(0, m_pVertexBuffer, 0, sizeof (VertexCol)));
    DX_CHECK(m_pD3DDevice->SetIndices(m_pIndexBuffer));
    DX_CHECK(m_pD3DDevice->SetVertexDeclaration(VertexCol::Declaration));


    D3DXMATRIX matrixWorld;
    D3DXMATRIX matrixTemp;
    D3DXMatrixIdentity(&matrixWorld);
    D3DXMatrixTranslation(&matrixTemp, x, y, z);
    D3DXMatrixMultiply(&matrixWorld, &matrixWorld, &matrixTemp);

    matrixTemp = matrixWorld * m_matrixView * m_matrixProjection;
    DX_CHECK(m_pEffect->SetMatrix(m_hWVP, &matrixTemp));

    DX_CHECK(m_pEffect->CommitChanges());

    for (int i = 0; i < 6; ++i)
    {
        DX_CHECK(m_pD3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLESTRIP, 0, 0, 24, i * 4, 2));
    }

    return;
}
