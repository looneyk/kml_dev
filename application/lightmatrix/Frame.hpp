#if !defined(QUETZAL_LIGHTMATRIX_FRAME_HPP)
#define QUETZAL_LIGHTMATRIX_FRAME_HPP
//------------------------------------------------------------------------------
// Light Matrix Simulation
// Frame.hpp
// Copyright (c) 2010-2017 Keith Michael Looney. All Rights Reserved.
//------------------------------------------------------------------------------

#include <vector>
#include <iosfwd>
#include "Pixel.hpp"

namespace quetzal::lightmatrix
{

    //----------------------------------------------------------------------
    class Frame
    {
    public:

        using Pixels = std::vector<Pixel>;

        Frame();
        Frame(int w, int h);
        Frame(const Frame&) = default;
        ~Frame() = default;

        Frame& operator=(const Frame&) = default;

        int width() const;
        int height() const;

        void set_size(int w, int h);

        const Pixel& pixel(int x, int y) const;
        Pixel& pixel(int x, int y);

        const Pixels& pixels() const;
        Pixels& pixels();

    private:

        int m_w;
        int m_h;

        Pixels m_pixels;
    };

    void blend(const Frame& fa, const Frame& fb, float t, Frame& f);
//    Frame lerp(const Frame& f0, const Frame& f1, float t);

    std::ostream& operator<<(std::ostream& os, const Frame& frame);

} // namespace quetzal::lightmatrix

#endif // QUETZAL_LIGHTMATRIX_FRAME_HPP
