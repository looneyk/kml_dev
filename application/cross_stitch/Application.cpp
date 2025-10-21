//------------------------------------------------------------------------------
// Application.cpp
//
//------------------------------------------------------------------------------

#include "Application.hpp"
#include "quetzal/ComException.hpp"
#include <cmath>
#include <cassert>

using namespace std;

namespace
{

    wchar_t* window_class_name = L"Application";

    // Required in place of window creation parameters to avoid slicing
    // Would this be an alternative:
    // https://msdn.microsoft.com/en-us/library/ff381400%28d=printer,v=vs.85%29.aspx
    quetzal::d2d::Application* g_pApplicationTemp = nullptr; // cache the value so it can be assigned at the proper time
    quetzal::d2d::Application* g_pApplication = nullptr;

    //----------------------------------------------------------------------
    LRESULT CALLBACK window_procedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_CREATE)
        {
            g_pApplication = g_pApplicationTemp;

            return 1;
        }

        if (g_pApplication != nullptr)
        {
            return g_pApplication->message_handler(message, wParam, lParam);
        }

        return DefWindowProc(hwnd, message, wParam, lParam);
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::d2d::Application::Application(HINSTANCE hinstance, int nCmdShow, const wchar_t* name, int idMenu) :
    m_platform(),
    m_hinstance(hinstance),
    m_nCmdShow(nCmdShow),
    m_name(name),
    m_idMenu(idMenu),
    m_hwnd(nullptr),
    m_pID2D1Factory(),
    m_pRenderTarget()
{
}

//------------------------------------------------------------------------------
quetzal::d2d::Application::~Application()
{
}

//------------------------------------------------------------------------------
void quetzal::d2d::Application::run()
{
    init_();

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::d2d::Application::init_()
{
    g_pApplicationTemp = this;

    HRESULT hr;

    COM_CHECK(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pID2D1Factory.p()));

    // Register the window class
    WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = window_procedure;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = sizeof(LONG_PTR);
    wc.hInstance     = m_hinstance;
    wc.hIcon         = nullptr;
    wc.hCursor       = LoadCursor(nullptr, IDI_APPLICATION);
    wc.hbrBackground = nullptr;
    wc.lpszMenuName  = MAKEINTRESOURCE(m_idMenu);
    wc.lpszClassName = window_class_name;
    wc.hIconSm       = nullptr;

    RegisterClassEx(&wc);

    // Because the CreateWindow function takes its size in pixels, obtain the system DPI and use it to scale the window size.
    // The factory returns the current system DPI. This is also the value it will use to create its own windows.
    FLOAT dpiX, dpiY;
    m_pID2D1Factory->GetDesktopDpi(&dpiX, &dpiY);

    // Create the window 
    m_hwnd = CreateWindow(window_class_name, m_name.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        static_cast<UINT>(ceil(640.f * dpiX / 96.f)), static_cast<UINT>(ceil(480.f * dpiY / 96.f)), nullptr, nullptr, m_hinstance, nullptr);
    COM_CHECK(m_hwnd != nullptr);

    ShowWindow(m_hwnd, m_nCmdShow);
    UpdateWindow(m_hwnd);

    return;
}

//------------------------------------------------------------------------------
HRESULT quetzal::d2d::Application::create_device_resources()
{
    if (m_pRenderTarget == nullptr)
    {
        RECT rect;
        GetClientRect(m_hwnd, &rect);

        D2D1_SIZE_U size = D2D1::SizeU(rect.right - rect.left, rect.bottom - rect.top);

        HRESULT hr = m_pID2D1Factory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(m_hwnd, size), &m_pRenderTarget.p());
        if (SUCCEEDED(hr))
        {
            return create_device_resources_();
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------------
void quetzal::d2d::Application::release_device_resources()
{
    release_device_resources_();

    m_pRenderTarget = nullptr;

    return;
}

//------------------------------------------------------------------------------
void quetzal::d2d::Application::on_render()
{
    HRESULT hr = create_device_resources();
    if (SUCCEEDED(hr))
    {
        m_pRenderTarget->BeginDraw();

        render_();

        hr = m_pRenderTarget->EndDraw();
    }

    if (hr == D2DERR_RECREATE_TARGET)
    {
        hr = S_OK;
        release_device_resources();
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::d2d::Application::on_resize(UINT w, UINT h)
{
    if (m_pRenderTarget != nullptr)
    {
        m_pRenderTarget->Resize(D2D1::SizeU(w, h)); // Errors ignored here, will be returned again by EndDraw
    }

    return;
}

//------------------------------------------------------------------------------
LRESULT quetzal::d2d::Application::message_handler(UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lresult = message_handler_(message, wParam, lParam);
    if (lresult == 0)
    {
        return lresult;
    }

    switch (message)
    {
        case WM_SIZE:
            g_pApplication->on_resize(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_PAINT:
            g_pApplication->on_render();
            ValidateRect(m_hwnd, nullptr);
            return 0;

        case WM_DISPLAYCHANGE:
            InvalidateRect(m_hwnd, nullptr, FALSE);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 1; // not 0? ...
    }

    return DefWindowProc(m_hwnd, message, wParam, lParam);
}
