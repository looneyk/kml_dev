//------------------------------------------------------------------------------
// Light Matrix Simulation
// Application.cpp
//------------------------------------------------------------------------------

#include "Application.hpp"
#include "quetzal/scene_graph/GeometryNode.hpp"
#include "quetzal/vulkan/Vertex.hpp"
#include "quetzal/math/math_util.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <filesystem>
#include <map>
#include <string>
#include <vector>
#include <cassert>

using namespace std;

namespace
{

    const std::string s_patterns_path = "patterns/";

    const vector<string> s_pattern_names =
    {
        "pattern0.xml",
        "pattern1.xml",
        "pattern2.xml",
        "pattern3.xml",
        "pattern4.xml",
        "pattern5.xml",
        "pattern6.xml",
        "pattern7.xml",
    };

    std::map<quetzal::lightmatrix::Smoothing, quetzal::lightmatrix::Smoothing> next_smoothing =
    {
        {quetzal::lightmatrix::Smoothing::Nearest, quetzal::lightmatrix::Smoothing::Linear},
        {quetzal::lightmatrix::Smoothing::Linear, quetzal::lightmatrix::Smoothing::Cubic},
        {quetzal::lightmatrix::Smoothing::Cubic, quetzal::lightmatrix::Smoothing::Nearest},
    };

    using V = quetzal::vulkan::VertexPositionNormalTexcoord;

} // namespace

//------------------------------------------------------------------------------
quetzal::lightmatrix::Application::Application(const quetzal::Config& config) :
    game::Application(m_window),
    m_window(config),
    m_renderer(config, m_window),
    m_scene_graph(m_renderer),
    m_assets(m_renderer, config["assets_path"], config["shared_assets_path"]),
    m_shaders(config["shaders_path"]),
    m_camera(game::Camera::ViewMode::Fixed),
    m_scene(),
    m_geometry_pixel(),
    m_material_pixel(),
    m_pixel_transforms(),
    m_pixel_materials(),
    m_speed(100.0f),
    m_bTracking(false),
    m_cursor_prev(m_window.get_cursor_position())
{
    m_window.set_user_pointer(this);
    set_window_event_interface();

    m_renderer.create();

    scene_graph::MeshData mesh_data = m_assets.load_mesh("unit_icosahedron.obj");
    std::vector<V> vertices;
    vertices.reserve(mesh_data.positions.size());
    for (size_t i = 0; i < mesh_data.positions.size(); ++i)
    {
        vertices.push_back({
            {mesh_data.positions[i][0], mesh_data.positions[i][1], mesh_data.positions[i][2]},
            mesh_data.normals.empty() ? glm::vec3{0.0f, 0.0f, 1.0f} : glm::vec3{mesh_data.normals[i][0], mesh_data.normals[i][1], mesh_data.normals[i][2]},
            {0.0f, 0.0f}
        });
    }
    m_geometry_pixel = m_renderer.upload_geometry<V>("pixel_geo", vertices, mesh_data.indices);

    static const uint32_t white = 0xFFFFFFFF;
    vulkan::TextureData white_td{VK_FORMAT_R8G8B8A8_SRGB, {{&white, 4, 1u, 1u}}};
    vulkan::TextureHandle texture = m_renderer.upload_texture("__default_white", white_td);
    vulkan::EffectHandle effect = m_renderer.create_effect(m_shaders.vert_path("tint").string(), m_shaders.frag_path("tint").string(), vulkan::vertex_binding<V>());
    m_material_pixel = m_renderer.create_material(effect, texture);

    m_scene.panel().set_size({400.0f, 400.0f});
    m_scene.panel().set_pixel_radius(2.5f);

    m_scene.frame().set_size({16, 16});
    m_scene.panel().lay_out(m_scene.frame());

    build_pixels();

    set_pattern(0);

    // The default forward/up are (0,1,0)/(0,0,1); setting up directly to (0,1,0) or
    // forward directly to (0,0,1) against those defaults yields a degenerate (parallel)
    // cross product. Route through a safe intermediate forward to avoid that.
    m_camera.set_forward({1.0f, 0.0f, 0.0f});
    m_camera.set_up({0.0f, 1.0f, 0.0f});
    m_camera.set_forward({0.0f, 0.0f, 1.0f});
    m_camera.set_position({0.0f, 0.0f, -500.0f, 1.0f});
    m_camera.set_fov(0.25f * game::PI);
}

