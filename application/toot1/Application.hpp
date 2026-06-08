#if !defined(QUETZAL_TOOT1_APPLICATION_HPP)
#define QUETZAL_TOOT1_APPLICATION_HPP
//------------------------------------------------------------------------------
// quetzal toot1
// Application.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/Config.hpp"
#include "quetzal/game/Application.hpp"
#include "quetzal/game/Camera.hpp"
#include "quetzal/vulkan/Renderer.hpp"
#include "quetzal/vulkan/Window.hpp"
#include "quetzal/scene_graph/Scene.hpp"

namespace quetzal::toot1
{

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

        // Input callbacks
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
        game::Camera m_camera;

        float m_speed;
        bool m_bTracking;
        std::array<double, 2> m_cursor_prev;
    };

} // namespace quetzal::toot1

#endif // QUETZAL_TOOT1_APPLICATION_HPP
