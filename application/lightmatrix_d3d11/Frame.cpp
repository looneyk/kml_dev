//------------------------------------------------------------------------------
// Light Matrix Simulation
// Frame.cpp
//------------------------------------------------------------------------------

#include "Frame.hpp"
#include <iostream>
#include <cassert>

using namespace std;

//------------------------------------------------------------------------------
quetzal::lightmatrix::Frame::Frame() :
    Frame(0, 0)
{
}

//------------------------------------------------------------------------------
quetzal::lightmatrix::Frame::Frame(size_t nx, size_t ny) :
    m_size{nx, ny},
    m_pixels(nx * ny)
{
}

//------------------------------------------------------------------------------
quetzal::lightmatrix::Frame::size_type quetzal::lightmatrix::Frame::size() const
{
    return m_size;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Frame::set_size(size_type size)
{
    m_size = size;
    m_pixels.resize(size[0] * size[1]);
    return;
}

//------------------------------------------------------------------------------
const quetzal::lightmatrix::Pixel& quetzal::lightmatrix::Frame::pixel(size_t x, size_t y) const
{
    assert(x < m_size[0]);
    assert(y < m_size[1]);

    return m_pixels[y * m_size[0] + x];
}

//------------------------------------------------------------------------------
quetzal::lightmatrix::Pixel& quetzal::lightmatrix::Frame::pixel(size_t x, size_t y)
{
    assert(x < m_size[0]);
    assert(y < m_size[1]);

    return m_pixels[y * m_size[0] + x];
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
/*
//------------------------------------------------------------------------------
quetzal::lightmatrix::Frame quetzal::lightmatrix::lerp(const Frame& frame0, const Frame& frame1, float t)
{
    assert(frame0.pixels().size() == frame1.pixels().size());

    Frame frame(frame0.width(), frame0.height());

    ranges::transform(frame0.pixels(), frame1.pixels(), frame.pixels().begin(), [t](const Pixel& a, const Pixel& b) { return ...

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
    const auto [ni, nj] = frame.size();

    for (size_t j = 0; j < nj; ++j)
    {
        for (size_t i = 0; i < ni; ++i)
        {
            os << frame.pixel(i, j);
            os << ((i < ni - 1) ? " " : "\n");
        }
    }

    return os;
}