//------------------------------------------------------------------------------
quetzal::lightmatrix::Application::~Application()
{
    m_renderer.destroy();
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Application::update(float dt)
{
    check_input(dt);

    m_scene.update(dt);
    update_pixels();

    update_camera(dt);

    auto extent = m_renderer.extent();

    m_camera.update_view();
    m_camera.set_perspective(static_cast<float>(extent.width) / static_cast<float>(extent.height), 1.0f, 5000.0f);

    m_scene_graph.set_camera(m_camera.view(), m_camera.projection());

    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Application::draw()
{
    m_scene_graph.draw();
    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Application::build_pixels()
{
    const auto [nx, ny] = m_scene.frame().size();

    m_pixel_transforms.reserve(nx * ny);
    m_pixel_materials.reserve(nx * ny);

    for (size_t j = 0; j < ny; ++j)
    {
        for (size_t i = 0; i < nx; ++i)
        {
            std::string id = "px_" + std::to_string(i) + "_" + std::to_string(j);

            auto& transform_ref = static_cast<scene_graph::TransformNode&>(m_scene_graph.insert(m_scene_graph.root(), std::make_shared<scene_graph::TransformNode>(id)));
            auto& material_ref = static_cast<scene_graph::MaterialNode&>(m_scene_graph.insert(transform_ref, std::make_shared<scene_graph::MaterialNode>(id + "_mat", m_material_pixel)));
            m_scene_graph.insert(material_ref, std::make_shared<scene_graph::GeometryNode>(id + "_geo", m_geometry_pixel));

            m_pixel_transforms.push_back(&transform_ref);
            m_pixel_materials.push_back(&material_ref);
        }
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Application::update_pixels()
{
    const Frame& frame = m_scene.frame();
    const auto [nx, ny] = frame.size();

    for (size_t j = 0; j < ny; ++j)
    {
        for (size_t i = 0; i < nx; ++i)
        {
            const Pixel& pixel = frame.pixel(i, j);
            Color color = pixel.color();
            float r = pixel.radius();
            float x = pixel.position().x();
            float y = pixel.position().y();

            size_t k = j * nx + i;

            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(r, r, r));
            m_pixel_transforms[k]->set_local_transform(model);
            m_pixel_materials[k]->set_color(glm::vec4(color.r(), color.g(), color.b(), color.a()));
        }
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Application::on_key(GLFWwindow* pwindow, int key, int scancode, int action, int mods)
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

        // Layout

        case GLFW_KEY_Q:
            m_scene.panel().set_arrangement(Arrangement::Radial);
            m_scene.panel().lay_out(m_scene.frame());
            break;

        case GLFW_KEY_R:
            m_scene.panel().set_arrangement(Arrangement::Rectangular);
            m_scene.panel().lay_out(m_scene.frame());
            break;

        // Initial pattern

        case GLFW_KEY_1:
        case GLFW_KEY_2:
        case GLFW_KEY_3:
        case GLFW_KEY_4:
        case GLFW_KEY_5:
        case GLFW_KEY_6:
        case GLFW_KEY_7:
        case GLFW_KEY_8:
        case GLFW_KEY_9:
        case GLFW_KEY_0:
            set_pattern(key == GLFW_KEY_0 ? 9 : key - GLFW_KEY_1);
            break;

        // Brightness/color

        case GLFW_KEY_C:
            m_scene.animation().set_shift_color(!m_scene.animation().shift_color());
            break;

        case GLFW_KEY_B:
            m_scene.animation().set_shift_brightness(!m_scene.animation().shift_brightness());
            break;

        case GLFW_KEY_M:
            m_scene.animation().set_smoothing(next_smoothing[m_scene.animation().smoothing()]);
            break;

        // Animation direction

        case GLFW_KEY_KP_1:
            m_scene.animation().set_direction({-1.0f, -1.0f});
            break;

        case GLFW_KEY_KP_2:
            m_scene.animation().set_direction({0.0f, -1.0f});
            break;

        case GLFW_KEY_KP_3:
            m_scene.animation().set_direction({1.0f, -1.0f});
            break;

        case GLFW_KEY_KP_4:
            m_scene.animation().set_direction({-1.0f, 0.0f});
            break;

        case GLFW_KEY_KP_5:
            m_scene.animation().set_direction({0.0f, 0.0f});
            break;

        case GLFW_KEY_KP_6:
            m_scene.animation().set_direction({1.0f, 0.0f});
            break;

        case GLFW_KEY_KP_7:
            m_scene.animation().set_direction({-1.0f, 1.0f});
            break;

        case GLFW_KEY_KP_8:
            m_scene.animation().set_direction({0.0f, 1.0f});
            break;

        case GLFW_KEY_KP_9:
            m_scene.animation().set_direction({1.0f, 1.0f});
            break;

        // Rate

        case GLFW_KEY_LEFT:
            m_scene.animation().increment_velocity({-0.1f, 0.0f});
            break;

        case GLFW_KEY_RIGHT:
            m_scene.animation().increment_velocity({0.1f, 0.0f});
            break;

        case GLFW_KEY_DOWN:
            m_scene.animation().increment_velocity({0.0f, -0.1f});
            break;

        case GLFW_KEY_UP:
            m_scene.animation().increment_velocity({0.0f, 0.1f});
            break;
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Application::on_char(GLFWwindow* pwindow, unsigned int codepoint)
{
    pwindow;
    codepoint;
    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Application::on_cursor_position(GLFWwindow* pwindow, double x, double y)
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
void quetzal::lightmatrix::Application::on_cursor_enter(GLFWwindow* pwindow, int entered)
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
void quetzal::lightmatrix::Application::on_mouse_button(GLFWwindow* pwindow, int button, int action, int mods)
{
    pwindow;
    button;
    action;
    mods;
    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Application::on_mouse_scroll(GLFWwindow* pwindow, double xoffset, double yoffset)
{
    pwindow;
    xoffset;
    yoffset;
    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Application::check_input(float dt)
{
    dt;

    if (m_window.get_key_state(GLFW_KEY_EQUAL))
    {
        m_scene.animation().increment_velocity({0.1f, 0.1f});
    }

    if (m_window.get_key_state(GLFW_KEY_MINUS))
    {
        m_scene.animation().increment_velocity({-0.1f, -0.1f});
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Application::update_camera(float dt)
{
    glm::vec4 position = m_camera.position();

    if (m_window.get_key_state(GLFW_KEY_W))
    {
        position += dt * m_speed * glm::vec4(m_camera.forward(), 0.0f);
    }

    if (m_window.get_key_state(GLFW_KEY_S))
    {
        position += -dt * m_speed * glm::vec4(m_camera.forward(), 0.0f);
    }

    m_camera.set_position(position);

    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Application::set_pattern(int nPattern)
{
    if (nPattern < 0 || nPattern >= static_cast<int>(s_pattern_names.size()))
    {
        return;
    }

    std::filesystem::path pattern_path = s_pattern_names[nPattern];
    if (!s_patterns_path.empty())
    {
        pattern_path = s_patterns_path / pattern_path;
    }

    auto [pattern, panel, animation] = load_pattern_layout(pattern_path);

    m_scene.panel().set_arrangement(panel.arrangement());
    m_scene.panel().lay_out(m_scene.frame());
    m_scene.animation() = animation;
    m_scene.set_pattern(std::move(pattern));
    return;
}
