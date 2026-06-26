#if !defined(QUETZAL_LIGHTMATRIX_APPLICATION_HPP)
#define QUETZAL_LIGHTMATRIX_APPLICATION_HPP
//------------------------------------------------------------------------------
// Light Matrix Simulation
// Application.hpp
//------------------------------------------------------------------------------

#include "Scene.hpp"
#include "quetzal/common/Config.hpp"
#include "quetzal/scene_graph/ShaderManager.hpp"
#include "quetzal/game/Application.hpp"
#include "quetzal/game/Camera.hpp"
#include "quetzal/vulkan/Renderer.hpp"
#include "quetzal/vulkan/Window.hpp"
#include "quetzal/scene_graph/AssetManager.hpp"
#include "quetzal/scene_graph/Scene.hpp"
#include "quetzal/scene_graph/TransformNode.hpp"
#include "quetzal/scene_graph/MaterialNode.hpp"
#include <array>

namespace quetzal::lightmatrix
{

    //----------------------------------------------------------------------
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

        void check_input(float dt);
        void update_camera(float dt);

        void build_pixels();
        void update_pixels();

        void set_pattern(int nPattern);

        vulkan::Window m_window;
        vulkan::Renderer m_renderer;
        scene_graph::Scene m_scene_graph;
        scene_graph::AssetManager m_assets;
        scene_graph::ShaderManager m_shaders;
        game::Camera m_camera;

        Scene m_scene;

        vulkan::GeometryHandle m_geometry_pixel;
        vulkan::MaterialHandle m_material_pixel;
        vulkan::TextureHandle m_texture_pixel;
        std::vector<scene_graph::TransformNode*> m_pixel_transforms;
        std::vector<scene_graph::MaterialNode*> m_pixel_materials;

        float m_speed;
        bool m_bTracking;
        std::array<double, 2> m_cursor_prev;
    };

} // namespace quetzal::lightmatrix

#endif // QUETZAL_LIGHTMATRIX_APPLICATION_HPP
