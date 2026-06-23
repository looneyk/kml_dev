#if !defined(QUETZAL_LIGHTMATRIX_FRAME_HPP)
#define QUETZAL_LIGHTMATRIX_FRAME_HPP
//------------------------------------------------------------------------------
// Light Matrix Simulation
// Frame.hpp
//------------------------------------------------------------------------------

#include "Pixel.hpp"
#include <array>
#include <iosfwd>
#include <vector>

namespace quetzal::lightmatrix
{

    //----------------------------------------------------------------------
    class Frame
    {
    public:

        using size_type = std::array<size_t, 2>;
        using Pixels = std::vector<Pixel>;

        Frame();
        Frame(size_t nx, size_t ny);
        Frame(const Frame&) = default;
        Frame(Frame&&) = default;
        ~Frame() = default;

        Frame& operator=(const Frame&) = default;
        Frame& operator=(Frame&&) = default;

        size_type size() const;
        void set_size(size_type size);

        const Pixel& pixel(size_t x, size_t y) const;
        Pixel& pixel(size_t x, size_t y);

        const Pixels& pixels() const;
        Pixels& pixels();

    private:

        size_type m_size;
        Pixels m_pixels;
    };

//    Frame lerp(const Frame& frame0, const Frame& frame1, float t);

    std::ostream& operator<<(std::ostream& os, const Frame& frame);

} // namespace quetzal::lightmatrix

#endif // QUETZAL_LIGHTMATRIX_FRAME_HPP
