//------------------------------------------------------------------------------
// Light Matrix Simulation
// Copyright (c) 2010 Keith Michael Looney. All Rights Reserved.
// Application.cpp
//------------------------------------------------------------------------------

#include "Application.hpp"
#include <cassert>

using namespace std;
using namespace DirectX;

namespace
{

    const std::string s_pattern_path = "resources/patterns/";

    const vector<string> s_patterns =
    {
        "pattern0.xml",
        "pattern1.xml",
        "pattern2.xml",
        "pattern3.xml",
        "pattern4.xml",
        "pattern5.xml",
        "pattern6.xml",
        "pattern7.xml",
        "pattern0.xml",
        "pattern0.xml"
    };

} // namespace

//------------------------------------------------------------------------------
quetzal::lightmatrix::Application::Application() :
    direct3d11::Application(),
    m_frame(),
    m_scene(*this, m_frame),
    m_controller(m_frame),
    m_camera(),
    m_up(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)),
    m_speed(100.0f),
    m_tDissolve(3.0f),
    m_statistics()
{
    m_window.create_windowed("Light Matrix", {100, 100}, {800, 800});
    m_renderer.create();

    set_pattern(0);
    m_scene.load();

    m_camera.set_position(XMVectorSet(0.0f, 0.0f, -500.0f, 1.0f));
    m_camera.look(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), m_up);
    init_projection();

    m_window.show();
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Application::update(float dt)
{
    check_input(dt);
    m_camera.update_view();
    m_controller.update(dt);

    m_statistics.set_vertices(m_scene.vertex_count());
    m_statistics.set_triangles(m_scene.triangle_count());
    m_statistics.update(dt);
    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Application::draw()
{
////    m_scene.draw(m_camera);
//    draw();

    m_renderer.clear();
    m_scene.draw(m_camera);

    m_statistics.set_vertices(m_scene.vertex_count());
    m_statistics.set_triangles(m_scene.triangle_count());
//    m_statistics.text(); ...

    m_renderer.present();
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
            m_controller.set_radial(false);
            break;

        case input::key::Code::V_R:
            m_controller.set_radial(true);
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
            m_controller.toggle_shift_color();
            break;

        case input::key::Code::V_B:
            m_controller.toggle_shift_brightness();
            break;

        case input::key::Code::V_M:
            m_controller.cycle_smoothing();
            break;

        // Amination

        case input::key::Code::V_NUMPAD1:
            m_controller.set_direction(-1., -1.);
            break;

        case input::key::Code::V_NUMPAD2:
            m_controller.set_direction(0., -1.);
            break;

        case input::key::Code::V_NUMPAD3:
            m_controller.set_direction(1., -1.);
            break;

        case input::key::Code::V_NUMPAD4:
            m_controller.set_direction(-1., 0.);
            break;

        case input::key::Code::V_NUMPAD5:
            m_controller.set_direction(0., 0.);
            break;

        case input::key::Code::V_NUMPAD6:
            m_controller.set_direction(1., 0.);
            break;

        case input::key::Code::V_NUMPAD7:
            m_controller.set_direction(-1., 1.);
            break;

        case input::key::Code::V_NUMPAD8:
            m_controller.set_direction(0., 1.);
            break;

        case input::key::Code::V_NUMPAD9:
            m_controller.set_direction(1., 1.);
            break;

        // Rate

        case input::key::Code::V_LEFT:
            m_controller.increment_velocity(-0.1f, 0.0f);
            break;

        case input::key::Code::V_RIGHT:
            m_controller.increment_velocity(0.1f, 0.0f);
            break;

        case input::key::Code::V_DOWN:
            m_controller.increment_velocity(0.0f, -0.1f);
            break;

        case input::key::Code::V_UP:
            m_controller.increment_velocity(0.0f, 0.1f);
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
    m_camera.pan(rx);
//    m_camera.pan(rx, m_up);
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
        m_controller.increment_velocity(0.1f, 0.1f);
    }

    if (input::query_key_down(input::key::Code::V_OEM_MINUS))
    {
        m_controller.increment_velocity(-0.1f, -0.1f);
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
    if (nPattern < 0 || nPattern >= static_cast<int>(s_patterns.size()))
    {
        return;
    }

    string path = s_patterns[nPattern];

    if (!s_pattern_path.empty())
    {
        path = s_pattern_path + path;
    }

    m_controller.load_pattern(path, m_tDissolve);

    return;
}
