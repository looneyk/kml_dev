#if !defined(QUETZAL_LIGHTMATRIX_VIEWPORT_HPP)
#define QUETZAL_LIGHTMATRIX_VIEWPORT_HPP
//------------------------------------------------------------------------------
// Light Matrix Simulation
// Viewport.hpp
//------------------------------------------------------------------------------

#include "types.hpp"

namespace quetzal::lightmatrix
{

    //--------------------------------------------------------------------------
    class Viewport
    {
    public:

        Viewport(const point_type& position = {}, const vector_type& size = {1.0f, 1.0f});
        Viewport(const Viewport&) = default;
        Viewport(Viewport&&) = default;
        ~Viewport() = default;

        Viewport& operator=(const Viewport&) = default;
        Viewport& operator=(Viewport&&) = default;

        point_type position() const;
        void set_position(const point_type& position);

        vector_type size() const;
        void set_size(const vector_type& size);

    private:

        point_type m_position;
        vector_type m_size;
    };

} // namespace quetzal::lightmatrix

#endif // QUETZAL_LIGHTMATRIX_VIEWPORT_HPP
