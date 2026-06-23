#if !defined(QUETZAL_LIGHTMATRIX_ANIMATION_HPP)
#define QUETZAL_LIGHTMATRIX_ANIMATION_HPP
//------------------------------------------------------------------------------
// Light Matrix Simulation
// Animation.hpp
//------------------------------------------------------------------------------

#include "Smoothing.hpp"
#include "types.hpp"

namespace quetzal::lightmatrix
{

    //----------------------------------------------------------------------
    class Animation
    {
    public:

        Animation();
        Animation(const Animation&) = default;
        Animation(Animation&&) = default;
        ~Animation() = default;

        Animation& operator=(const Animation&) = default;
        Animation& operator=(Animation&&) = default;

        const bool shift_color() const;
        void set_shift_color(bool shift);

        const bool shift_brightness() const;
        void set_shift_brightness(bool shift);

        const Smoothing smoothing() const;
        void set_smoothing(Smoothing smoothing);

        const vector_type velocity() const; // magnitude
        void set_velocity_default(const vector_type& v); // deprecayed, stored in application? need to rework where files are parsed to ...
        void set_velocity(const vector_type& v);
        void increment_velocity(const vector_type& dv);

        const vector_type direction() const;
        void set_direction(const vector_type& v);

        const bool animating() const;
        void set_animating(bool activated);

    private:

        bool m_shift_color;
        bool m_shift_brightness;
        Smoothing m_smoothing;
        vector_type m_velocity_orig;
        vector_type m_velocity;
        vector_type m_direction;
        bool m_animating;
    };

} // namespace quetzal::lightmatrix

#endif // QUETZAL_LIGHTMATRIX_ANIMATION_HPP
