#if !defined(QUETZAL_LIGHTMATRIX_PANEL_HPP)
#define QUETZAL_LIGHTMATRIX_PANEL_HPP
//------------------------------------------------------------------------------
// Light Matrix Simulation
// Panel.hpp
//------------------------------------------------------------------------------

#include "Arrangement.hpp"
#include "types.hpp"

namespace quetzal::lightmatrix
{

    class Frame;

    //--------------------------------------------------------------------------
    class Panel
    {
    public:

        Panel(Arrangement arrangement = Arrangement::Rectangular, const vector_type& size = {}, float pixel_radius = 0.0f);
        Panel(const Panel&) = default;
        Panel(Panel&&) = default;
        ~Panel() = default;

        Panel& operator=(const Panel&) = default;
        Panel& operator=(Panel&&) = default;

        Arrangement arrangement() const;
        void set_arrangement(Arrangement arrangement);

        vector_type size() const;
        void set_size(const vector_type& size);

        float pixel_radius() const;
        void set_pixel_radius(float radius);

        // Computes and stores each pixel's physical position and radius on frame, given frame's current pixel count
        void lay_out(Frame& frame) const;

    private:

        Arrangement m_arrangement;
        vector_type m_size;
        float m_pixel_radius;
    };

}

// namespace quetzal::lightmatrix

#endif // QUETZAL_LIGHTMATRIX_PANEL_HPP
