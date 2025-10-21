//------------------------------------------------------------------------------
// CrossStitch.cpp
//
//------------------------------------------------------------------------------

#include "CrossStitch.hpp"
#include "resource.h"
#include <cassert>

//------------------------------------------------------------------------------
quetzal::CrossStitch::CrossStitch(HINSTANCE hInstance, int nCmdShow) :
    Application(hInstance, nCmdShow, L"Cross Stitch Designer", IDC_CROSS_STITCH),
    m_pLightSlateGrayBrush(),
    m_pCornflowerBlueBrush()
{
}

//------------------------------------------------------------------------------
quetzal::CrossStitch::~CrossStitch()
{
}

//------------------------------------------------------------------------------
HRESULT quetzal::CrossStitch::create_device_resources_()
{
    HRESULT hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LightSlateGray), &m_pLightSlateGrayBrush.p());
    if (SUCCEEDED(hr))
    {
        hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::CornflowerBlue), &m_pCornflowerBlueBrush.p());
    }

    return hr;
}

//------------------------------------------------------------------------------
void quetzal::CrossStitch::release_device_resources_()
{
    m_pLightSlateGrayBrush = nullptr;
    m_pCornflowerBlueBrush = nullptr;

    return;
}

//------------------------------------------------------------------------------
LRESULT quetzal::CrossStitch::message_handler_(UINT message, WPARAM wParam, LPARAM lParam)
{
    // return 0 if message is handled ...

    return 1;
}

//------------------------------------------------------------------------------
void quetzal::CrossStitch::render_()
{
    assert(m_pRenderTarget != nullptr);
    assert(m_pLightSlateGrayBrush != nullptr);
    assert(m_pCornflowerBlueBrush != nullptr);

    m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
    m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

    D2D1_SIZE_F size = m_pRenderTarget->GetSize();

    // Draw a grid background.
    int w = static_cast<int>(size.width);
    int h = static_cast<int>(size.height);

    for (int x = 0; x < w; x += 10)
    {
        m_pRenderTarget->DrawLine(D2D1::Point2F(static_cast<FLOAT>(x), 0.0f), D2D1::Point2F(static_cast<FLOAT>(x), size.height), m_pLightSlateGrayBrush, 0.5f);
    }

    for (int y = 0; y < h; y += 10)
    {
        m_pRenderTarget->DrawLine(D2D1::Point2F(0.0f, static_cast<FLOAT>(y)), D2D1::Point2F(size.width, static_cast<FLOAT>(y)), m_pLightSlateGrayBrush, 0.5f);
    }

    // Draw two rectangles.
    D2D1_RECT_F rectangle1 = D2D1::RectF(size.width / 2 - 50.0f, size.height / 2 - 50.0f, size.width / 2 + 50.0f, size.height / 2 + 50.0f);
    D2D1_RECT_F rectangle2 = D2D1::RectF(size.width / 2 - 100.0f, size.height / 2 - 100.0f, size.width / 2 + 100.0f, size.height / 2 + 100.0f);

    // Draw a filled rectangle.
    m_pRenderTarget->FillRectangle(&rectangle1, m_pLightSlateGrayBrush);

    // Draw the outline of a rectangle.
    m_pRenderTarget->DrawRectangle(&rectangle2, m_pCornflowerBlueBrush);

    return;
}
