//------------------------------------------------------------------------------
// quetzal game
// Application.cpp
//------------------------------------------------------------------------------

#include "Application.hpp"
#include "Window.hpp"
#include <thread>
#include <vector>

using namespace std;

//------------------------------------------------------------------------------
quetzal::game::Application::Application(Window& window) :
    m_window(window),
    m_suspended(false),
    m_sizing(false),
    m_timer(),
    m_statistics_frame_count(0),
    m_statistics_interval(1.0f),
    m_statistics_time_prev(0.0f),
    m_frame_rate(0.0f)
{
}

//------------------------------------------------------------------------------
int quetzal::game::Application::run()
{
    m_timer.reset();

    while (!m_window.should_close())
    {
        m_window.poll();

        if (m_suspended) // Give other applications some extra execution time
        {
            this_thread::sleep_for(10ms);
            continue;
        }

        update(static_cast<float>(m_timer.delta()));
        draw();
        m_timer.tick();
        on_frame();
    }

    return 0;
}

//------------------------------------------------------------------------------
float quetzal::game::Application::frame_rate() const
{
    return m_frame_rate;
}

//------------------------------------------------------------------------------
void quetzal::game::Application::set_window_event_interface()
{
    m_window.set_user_pointer(this);

    // System callbacks

    auto close_callback = [](GLFWwindow* pwindow) -> void
        { static_cast<Application*>(glfwGetWindowUserPointer(pwindow))->on_close(pwindow); };
    m_window.set_close_callback(close_callback);

    auto focus_callback = [](GLFWwindow* pwindow, int focused) -> void
        { static_cast<Application*>(glfwGetWindowUserPointer(pwindow))->on_focus(pwindow, focused); };
    m_window.set_focus_callback(focus_callback);

    auto pos_callback = [](GLFWwindow* pwindow, int x, int y) -> void
        { static_cast<Application*>(glfwGetWindowUserPointer(pwindow))->on_position(pwindow, x, y); };
    m_window.set_position_callback(pos_callback);

    auto size_callback = [](GLFWwindow* pwindow, int width, int height) -> void
        { static_cast<Application*>(glfwGetWindowUserPointer(pwindow))->on_size(pwindow, width, height); };
    m_window.set_size_callback(size_callback);

    auto iconify_callback = [](GLFWwindow* pwindow, int iconified) -> void
        { static_cast<Application*>(glfwGetWindowUserPointer(pwindow))->on_iconify(pwindow, iconified); };
    m_window.set_iconify_callback(iconify_callback);

    auto maximize_callback = [](GLFWwindow* pwindow, int maximized) -> void
        { static_cast<Application*>(glfwGetWindowUserPointer(pwindow))->on_maximize(pwindow, maximized); };
    m_window.set_maximize_callback(maximize_callback);

    auto framebuffer_size_callback = [](GLFWwindow* pwindow, int width, int height) -> void
        { static_cast<Application*>(glfwGetWindowUserPointer(pwindow))->on_framebuffer_size(pwindow, width, height); };
    m_window.set_framebuffer_size_callback(framebuffer_size_callback);

    auto refresh_callback = [](GLFWwindow* pwindow) -> void
        { static_cast<Application*>(glfwGetWindowUserPointer(pwindow))->on_refresh(pwindow); };
    m_window.set_refresh_callback(refresh_callback);

    // Input callbacks

    auto key_callback = [](GLFWwindow* pwindow, int key, int scancode, int action, int mods) -> void
        { static_cast<Application*>(glfwGetWindowUserPointer(pwindow))->on_key(pwindow, key, scancode, action, mods); };
    m_window.set_key_callback(key_callback);

    auto char_callback = [](GLFWwindow* pwindow, unsigned int codepoint) -> void
        { static_cast<Application*>(glfwGetWindowUserPointer(pwindow))->on_char(pwindow, codepoint); };
    m_window.set_char_callback(char_callback);

    auto cursor_position_callback = [](GLFWwindow* pwindow, double x, double y) -> void
        { static_cast<Application*>(glfwGetWindowUserPointer(pwindow))->on_cursor_position(pwindow, x, y); };
    m_window.set_cursor_position_callback(cursor_position_callback);

    auto cursor_enter_callback = [](GLFWwindow* pwindow, int entered) -> void
        { static_cast<Application*>(glfwGetWindowUserPointer(pwindow))->on_cursor_enter(pwindow, entered); };
    m_window.set_cursor_enter_callback(cursor_enter_callback);

    auto mouse_button_callback = [](GLFWwindow* pwindow, int button, int action, int mods) -> void
        { static_cast<Application*>(glfwGetWindowUserPointer(pwindow))->on_mouse_button(pwindow, button, action, mods); };
    m_window.set_mouse_button_callback(mouse_button_callback);

    auto mouse_scroll_callback = [](GLFWwindow* pwindow, double xoffset, double yoffset) -> void
        { static_cast<Application*>(glfwGetWindowUserPointer(pwindow))->on_mouse_scroll(pwindow, xoffset, yoffset); };
    m_window.set_mouse_scroll_callback(mouse_scroll_callback);

    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Application::on_key(GLFWwindow* pwindow, int key, int scancode, int action, int mods)
{
    pwindow;
    key;
    scancode;
    action;
    mods;
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Application::on_char(GLFWwindow* pwindow, unsigned int codepoint)
{
    pwindow;
    codepoint;
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Application::on_cursor_position(GLFWwindow* pwindow, double x, double y)
{
    pwindow;
    x;
    y;
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Application::on_cursor_enter(GLFWwindow* pwindow, int entered)
{
    pwindow;
    entered;
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Application::on_mouse_button(GLFWwindow* pwindow, int button, int action, int mods)
{
    pwindow;
    button;
    action;
    mods;
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Application::on_mouse_scroll(GLFWwindow* pwindow, double xoffset, double yoffset)
{
    pwindow;
    xoffset;
    yoffset;
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Application::on_close(GLFWwindow* pwindow)
{
    pwindow;
    on_close_();
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Application::on_focus(GLFWwindow* pwindow, int focused)
{
    pwindow;

    if (focused && m_suspended)
    {
        on_resume();
    }
    else if (!focused && !m_suspended)
    {
        on_suspend();
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Application::on_position(GLFWwindow* pwindow, int x, int y)
{
    pwindow;
    on_position_({x, y});
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Application::on_size(GLFWwindow* pwindow, int width, int height)
{
    pwindow;
    on_size_({width, height});
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Application::on_iconify(GLFWwindow* pwindow, int iconified)
{
    pwindow;

    if (iconified && !m_suspended)
    {
        on_suspend();
    }
    else if (!iconified && m_suspended)
    {
        on_resume();
    }

    m_window.update_mode(iconified ? Window::Mode::Minimized : Window::Mode::Windowed); // maybe not Windowed, really need to track state before iconified ...
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Application::on_maximize(GLFWwindow* pwindow, int maximized)
{
    pwindow;

    if (maximized && m_suspended)
    {
        on_resume();
    }

    m_window.update_mode(maximized ? Window::Mode::Full : Window::Mode::Windowed); // maybe not Windowed, really need to track state before iconified ...
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Application::on_framebuffer_size(GLFWwindow* pwindow, int width, int height)
{
    pwindow;
    on_framebuffer_size_({width, height});
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Application::on_refresh(GLFWwindow* pwindow)
{
    pwindow;
    on_refresh_();
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Application::on_frame()
{
    ++m_statistics_frame_count;

    // Compute statistics once per second
    // better yet, provide rolling statistics over the last second ...

    if ((m_timer.total() - m_statistics_time_prev) >= m_statistics_interval)
    {
        m_frame_rate = m_statistics_frame_count / m_statistics_interval;
        // Reset for next data point
        m_statistics_frame_count = 0;
        m_statistics_time_prev += m_statistics_interval;
    }

    on_frame_();

    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Application::on_suspend()
{
    m_suspended = true;
    on_suspend_();
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Application::on_resume()
{
    m_suspended = false;
    on_resume_();
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Application::on_frame_()
{
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Application::on_suspend_()
{
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Application::on_resume_()
{
    return;
}

//------------------------------------------------------------------------------
bool quetzal::game::Application::on_close_()
{
    return true;
}

//------------------------------------------------------------------------------
void quetzal::game::Application::on_position_(position_type position)
{
    position;
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Application::on_size_(size_type size)
{
    size;
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Application::on_framebuffer_size_(size_type size)
{
    size;
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Application::on_refresh_()
{
    return;
}
