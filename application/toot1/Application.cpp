//------------------------------------------------------------------------------
// quetzal toot1
// Application.cpp
//------------------------------------------------------------------------------

#include "Application.hpp"
#include "quetzal/scene_graph/LightNode.hpp"
#include "quetzal/game/types.hpp"
#include "quetzal/math/math_util.hpp"
#include "quetzal/vulkan/Vertex.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <ranges>
#include <cassert>

#include <iostream>

using namespace std;

const std::vector<std::string> model_paths =
{
    "viking_room.obj",
    "tuskified_spikosahedron.obj",
};

const std::vector<std::string> texture_paths =
{
    "viking_room.png",
    "142LL_1.png",
};

//------------------------------------------------------------------------------
quetzal::toot1::Application::Application(const quetzal::Config& config) :
    game::Application(m_window),
    m_window(config),
    m_renderer(config, m_window),
    m_scene(m_renderer),
    m_assets(m_renderer, config["assets_path"], config["shared_assets_path"]),
    m_shaders(config["shaders_path"]),
    m_camera(math::PiThird<float>),
    m_speed(1.0f),
    m_bTracking(false),
    m_cursor_prev(m_window.get_cursor_position())
{
    m_window.set_user_pointer(this);
    set_window_event_interface();

    m_renderer.create();

    scene_graph::load<vulkan::VertexPositionColorTexcoord>(m_scene, m_renderer, m_assets, "0", model_paths[0], texture_paths[0], glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, 0.0f)), m_shaders, "pct");
    scene_graph::load<vulkan::VertexPositionNormalTexcoord>(m_scene, m_renderer, m_assets, "1", model_paths[1], texture_paths[1], glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.01f, 0.01f, 0.01f)), m_shaders, "pnt");

    auto light_transform = std::make_shared<scene_graph::TransformNode>("light0_transform", glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 3.0f, 3.0f)));
    scene_graph::Node& light_xform_ref = m_scene.insert(m_scene.root(), std::move(light_transform));
    m_scene.insert(light_xform_ref, std::make_shared<scene_graph::LightNode>("light0", glm::vec3(1.0f, 1.0f, 1.0f), 25.0f));

    m_camera.set_position({2.0f, 2.0f, 2.0f, 1.0f});
    m_camera.set_forward({-1.0f, -1.0f, -1.0f});
}

//------------------------------------------------------------------------------
quetzal::toot1::Application::~Application()
{
    m_renderer.destroy();
}

//------------------------------------------------------------------------------
void quetzal::toot1::Application::update(float dt)
{
    // update scene

    const float omega = 0.25f * game::PI;

    std::array<glm::mat4, 2> model_matrices =
    {
         glm::rotate(glm::mat4(1.0f), dt * omega, glm::vec3(0.0f, 0.0f, 1.0f)),
         glm::rotate(glm::mat4(1.0f), -dt * omega, glm::vec3(0.0f, 0.0f, 1.0f)),
    };

    // update view

    update_camera(dt);

    auto extent = m_renderer.extent();

    m_camera.update_view();
    m_camera.set_perspective(static_cast<float>(extent.width) / static_cast<float>(extent.height), 0.1f, 100.0f);

    m_scene.set_camera(m_camera.view(), m_camera.projection());

    std::array<std::string, 2> ids = {"0", "1"};

    for (const auto& e : std::views::zip(ids, model_matrices))
    {
        m_scene.update_model_instance(get<0>(e), get<1>(e));
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::toot1::Application::draw()
{
    m_scene.draw();
    return;
}

//------------------------------------------------------------------------------
void quetzal::toot1::Application::on_key(GLFWwindow* pwindow, int key, int scancode, int action, int mods)
{
    pwindow;
    key;
    scancode;
    action;
    mods;

    //std::cout << "on_key " << (char)key << std::endl;
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
void quetzal::toot1::Application::on_char(GLFWwindow* pwindow, unsigned int codepoint)
{
    pwindow;
    codepoint;
    return;
}

//------------------------------------------------------------------------------
void quetzal::toot1::Application::on_cursor_position(GLFWwindow* pwindow, double x, double y)
{
    pwindow;

    if (!m_bTracking)
    {
        return;
    }

    //std::cout << "on_cursor_position " << x << " " << y << std::endl;

    auto extent = m_renderer.extent();

    double dx = x - m_cursor_prev[0];
    double dy = y - m_cursor_prev[1];
    if (dx == 0.0 && dy == 0.0)
    {
        return;
    }

    float sensitivity = 0.5f; // missing factor of 2? ...
    float factor = sensitivity * m_camera.fov() / extent.width;

    float rx = factor * static_cast<float>(-dx);
    float ry = factor * static_cast<float>(-dy);
    //std::cout << "radians " << rx << " " << ry << std::endl;

    m_camera.pan(rx);
    m_camera.tilt(ry);
    m_camera.update_view();

    m_cursor_prev = {x, y};
    return;
}

//------------------------------------------------------------------------------
void quetzal::toot1::Application::on_cursor_enter(GLFWwindow* pwindow, int entered)
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
void quetzal::toot1::Application::on_mouse_button(GLFWwindow* pwindow, int button, int action, int mods)
{
    pwindow;
    button;
    action;
    mods;
    return;
}

//------------------------------------------------------------------------------
void quetzal::toot1::Application::on_mouse_scroll(GLFWwindow* pwindow, double xoffset, double yoffset)
{
    pwindow;
    xoffset;
    yoffset;
    return;
}

//------------------------------------------------------------------------------
void quetzal::toot1::Application::update_camera(float dt)
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
