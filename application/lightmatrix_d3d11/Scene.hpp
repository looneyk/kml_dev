#if !defined(QUETZAL_LIGHTMATRIX_SCENE_HPP)
#define QUETZAL_LIGHTMATRIX_SCENE_HPP
//------------------------------------------------------------------------------
// Light Matrix Simulation
// Scene.hpp
//------------------------------------------------------------------------------

#include "Animation.hpp"
#include "Frame.hpp"
#include "Panel.hpp"
#include "Pattern.hpp"
#include "Transition.hpp"
#include "Viewport.hpp"
#include <array>

namespace quetzal::lightmatrix
{

    class Application;

    //--------------------------------------------------------------------------
    class Scene
    {
    public:

        using patterns_type = std::array<Pattern, 2>;

        Scene();
        Scene(const Scene&) = delete;
        Scene(Scene&&) = delete;
        ~Scene() = default;

        Scene& operator=(const Scene&) = delete;
        Scene& operator=(Scene&&) = delete;

        const Panel& panel() const;
        Panel& panel();

        const Frame& frame() const;
        Frame& frame();

        const patterns_type& patterns() const;
        void set_pattern(Pattern&& pattern);
        void set_transition_pattern(Pattern&& pattern);

        const Viewport& viewport() const;
        Viewport& viewport();

        const Animation& animation() const;
        Animation& animation();

        const Transition& transition() const;
        Transition& transition();

        void update(float dt);

    private:

        void paint_frame(bool bRgb, bool bAlpha);

        Panel m_panel;
        Frame m_frame;
        patterns_type m_patterns;
        Viewport m_viewport;
        Animation m_animation;
        Transition m_transition;
    };

} // namespace quetzal::lightmatrix

#endif // QUETZAL_LIGHTMATRIX_SCENE_HPP
