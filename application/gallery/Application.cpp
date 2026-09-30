//------------------------------------------------------------------------------
// quetzal gallery
// Application.cpp
//------------------------------------------------------------------------------

#include "Application.hpp"
#include "quetzal/scene_graph/LightNode.hpp"
#include "quetzal/game/types.hpp"
#include "quetzal/math/math_util.hpp"
#include "quetzal/scene_graph/GltfLoader.hpp"
#include "quetzal/vulkan/Vertex.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <cassert>

using namespace std;

//------------------------------------------------------------------------------
quetzal::gallery::Application::Application(const quetzal::Config& config) :
    game::Application(m_window),
    m_window(config),
    m_renderer(config, m_window),
    m_scene(m_renderer),
    m_assets(m_renderer, config.value_or("assets_path", ""), config.value_or("shared_assets_path", "")),
    m_shaders(config.value_or("shaders_path", "")),
    m_camera(game::Camera::ViewMode::Fixed, math::PiThird<float>),
    m_speed(200.0f),
    m_bTracking(false),
    m_cursor_prev(m_window.get_cursor_position())
{
    m_window.set_user_pointer(this);
    set_window_event_interface();

    m_renderer.create();

    scene_graph::load_gltf(m_scene, m_scene.root(), m_renderer, m_assets, m_assets.shared_assets_path() / "models" / "axes.gltf", m_shaders);
    scene_graph::load<vulkan::VertexPositionNormalTexcoord>(m_scene, m_renderer, m_assets, "floor", "floor.obj", "stones.png", glm::mat4(1.0f), m_shaders, "pnt");
    scene_graph::load<vulkan::VertexPositionNormalTexcoord>(m_scene, m_renderer, m_assets, "spikosahedron", "spikosahedron.obj", "142LL_1.png", glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 115.225f)), m_shaders, "pnt");

    auto light_transform = std::make_shared<scene_graph::TransformNode>("light0_transform", glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -500.0f, 800.0f)));
    scene_graph::Node& light_xform_ref = m_scene.insert(m_scene.root(), std::move(light_transform));
    m_scene.insert(light_xform_ref, std::make_shared<scene_graph::LightNode>("light0", glm::vec3(1.0f, 1.0f, 1.0f), 1000000.0f));

    m_camera.set_position({0.0f, -1000.0f, 500.0f, 1.0f});
    m_camera.set_forward({0.0f, 1.0f, -0.5f});
}

//------------------------------------------------------------------------------
quetzal::gallery::Application::~Application()
{
}

//------------------------------------------------------------------------------
void quetzal::gallery::Application::update(float dt)
{
    update_camera(dt);

    auto extent = m_renderer.extent();

    m_camera.update_view();
    m_camera.set_perspective(static_cast<float>(extent.width) / static_cast<float>(extent.height), 1.0f, 10000.0f);

    m_scene.set_camera(m_camera.view(), m_camera.projection());

    return;
}

//------------------------------------------------------------------------------
void quetzal::gallery::Application::draw()
{
    m_scene.draw();
    return;
}

//------------------------------------------------------------------------------
void quetzal::gallery::Application::on_key(GLFWwindow* pwindow, int key, int scancode, int action, int mods)
{
    pwindow;
    key;
    scancode;
    action;
    mods;

    switch (key)
    {
        case GLFW_KEY_ESCAPE:
            m_window.set_close();
            break;

        case GLFW_KEY_UP:
            m_speed += 0.1f;
            break;

        case GLFW_KEY_DOWN:
            if (m_speed <= 0.1f)
            {
                m_speed = 0.0f;
            }
            else
            {
                m_speed -= 0.1f;
            }
            break;
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::gallery::Application::on_char(GLFWwindow* pwindow, unsigned int codepoint)
{
    pwindow;
    codepoint;
    return;
}

//------------------------------------------------------------------------------
void quetzal::gallery::Application::on_cursor_position(GLFWwindow* pwindow, double x, double y)
{
    pwindow;

    if (!m_bTracking)
    {
        return;
    }

    auto extent = m_renderer.extent();

    double dx = x - m_cursor_prev[0];
    double dy = y - m_cursor_prev[1];
    if (dx == 0.0 && dy == 0.0)
    {
        return;
    }

    float sensitivity = 0.5f;
    float factor = sensitivity * m_camera.fov() / extent.width;

    float rx = factor * static_cast<float>(-dx);
    float ry = factor * static_cast<float>(-dy);

    m_camera.pan(rx);
    m_camera.tilt(ry);
    m_camera.update_view();

    m_cursor_prev = {x, y};
    return;
}

//------------------------------------------------------------------------------
void quetzal::gallery::Application::on_cursor_enter(GLFWwindow* pwindow, int entered)
{
    pwindow;

    m_bTracking = entered;
    if (m_bTracking)
    {
        m_cursor_prev = m_window.get_cursor_position();
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::gallery::Application::on_mouse_button(GLFWwindow* pwindow, int button, int action, int mods)
{
    pwindow;
    button;
    action;
    mods;
    return;
}

//------------------------------------------------------------------------------
void quetzal::gallery::Application::on_mouse_scroll(GLFWwindow* pwindow, double xoffset, double yoffset)
{
    pwindow;
    xoffset;
    yoffset;
    return;
}

//------------------------------------------------------------------------------
void quetzal::gallery::Application::update_camera(float dt)
{
    glm::vec4 position = m_camera.position();

    if (m_window.get_key_state(GLFW_KEY_W))
    {
        position += dt * m_speed * glm::vec4(m_camera.forward(), 1.0f);
    }

    if (m_window.get_key_state(GLFW_KEY_S))
    {
        position += -dt * m_speed * glm::vec4(m_camera.forward(), 1.0f);
    }

    if (m_window.get_key_state(GLFW_KEY_D))
    {
        position += dt * m_speed * glm::vec4(m_camera.right(), 1.0f);
    }

    if (m_window.get_key_state(GLFW_KEY_A))
    {
        position += -dt * m_speed * glm::vec4(m_camera.right(), 1.0f);
    }

    m_camera.set_position(position);

    return;
}
