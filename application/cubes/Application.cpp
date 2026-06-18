//------------------------------------------------------------------------------
// Cubes
// Application.cpp
//------------------------------------------------------------------------------

#include "Application.hpp"
#include "quetzal/math/math_util.hpp"
#include "quetzal/scene_graph/GltfLoader.hpp"

using namespace std;

const std::array<glm::vec4, 16> quetzal::cubes::Application::s_colors =
{{
    {1.0f, 0.0f, 0.0f, 1.0f},
    {1.0f, 0.2f, 0.0f, 1.0f},
    {1.0f, 0.4f, 0.0f, 1.0f},
    {1.0f, 0.6f, 0.0f, 1.0f},
    {1.0f, 0.8f, 0.0f, 1.0f},
    {1.0f, 1.0f, 0.0f, 1.0f},
    {0.5f, 1.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 0.5f, 1.0f},
    {0.0f, 1.0f, 1.0f, 1.0f},
    {0.0f, 0.5f, 1.0f, 1.0f},
    {0.0f, 0.0f, 1.0f, 1.0f},
    {0.4f, 0.0f, 1.0f, 1.0f},
    {0.6f, 0.0f, 1.0f, 1.0f},
    {1.0f, 0.0f, 1.0f, 1.0f},
    {1.0f, 0.0f, 0.5f, 1.0f},
}};

//------------------------------------------------------------------------------
quetzal::cubes::Application::Application(const quetzal::Config& config) :
    game::Application(m_window),
    m_window(config),
    m_renderer(config, m_window),
    m_scene(m_renderer),
    m_assets(m_renderer, config["assets_path"], config["shared_assets_path"]),
    m_shaders(config["shaders_path"]),
    m_camera(game::Camera::ViewMode::Fixed, math::PiThird<float>),
    m_nColor(11),
    m_color(s_colors[11]),
    m_bCycle(false),
    m_bSmooth(false),
    m_tTransition(0.15f),
    m_dt(0.0f),
    m_speed(6.0f),
    m_bOrthographic(false),
    m_scale(0.03f),
    m_bTracking(false),
    m_cursor_prev(m_window.get_cursor_position())
{
    m_window.set_user_pointer(this);
    set_window_event_interface();

    m_renderer.create();

    scene_graph::load_gltf(m_scene, m_scene.root(), m_renderer, m_assets, m_assets.shared_assets_path() / "models" / "axes.gltf", m_shaders);
    scene_graph::load_gltf(m_scene, m_scene.root(), m_renderer, m_assets, m_assets.assets_path() / "models" / "cubes.gltf", m_shaders);

    m_camera.set_up({0.0f, 0.0f, 1.0f});
    m_camera.set_position({12.0f, 12.0f, 10.0f, 1.0f});
    m_camera.set_forward({-12.0f, -12.0f, -6.5f});
    m_camera.update_view();
}

//------------------------------------------------------------------------------
quetzal::cubes::Application::~Application()
{
    m_renderer.destroy();
}

//------------------------------------------------------------------------------
void quetzal::cubes::Application::update(float dt)
{
    m_dt += dt;

    while (m_dt >= m_tTransition)
    {
        if (m_bCycle)
        {
            m_nColor = (m_nColor + 1) % s_colors.size();
        }

        m_dt -= m_tTransition;
    }

    if (m_bCycle)
    {
        if (m_bSmooth)
        {
            size_t nColor1 = (m_nColor + 1) % s_colors.size();
            float t = m_dt / m_tTransition;
            m_color = s_colors[m_nColor] * (1.0f - t) + s_colors[nColor1] * t;
        }
        else
        {
            m_color = s_colors[m_nColor];
        }
    }

    update_camera(dt);

    auto extent = m_renderer.extent();
    float w = static_cast<float>(extent.width);
    float h = static_cast<float>(extent.height);

    m_camera.update_view();

    if (m_bOrthographic)
    {
        m_camera.set_orthographic(w * m_scale, h * m_scale, 0.1f, 1000.0f);
    }
    else
    {
        m_camera.set_perspective(w / h, 0.1f, 1000.0f);
    }

    m_scene.set_camera(m_camera.view(), m_camera.projection());
    return;
}

