//------------------------------------------------------------------------------
// Light Matrix Simulation
// Application.cpp
//------------------------------------------------------------------------------

#include "Application.hpp"
#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/direct3d11/Vertex.hpp"
#include "quetzal/direct3d11/import_model.hpp"
#include "quetzal/direct3d11/mesh_util.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include <string>
#include <vector>
#include <cassert>
#include <cmath>

using namespace std;
using namespace DirectX;

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

    using V = quetzal::direct3d11::VertexPositionNormal;
    using I = uint16_t;
    using mesh_traits = quetzal::brep::MeshTraits<quetzal::math::HomogeneousTraits<float, 3>>;
    using mesh_type = quetzal::brep::Mesh<mesh_traits>;

} // namespace

//------------------------------------------------------------------------------
quetzal::lightmatrix::Application::Application() :
    direct3d11::Application(),
    m_scene(),
    m_camera(),
    m_up(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)),
    m_speed(100.0f),
    m_pModelPixel(),
    m_pEffectPixel(),
    m_statistics()
{
    m_window.create_windowed("Light Matrix", {100, 100}, {800, 800});
    m_renderer.create();
    m_model_manager.set_path("models");

    direct3d11::EffectManager::EffectInfo effect_info = {};
    effect_info.m_name = "pixel";
    effect_info.m_bLightingDefault = true;
    effect_info.m_colorDiffuse = {1.0f, 1.0f, 1.0f};
    effect_info.m_alpha = 1.0f;
    auto pEffect = m_effect_manager.open(effect_info);
    m_pEffectPixel = std::dynamic_pointer_cast<direct3d11::EffectBasic>(pEffect);
    assert(m_pEffectPixel != nullptr);

    using direct3d11::to_xm;
    auto transfer_vertex = [](const mesh_type::vertex_attributes_type& av) -> V { return {to_xm(av.position()), to_xm(av.normal())}; };
    m_pModelPixel = m_model_manager.open<V, I, mesh_type>("unit_icosahedron.obj", transfer_vertex, pEffect);
    assert(m_pModelPixel != nullptr);

    m_scene.panel().set_size({400.0f, 400.0f});
    m_scene.panel().set_pixel_radius(2.5f);

    m_scene.frame().set_size({16, 16});
    m_scene.panel().lay_out(m_scene.frame());

    set_pattern(0);

    m_camera.set_position(XMVectorSet(0.0f, 0.0f, -500.0f, 1.0f));
    m_camera.look(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), m_up);
    init_projection();

    m_window.show();
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Application::update(float dt)
{
    check_input(dt);
    m_scene.update(dt);
    m_camera.update_view();

//    m_statistics.set_vertices(m_scene.vertex_count());
//    m_statistics.set_triangles(m_scene.triangle_count());
    m_statistics.update(dt);
    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Application::draw()
{
    m_renderer.clear();
    draw(m_scene);

//    m_statistics.set_vertices(m_scene.vertex_count());
//    m_statistics.set_triangles(m_scene.triangle_count());
//    m_statistics.text(); ...

    m_renderer.present();
    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Application::draw(Scene& scene)
{
    const Frame& frame = scene.frame();
    const auto [nx, ny] = frame.size();

    for (size_t j = 0; j < ny; ++j)
    {
        for (size_t i = 0; i < nx; ++i)
        {
            const Pixel& pixel = frame.pixel(i, j);
            Color color = pixel.color();
            float r = pixel.radius();
            const auto [x, y] = pixel.position().rep();

            m_pEffectPixel->SetDiffuseColor(XMVectorSet(color.r(), color.g(), color.b(), 1.0f));

            XMMATRIX world = XMMatrixScaling(r, r, r) * XMMatrixTranslation(x, y, 0.0f);
            m_pModelPixel->draw(m_renderer, m_common_states, world, m_camera.view(), m_camera.projection());
        }
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Application::on_size(framework::Window::size_type size)
{
    size;
    init_projection();
    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Application::on_key_down(Timestamp ts, input::key::Modifiers modifiers, input::key::Code code)
{
    ts;
    modifiers;

    switch (code)
    {
        case input::key::Code::V_ESCAPE:
            m_window.destroy();
            break;

/*
        case input::key::Code::V_F:
            if (fullscreen())
            {
                set_windowed();
            }
            else
            {
                set_fullscreen();
            }
            break;

        case input::key::Code::V_I:
            if (solid())
            {
                set_wireframe();
            }
            else
            {
                set_solid();
            }
            break;
*/

        // Layout

        case input::key::Code::V_Q:
            m_scene.panel().set_arrangement(Arrangement::Radial);
            m_scene.panel().lay_out(m_scene.frame());
            break;

        case input::key::Code::V_R:
            m_scene.panel().set_arrangement(Arrangement::Rectangular);
            m_scene.panel().lay_out(m_scene.frame());
            break;

        // Initial pattern

        case input::key::Code::V_1:
        case input::key::Code::V_2:
        case input::key::Code::V_3:
        case input::key::Code::V_4:
        case input::key::Code::V_5:
        case input::key::Code::V_6:
        case input::key::Code::V_7:
        case input::key::Code::V_8:
        case input::key::Code::V_9:
        case input::key::Code::V_0:
            set_pattern(static_cast<int>(code) - static_cast<int>(input::key::Code::V_1));
            break;

        // Brightness/color

        case input::key::Code::V_C:
            m_scene.animation().set_shift_color(!m_scene.animation().shift_color());
            break;

        case input::key::Code::V_B:
            m_scene.animation().set_shift_brightness(!m_scene.animation().shift_brightness());
            break;

        case input::key::Code::V_M:
            m_scene.animation().set_smoothing(next_smoothing[m_scene.animation().smoothing()]);
            break;

        // Amination

        case input::key::Code::V_NUMPAD1:
            m_scene.animation().set_direction({-1.0, -1.0});
            break;

        case input::key::Code::V_NUMPAD2:
            m_scene.animation().set_direction({0.0, -1.0});
            break;

        case input::key::Code::V_NUMPAD3:
            m_scene.animation().set_direction({1.0, -1.0});
            break;

        case input::key::Code::V_NUMPAD4:
            m_scene.animation().set_direction({-1.0, 0.0});
            break;

        case input::key::Code::V_NUMPAD5:
            m_scene.animation().set_direction({0.0, 0.0});
            break;

        case input::key::Code::V_NUMPAD6:
            m_scene.animation().set_direction({1.0, 0.0});
            break;

        case input::key::Code::V_NUMPAD7:
            m_scene.animation().set_direction({-1.0, 1.0});
            break;

        case input::key::Code::V_NUMPAD8:
            m_scene.animation().set_direction({0.0, 1.0});
            break;

        case input::key::Code::V_NUMPAD9:
            m_scene.animation().set_direction({1.0, 1.0});
            break;

        // Rate

        case input::key::Code::V_LEFT:
            m_scene.animation().increment_velocity({-0.1f, 0.0f});
            break;

        case input::key::Code::V_RIGHT:
            m_scene.animation().increment_velocity({0.1f, 0.0f});
            break;

        case input::key::Code::V_DOWN:
            m_scene.animation().increment_velocity({0.0f, -0.1f});
            break;

        case input::key::Code::V_UP:
            m_scene.animation().increment_velocity({0.0f, 0.1f});
            break;
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Application::on_mouse_move(Timestamp ts, input::key::Modifiers modifiers, framework::Window::position_type position)
{
    ts;
    modifiers;

    // Camera movement

    float roty = position.x / 1000.0f;
    float rotx = position.y / 1000.0f;
    if (roty == 0.0f && rotx == 0.0f)
    {
        return;
    }

// mouse_move disabled ...
//m_ofs << "ri mouse " << roty << ", " << rotx << endl;
//    XMMATRIX rotation = XMMatrixRotationY(roty) * XMMatrixRotationX(rotx);
//    m_forward = XMVector3Transform(m_forward, rotation);
//    m_forward = XMVector3Normalize(m_forward); // ? ...

    float rx = XMConvertToRadians(0.25f * static_cast<float>(position.x));
    float ry = XMConvertToRadians(0.25f * static_cast<float>(position.y));

    // Camera movement
    m_camera.pan(rx, m_up);
    m_camera.tilt(ry);
    m_camera.update_view();

    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Application::check_input(float dt)
{
    // Rate

    if (input::query_key_down(input::key::Code::V_OEM_PLUS))
    {
        m_scene.animation().increment_velocity({0.1f, 0.1f});
    }

    if (input::query_key_down(input::key::Code::V_OEM_MINUS))
    {
        m_scene.animation().increment_velocity({-0.1f, -0.1f});
    }

    // Movement

    if (input::query_key_down(input::key::Code::V_W))
    {
        m_camera.set_position(XMVectorAdd(m_camera.position(), XMVectorScale(m_camera.forward(), m_speed * dt)));
    }

    if (input::query_key_down(input::key::Code::V_S))
    {
        m_camera.set_position(XMVectorAdd(m_camera.position(), XMVectorScale(m_camera.forward(), -m_speed * dt)));
    }

/*
    if (input::query_key_down(input::key::Code::V_A))
    {
        m_camera.set_position(XMVectorAdd(m_camera.position(), XMVectorScale(m_camera.right(), -dt * m_speed)));
    }

    if (input::query_key_down(input::key::Code::V_D))
    {
        m_camera.set_position(XMVectorAdd(m_camera.position(), XMVectorScale(m_camera.right(), dt * m_speed)));
    }
*/

    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Application::init_projection()
{
    m_camera.set_lens(.25f * XM_PI, static_cast<float>(m_window.client_size().aspect_ratio()), 1.0f, 5000.0f);
    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Application::set_pattern(int nPattern)
{
    if (nPattern < 0 || nPattern >= static_cast<int>(s_pattern_names.size()))
    {
        return;
    }

    filesystem::path pattern_path = s_pattern_names[nPattern];
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
