//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#include <windows.h>
#include <d3d9.h>
#include <d3dx9tex.h>
//#include <dxerr9.h>

// Function prototypes.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void RegisterWindowClass(HINSTANCE hInstance);
void CreateAppWindow(HINSTANCE hInstance);
WPARAM StartMessageLoop();
HRESULT InitDirect3D();
void Render();
void CleanUpDirect3D();
void InitVertices();
void InitMatrices();
void InitLights();
bool CheckDevice();

// Global variables.
HWND g_hWnd;
IDirect3D9* g_pDirect3D = NULL;
IDirect3DDevice9* g_pDirect3DDevice = NULL;
IDirect3DVertexBuffer9* g_pVertexBuf = NULL;
IDirect3DIndexBuffer9* g_pIndexBuf = NULL;
IDirect3DSurface9* g_pBackBuffer = NULL;
IDirect3DSurface9* g_pBitmapSurface = NULL;
IDirect3DTexture9* g_pTexture = NULL;

//------------------------------------------------------------------------------
struct CustomVertex
{
    D3DVECTOR position;
    D3DVECTOR normal;
    float tu;
    float tv;
};

//------------------------------------------------------------------------------
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, INT)
{
    RegisterWindowClass(hInstance);
    CreateAppWindow(hInstance);
    ShowWindow(g_hWnd, SW_SHOWDEFAULT);
    UpdateWindow(g_hWnd);

    HRESULT hr = InitDirect3D();
    if (SUCCEEDED(hr))
    {
        InitVertices();
        WPARAM result = StartMessageLoop();
        result; // ...
    }

    CleanUpDirect3D();

    return 0;
}

//------------------------------------------------------------------------------
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_CREATE:
            return 0;

        case WM_DESTROY:
            PostQuitMessage( 0 );
            return 0;

        case WM_PAINT:
            ValidateRect(g_hWnd, NULL);
            return 0;

        case WM_KEYDOWN:
            switch(wParam)
            {
                case VK_ESCAPE:
                    PostQuitMessage(WM_QUIT);
                    break;
            }
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

//------------------------------------------------------------------------------
void RegisterWindowClass(HINSTANCE hInstance)
{
    WNDCLASSEX wc;
    wc.cbSize = sizeof (WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = (HCURSOR)LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = L"D3DTest";
    wc.hIconSm = NULL;

    RegisterClassEx(&wc);

    return;
}

//------------------------------------------------------------------------------
void CreateAppWindow(HINSTANCE hInstance)
{
    g_hWnd = CreateWindowEx(
        NULL,
        L"D3DTest",
        L"D3D Test",
        WS_OVERLAPPEDWINDOW,
        100, 100, 648, 514,
        GetDesktopWindow(),
        NULL,
        hInstance,
        NULL);

    return;
}

//------------------------------------------------------------------------------
WPARAM StartMessageLoop()
{
    MSG msg;

    for (; ; )
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            Render();
        }
    }

    return msg.wParam;
}

//------------------------------------------------------------------------------
HRESULT InitDirect3D()
{
    // Create the application's Direct3D object.
    g_pDirect3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (g_pDirect3D == NULL)
        return E_FAIL;

    // Verify that the hardware can handle the required display mode.
    HRESULT hr = g_pDirect3D->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, D3DFMT_X8R8G8B8, D3DFMT_X8R8G8B8, FALSE);
    if (hr != D3D_OK)
    {
        MessageBox(g_hWnd, L"Sorry. This program won't run on your system.", L"DirectX Error", MB_OK);
        return E_FAIL;
    }

    // Create the application's Direct3D device object.
    D3DPRESENT_PARAMETERS D3DPresentParams;
    ZeroMemory(&D3DPresentParams, sizeof (D3DPRESENT_PARAMETERS));
    D3DPresentParams.Windowed = FALSE;
    D3DPresentParams.BackBufferCount = 1;
    D3DPresentParams.BackBufferWidth = 1024;
    D3DPresentParams.BackBufferHeight = 768;
    D3DPresentParams.BackBufferFormat = D3DFMT_X8R8G8B8;
    D3DPresentParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
    D3DPresentParams.hDeviceWindow = g_hWnd;

