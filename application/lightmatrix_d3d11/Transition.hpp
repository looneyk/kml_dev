#if !defined(QUETZAL_LIGHTMATRIX_TRANSITION_HPP)
#define QUETZAL_LIGHTMATRIX_TRANSITION_HPP
//------------------------------------------------------------------------------
// Light Matrix Simulation
// Transition.hpp
//------------------------------------------------------------------------------

namespace quetzal::lightmatrix
{

    //----------------------------------------------------------------------
    class Transition
    {
    public:

        enum Type
        {
            Cut,
            Dissolve
        };

        Transition();
        Transition(const Transition&) = default;
        Transition(Transition&&) = default;
        ~Transition() = default;

        Transition& operator=(const Transition&) = default;
        Transition& operator=(Transition&&) = default;

        Type type() const;
        void set_type(Type type);

        float duration() const;
        void set_duration(float duration);

        float progress() const;

        bool transitioning() const;
        void set_transitioning(bool activated);

        // Returns true if the transition ended this call, so patterns can be cleaned up
        bool update(float dt);

    private:

        Type m_type;
        float m_duration;
        float m_time;
        bool m_transitioning;
    };

} // namespace quetzal::lightmatrix

#endif // QUETZAL_LIGHTMATRIX_TRANSITION_HPP
