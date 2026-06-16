#if !defined(QUETZAL_CUBES_APPLICATION_HPP)
#define QUETZAL_CUBES_APPLICATION_HPP
//------------------------------------------------------------------------------
// Cubes
// Application.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/Config.hpp"
#include "quetzal/game/Application.hpp"
#include "quetzal/game/Camera.hpp"
#include "quetzal/vulkan/Renderer.hpp"
#include "quetzal/vulkan/Window.hpp"
#include "quetzal/scene_graph/AssetManager.hpp"
#include "quetzal/scene_graph/Scene.hpp"
#include <glm/glm.hpp>
#include <array>

namespace quetzal::cubes
{

    //--------------------------------------------------------------------------
    class Application : public game::Application
    {
    public:

        Application(const Config& config);
        Application(const Application&) = delete;
        Application(Application&&) = delete;
        ~Application();

        Application& operator=(const Application&) = delete;
        Application& operator=(Application&&) = delete;

    private:

        void update(float dt) override;
        void draw() override;

        virtual void on_key(GLFWwindow* pwindow, int key, int scancode, int action, int mods) override;
        virtual void on_char(GLFWwindow* pwindow, unsigned int codepoint) override;
        virtual void on_cursor_position(GLFWwindow* pwindow, double x, double y) override;
        virtual void on_cursor_enter(GLFWwindow* pwindow, int entered) override;
        virtual void on_mouse_button(GLFWwindow* pwindow, int button, int action, int mods) override;
        virtual void on_mouse_scroll(GLFWwindow* pwindow, double xoffset, double yoffset) override;

        void update_camera(float dt);

        vulkan::Window m_window;
        vulkan::Renderer m_renderer;
        scene_graph::Scene m_scene;
        scene_graph::AssetManager m_assets;
        game::Camera m_camera;

        static const std::array<glm::vec4, 16> s_colors;
        size_t m_nColor;
        glm::vec4 m_color;
        bool m_bCycle;
        bool m_bSmooth;
        float m_tTransition;
        float m_dt;

        float m_speed;
        bool m_bOrthographic;
        float m_scale;
        bool m_bTracking;
        std::array<double, 2> m_cursor_prev;
    };

} // namespace quetzal::cubes

#endif // QUETZAL_CUBES_APPLICATION_HPP
