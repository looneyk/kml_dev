//------------------------------------------------------------------------------
// Light Matrix Simulation
// Frame.cpp
// Copyright (c) 2010-2017 Keith Michael Looney. All Rights Reserved.
//------------------------------------------------------------------------------

#include <iostream>
#include <cassert>
#include "Frame.hpp"

using namespace std;

//------------------------------------------------------------------------------
quetzal::lightmatrix::Frame::Frame() :
    Frame(0, 0)
{
}

//------------------------------------------------------------------------------
quetzal::lightmatrix::Frame::Frame(int w, int h) :
    m_w(w),
    m_h(h),
    m_pixels(w * h)
{
}

//------------------------------------------------------------------------------
int quetzal::lightmatrix::Frame::width() const
{
    return m_w;
}

//------------------------------------------------------------------------------
int quetzal::lightmatrix::Frame::height() const
{
    return m_h;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Frame::set_size(int w, int h)
{
    m_w = w;
    m_h = h;

    m_pixels.resize(m_w * m_h);

    return;
}

//------------------------------------------------------------------------------
const quetzal::lightmatrix::Pixel& quetzal::lightmatrix::Frame::pixel(int x, int y) const
{
    assert(x < m_w);
    assert(y < m_h);

    return m_pixels[y * m_w + x];
}

//------------------------------------------------------------------------------
quetzal::lightmatrix::Pixel& quetzal::lightmatrix::Frame::pixel(int x, int y)
{
    assert(x < m_w);
    assert(y < m_h);

    return m_pixels[y * m_w + x];
}

//------------------------------------------------------------------------------
const quetzal::lightmatrix::Frame::Pixels& quetzal::lightmatrix::Frame::pixels() const
{
    return m_pixels;
}

//------------------------------------------------------------------------------
quetzal::lightmatrix::Frame::Pixels& quetzal::lightmatrix::Frame::pixels()
{
    return m_pixels;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::blend(const Frame& fa, const Frame& fb, float t, Frame& f)
{
    assert(fa.pixels().size() == f.pixels().size());
    assert(fb.pixels().size() == f.pixels().size());

    float wa = 1.0f - t;
    float wb = t;

    auto ia = fa.pixels().begin();
    auto ib = fb.pixels().begin();

    for (Pixel& p : f.pixels())
    {
        Color ca = ia->color();
        Color cb = ib->color();

        p.set_color({
            ca.r() * ca.a() * wa + cb.r() * cb.a() * wb,
            ca.g() * ca.a() * wa + cb.g() * cb.a() * wb,
            ca.b() * ca.a() * wa + cb.b() * cb.a() * wb,
            ca.a() * wa + cb.a() * wb}); // weighted average is not the right operation here ...

        ++ia;
        ++ib;
    }

    return;
}
/*
//------------------------------------------------------------------------------
quetzal::lightmatrix::Frame quetzal::lightmatrix::lerp(const Frame& f0, const Frame& f1, float t)
{
    assert(frame0.pixels().size() == frame1.pixels().size());

    Frame frame(f0.width(), f0.height());

    ranges::transform(f0.pixels(), f1.pixels(), frame.pixels().begin(), [t](const Pixel& a, const Pixel& b) { return ...

    auto ia = frame0.pixels().begin();
    auto ib = frame1.pixels().begin();

    for (Pixel& p : frame.pixels())
    {
        Color ca = ia->color();
        Color cb = ib->color();

        p.set_color({
            ca.r() * ca.a() * wa + cb.r() * cb.a() * wb,
            ca.g() * ca.a() * wa + cb.g() * cb.a() * wb,
            ca.b() * ca.a() * wa + cb.b() * cb.a() * wb,
            ca.a() * wa + cb.a() * wb}); // weighted average is not the right operation here ...

        ++ia;
        ++ib;
    }

    return frame;
}
*/
//------------------------------------------------------------------------------
ostream& quetzal::lightmatrix::operator<<(ostream& os, const Frame& frame)
{
    const int w = frame.width();
    const int h = frame.height();

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            os << frame.pixel(x, y);
            os << ((x < w - 1) ? " " : "\n");
        }
    }

    return os;
}