//    D3DPresentParams.EnableAutoDepthStencil = TRUE;
//    D3DPresentParams.AutoDepthStencilFormat = D3DFMT_D16;

    hr = g_pDirect3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, g_hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &D3DPresentParams, &g_pDirect3DDevice);
    if (FAILED(hr))
        return E_FAIL;

    // Get a pointer to the backbuffer.
    g_pDirect3DDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &g_pBackBuffer);

    // Create and load a surface for the background bitmap.
    g_pDirect3DDevice->CreateOffscreenPlainSurface(1024, 768, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &g_pBitmapSurface, NULL);
    D3DXLoadSurfaceFromFile(g_pBitmapSurface, NULL, NULL, L"spinart_kml_2010_1024_768.bmp", NULL, D3DX_DEFAULT, 0, NULL);

    // Lighting
//    g_pDirect3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

    // Culling
//    g_pDirect3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    g_pDirect3DDevice->SetRenderState(D3DRS_ZENABLE, TRUE);

    hr = D3DXCreateTextureFromFile(g_pDirect3DDevice, L"texture2.bmp", &g_pTexture);
    g_pDirect3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    g_pDirect3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    g_pDirect3DDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    g_pDirect3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

    return D3D_OK;
}

//------------------------------------------------------------------------------
void CleanUpDirect3D()
{
    if (g_pTexture)
    {
        g_pTexture->Release();
        g_pTexture = NULL;
    }

    if (g_pBitmapSurface)
    {
        g_pBitmapSurface->Release();
        g_pBitmapSurface = NULL;
    }

    if (g_pIndexBuf)
    {
        g_pIndexBuf->Release();
        g_pIndexBuf = NULL;
    }

    if (g_pVertexBuf)
    {
        g_pVertexBuf->Release();
        g_pVertexBuf = NULL;
    }

    if (g_pBackBuffer)
    {
        g_pBackBuffer->Release();
        g_pBackBuffer = NULL;
    }

    if (g_pDirect3DDevice)
    {
        g_pDirect3DDevice->Release();
        g_pDirect3DDevice = NULL;
    }

    if (g_pDirect3D)
    {
        g_pDirect3D->Release();
        g_pDirect3D = NULL;
    }

    return;
}

//------------------------------------------------------------------------------
void Render()
{
    // Stop rendering if the app has lost its Direct3D device.
    if (!CheckDevice()) return;

    // Clear the back buffer to black.
    g_pDirect3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0 );

    g_pDirect3DDevice->BeginScene();

    InitLights();
    InitMatrices();

    // Copy the background bitmap to the back buffer.
    g_pDirect3DDevice->StretchRect(g_pBitmapSurface, NULL, g_pBackBuffer, NULL, D3DTEXF_NONE);

    // Give Direct3D the vertex buffer and vertex format.
    g_pDirect3DDevice->SetStreamSource(0, g_pVertexBuf, 0, sizeof (CustomVertex));
    g_pDirect3DDevice->SetTexture(0, g_pTexture);
    g_pDirect3DDevice->SetFVF(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1);

//    g_pDirect3DDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 60);
    g_pDirect3DDevice->SetIndices(g_pIndexBuf);
    g_pDirect3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 60, 0, 20);

    g_pDirect3DDevice->EndScene();

    g_pDirect3DDevice->Present(NULL, NULL, NULL, NULL);

    return;
}