//------------------------------------------------------------------------------
void quetzal::cubes::Application::draw()
{
    m_scene.draw();
    return;
}

//------------------------------------------------------------------------------
void quetzal::cubes::Application::on_key(GLFWwindow* pwindow, int key, int scancode, int action, int mods)
{
    pwindow;
    scancode;
    mods;

    if (action != GLFW_PRESS)
    {
        return;
    }

    switch (key)
    {
        case GLFW_KEY_ESCAPE:
            m_window.set_close();
            break;

        case GLFW_KEY_C:
            m_bCycle = !m_bCycle;
            break;

        case GLFW_KEY_M:
            m_bSmooth = !m_bSmooth;
            break;

        case GLFW_KEY_G:
            m_bOrthographic = !m_bOrthographic;
            m_scale = 0.03f;
            break;
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::cubes::Application::on_char(GLFWwindow* pwindow, unsigned int codepoint)
{
    pwindow;
    codepoint;
    return;
}

//------------------------------------------------------------------------------
void quetzal::cubes::Application::on_cursor_position(GLFWwindow* pwindow, double x, double y)
{
    pwindow;

    if (!m_bTracking)
    {
        return;
    }

    double dx = x - m_cursor_prev[0];
    double dy = y - m_cursor_prev[1];
    if (dx == 0.0 && dy == 0.0)
    {
        return;
    }

    auto extent = m_renderer.extent();
    float factor = 0.5f * m_camera.fov() / static_cast<float>(extent.width);

    m_camera.pan(factor * static_cast<float>(-dx));
    m_camera.tilt(factor * static_cast<float>(-dy));
    m_camera.update_view();

    m_cursor_prev = {x, y};
    return;
}

//------------------------------------------------------------------------------
void quetzal::cubes::Application::on_cursor_enter(GLFWwindow* pwindow, int entered)
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
void quetzal::cubes::Application::on_mouse_button(GLFWwindow* pwindow, int button, int action, int mods)
{
    pwindow;
    button;
    action;
    mods;
    return;
}

//------------------------------------------------------------------------------
void quetzal::cubes::Application::on_mouse_scroll(GLFWwindow* pwindow, double xoffset, double yoffset)
{
    pwindow;
    xoffset;
    yoffset;
    return;
}

//------------------------------------------------------------------------------
void quetzal::cubes::Application::update_camera(float dt)
{
    glm::vec4 position = m_camera.position();
    float speed = dt * m_speed;

    if (m_window.get_key_state(GLFW_KEY_W))
    {
        if (m_bOrthographic)
        {
            position += speed * glm::vec4(m_camera.up(), 0.0f);
        }
        else
        {
            position += speed * glm::vec4(m_camera.forward(), 0.0f);
        }
    }

    if (m_window.get_key_state(GLFW_KEY_S))
    {
        if (m_bOrthographic)
        {
            position -= speed * glm::vec4(m_camera.up(), 0.0f);
        }
        else
        {
            position -= speed * glm::vec4(m_camera.forward(), 0.0f);
        }
    }

    if (m_window.get_key_state(GLFW_KEY_D))
    {
        position += speed * glm::vec4(m_camera.right(), 0.0f);
    }

    if (m_window.get_key_state(GLFW_KEY_A))
    {
        position -= speed * glm::vec4(m_camera.right(), 0.0f);
    }

    if (m_bOrthographic)
    {
        if (m_window.get_key_state(GLFW_KEY_Q))
        {
            m_scale -= m_speed / 500.0f * dt;
        }

        if (m_window.get_key_state(GLFW_KEY_Z))
        {
            m_scale += m_speed / 500.0f * dt;
        }
    }

    m_camera.set_position(position);
    return;
}