//------------------------------------------------------------------------------
void InitVertices()
{
    const float gr = (1.0f + sqrt(5.0f)) / 2.0f;
    const float nx = (21.0f + 9.0f * sqrt(5.0f)) / 2.0f;
    const float ngr = gr / nx;
    const float ngr1 = (gr + 1.0f) / nx;
    const float n2gr1 = (2.0f * gr + 1.0f) / nx;

    CustomVertex vertices[] =
    {
        {0.0f, 1.0f, gr, ngr, 0.0f, n2gr1, 0.5f, 0.0f},
        {0.0f, -1.0f, gr, ngr, 0.0f, n2gr1, 0.0f, 0.866f},
        {gr, 0.0f, 1.0f, ngr, 0.0f, n2gr1, 1.0f, 0.866f},

        {0.0f, 1.0f, gr, -ngr, 0.0f, n2gr1, 0.5f, 0.0f},
        {-gr, 0.0f, 1.0f, -ngr, 0.0f, n2gr1, 0.0f, 0.866f},
        {0.0f, -1.0f, gr, -ngr, 0.0f, n2gr1, 1.0f, 0.866f},

        {0.0f, 1.0f, gr, -ngr1, ngr1, ngr1, 0.5f, 0.0f},
        {-1.0f, gr, 0.0f, -ngr1, ngr1, ngr1, 0.0f, 0.866f},
        {-gr, 0.0f, 1.0f, -ngr1, ngr1, ngr1, 1.0f, 0.866f},

        {0.0f, 1.0f, gr, 0.0f, n2gr1, ngr, 0.5f, 0.0f},
        {1.0f, gr, 0.0f, 0.0f, n2gr1, ngr, 0.0f, 0.866f},
        {-1.0f, gr, 0.0f, 0.0f, n2gr1, ngr, 1.0f, 0.866f},

        {0.0f, 1.0f, gr, ngr1, ngr1, ngr1, 0.5f, 0.0f},
        {gr, 0.0f, 1.0f, ngr1, ngr1, ngr1, 0.0f, 0.866f},
        {1.0f, gr, 0.0f, ngr1, ngr1, ngr1, 1.0f, 0.866f},

        {gr, 0.0f, 1.0f, n2gr1, -ngr, 0.0f, 0.5f, 0.0f},
        {1.0f, -gr, 0.0f, n2gr1, -ngr, 0.0f, 0.0f, 0.866f},
        {gr, 0.0f, -1.0f, n2gr1, -ngr, 0.0f, 1.0f, 0.866f},

        {gr, 0.0f, 1.0f, ngr1, -ngr1, ngr1, 0.5f, 0.0f},
        {0.0f, -1.0f, gr, ngr1, -ngr1, ngr1, 0.0f, 0.866f},
        {1.0f, -gr, 0.0f, ngr1, -ngr1, ngr1, 1.0f, 0.866f},

        {0.0f, -1.0f, gr, 0.0f, -n2gr1, ngr, 0.5f, 0.0f},
        {-1.0f, -gr, 0.0f, 0.0f, -n2gr1, ngr, 0.0f, 0.866f},
        {1.0f, -gr, 0.0f, 0.0f, -n2gr1, ngr, 1.0f, 0.866f},

        {0.0f, -1.0f, gr, -ngr1, -ngr1, ngr1, 0.5f, 0.0f},
        {-gr, 0.0f, 1.0f, -ngr1, -ngr1, ngr1, 0.0f, 0.866f},
        {-1.0f, -gr, 0.0f, -ngr1, -ngr1, ngr1, 1.0f, 0.866f},

        {-gr, 0.0f, 1.0f, -n2gr1, -ngr, 0.0f, 0.5f, 0.0f},
        {-gr, 0.0f, -1.0f, -n2gr1, -ngr, 0.0f, 0.0f, 0.866f},
        {-1.0f, -gr, 0.0f, -n2gr1, -ngr, 0.0f, 1.0f, 0.866f},

        {-gr, 0.0f, 1.0f, -n2gr1, ngr, 0.0f, 0.5f, 0.0f},
        {-1.0f, gr, 0.0f, -n2gr1, ngr, 0.0f, 0.0f, 0.866f},
        {-gr, 0.0f, -1.0f, -n2gr1, ngr, 0.0f, 1.0f, 0.866f},

        {-1.0f, gr, 0.0f, -ngr1, ngr1, -ngr1, 0.5f, 0.0f},
        {0.0f, 1.0f, -gr, -ngr1, ngr1, -ngr1, 0.0f, 0.866f},
        {-gr, 0.0f, -1.0f, -ngr1, ngr1, -ngr1, 1.0f, 0.866f},

        {-1.0f, gr, 0.0f, 0.0f, n2gr1, -ngr, 0.5f, 0.0f},
        {1.0f, gr, 0.0f, 0.0f, n2gr1, -ngr, 0.0f, 0.866f},
        {0.0f, 1.0f, -gr, 0.0f, n2gr1, -ngr, 1.0f, 0.866f},

        {1.0f, gr, 0.0f, ngr1, ngr1, -ngr1, 0.5f, 0.0f},
        {gr, 0.0f, -1.0f, ngr1, ngr1, -ngr1, 0.0f, 0.866f},
        {0.0f, 1.0f, -gr, ngr1, ngr1, -ngr1, 1.0f, 0.866f},

        {1.0f, gr, 0.0f, n2gr1, ngr, 0.0f, 0.5f, 0.0f},
        {gr, 0.0f, 1.0f, n2gr1, ngr, 0.0f, 0.0f, 0.866f},
        {gr, 0.0f, -1.0f, n2gr1, ngr, 0.0f, 1.0f, 0.866f},

        {gr, 0.0f, -1.0f, ngr1, -ngr1, -ngr1, 0.5f, 0.0f},
        {1.0f, -gr, 0.0f, ngr1, -ngr1, -ngr1, 0.0f, 0.866f},
        {0.0f, -1.0f, -gr, ngr1, -ngr1, -ngr1, 1.0f, 0.866f},

        {1.0f, -gr, 0.0f, 0.0f, -n2gr1, -ngr, 0.5f, 0.0f},
        {-1.0f, -gr, 0.0f, 0.0f, -n2gr1, -ngr, 0.0f, 0.866f},
        {0.0f, -1.0f, -gr, 0.0f, -n2gr1, -ngr, 1.0f, 0.866f},

        {-1.0f, -gr, 0.0f, -ngr1, -ngr1, -ngr1, 0.5f, 0.0f},
        {-gr, 0.0f, -1.0f, -ngr1, -ngr1, -ngr1, 0.0f, 0.866f},
        {0.0f, -1.0f, -gr, -ngr1, -ngr1, -ngr1, 1.0f, 0.866f},

        {-gr, 0.0f, -1.0f, -ngr, 0.0f, -n2gr1, 0.5f, 0.0f},
        {0.0f, 1.0f, -gr, -ngr, 0.0f, -n2gr1, 0.0f, 0.866f},
        {0.0f, -1.0f, -gr, -ngr, 0.0f, -n2gr1, 1.0f, 0.866f},

        {0.0f, 1.0f, -gr, ngr, 0.0f, -n2gr1, 0.5f, 0.0f},
        {gr, 0.0f, -1.0f, ngr, 0.0f, -n2gr1, 0.0f, 0.866f},
        {0.0f, -1.0f, -gr, ngr, 0.0f, -n2gr1, 1.0f, 0.866f}
    };

    WORD indices[] =
    {
          0,  1,  2,
          3,  4,  5,
          6,  7,  8,
          9, 10, 11,
         12, 13, 14,
         15, 16, 17,
         18, 19, 20,
         21, 22, 23,
         24, 25, 26,
         27, 28, 29,
         30, 31, 32,
         33, 34, 35,
         36, 37, 38,
         39, 40, 41,
         42, 43, 44,
         45, 46, 47,
         48, 49, 50,
         51, 52, 53,
         54, 55, 56,
         57, 58, 59
    };

    int nVertices = sizeof vertices / sizeof vertices[0];
    int nIndices = sizeof indices / sizeof indices[0];

    // Create the vertex buffer.
    g_pDirect3DDevice->CreateVertexBuffer(nVertices * sizeof (CustomVertex), 0, D3DFVF_XYZ | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &g_pVertexBuf, NULL);

    // Copy the vertices into the buffer.
    void* pVertices;
    g_pVertexBuf->Lock(0, sizeof vertices, (void**)&pVertices, 0);
    memcpy(pVertices, vertices, sizeof vertices);
    g_pVertexBuf->Unlock();

    // Create the index buffer.
    g_pDirect3DDevice->CreateIndexBuffer(nIndices * sizeof (WORD), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, &g_pIndexBuf, 0);

    // Copy the indices into the buffer.
    void* pIndices;
    g_pIndexBuf->Lock(0, sizeof indices, (void**)&pIndices, 0);
    memcpy(pIndices, indices, sizeof indices);
    g_pIndexBuf->Unlock();

    return;
}

//------------------------------------------------------------------------------
void InitMatrices()
{
    // Set up the world transformation.
    D3DXMATRIX worldMatrix;
    D3DXMATRIX tempMatrix;
    float radians = timeGetTime() / 1000.0f;
    D3DXMatrixRotationY(&worldMatrix, radians);
    g_pDirect3DDevice->SetTransform(D3DTS_WORLD, &worldMatrix);

//  Die rolling movement ...
//    D3DXMatrixIdentity(&worldMatrix);
//    D3DXMatrixTranslation(&tempMatrix, x, y, z);
//    D3DXMatrixMultiply(&worldMatrix, &worldMatrix, &tempmatrix);

    // Set up the view transformation.
    D3DXVECTOR3 vEyePt(0.0f, 0.0f, -20.0f);
    D3DXVECTOR3 vLookatPt(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vUpVec(0.0f, 1.0f, 0.0f);
    D3DXMATRIX viewMatrix;
    D3DXMatrixLookAtLH(&viewMatrix, &vEyePt, &vLookatPt, &vUpVec);
    g_pDirect3DDevice->SetTransform(D3DTS_VIEW, &viewMatrix);

    // Set up the projection transformation.
    D3DXMATRIX projectionMatrix;
    D3DXMatrixPerspectiveFovLH(&projectionMatrix, D3DX_PI / 4, 1.333f, 1.0f, 50.0f);
    g_pDirect3DDevice->SetTransform(D3DTS_PROJECTION, &projectionMatrix);

    return;
}

//------------------------------------------------------------------------------
void InitLights()
{
    D3DLIGHT9 light0;
    ZeroMemory(&light0, sizeof(light0));

    light0.Type = D3DLIGHT_POINT;
    light0.Diffuse.r  = 10.0f;
    light0.Diffuse.g  = 10.0f;
    light0.Diffuse.b  = 10.0f;
    light0.Ambient.r  = 1.0f;
    light0.Ambient.g  = 1.0f;
    light0.Ambient.b  = 1.0f;
    light0.Specular.r  = 1.0f;
    light0.Specular.g  = 1.0f;
    light0.Specular.b  = 1.0f;
    light0.Position.x = 5.0f;
    light0.Position.y = 5.0f;
    light0.Position.z = -5.0f;
    light0.Range = 100.0f;
    light0.Attenuation1 = 0.3f; 

    g_pDirect3DDevice->SetLight(0, &light0);
    g_pDirect3DDevice->LightEnable(0, TRUE);

    D3DMATERIAL9 material;
    ZeroMemory(&material, sizeof(material));
    material.Diffuse.r = 1.0f;
    material.Diffuse.g = 1.0f;
    material.Diffuse.b = 1.0f;
    material.Diffuse.a = 0.0f;
    material.Ambient.r = 1.0f;
    material.Ambient.g = 1.0f;
    material.Ambient.b = 1.0f;
    material.Ambient.a = 0.0f;
    g_pDirect3DDevice->SetMaterial(&material);

    return;
}

//------------------------------------------------------------------------------
bool CheckDevice()
{
    HRESULT hr = g_pDirect3DDevice->TestCooperativeLevel();
    if (hr == D3DERR_DEVICELOST)
    {
        return false;
    }
    else if (hr == D3DERR_DEVICENOTRESET)
    {
        CleanUpDirect3D();
        InitDirect3D();
        InitVertices();
        return true;
    }

    return true;
}
