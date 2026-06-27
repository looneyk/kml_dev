//------------------------------------------------------------------------------
// quetzal game
// Window.cpp
//------------------------------------------------------------------------------

#include "Window.hpp"
#include "quetzal/common/Exception.hpp"
#include <sstream>
#include <stdexcept>
#include <vector>
#include <cassert>

using namespace std;

//------------------------------------------------------------------------------
quetzal::game::Window::FrameworkSentry::FrameworkSentry()
{
    if (glfwInit() != GLFW_TRUE)
    {
        throw std::runtime_error("Error glfwInit");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
}

//------------------------------------------------------------------------------
quetzal::game::Window::FrameworkSentry::~FrameworkSentry()
{
    glfwTerminate();
}

//------------------------------------------------------------------------------
quetzal::game::Window::Settings::Settings(Mode mode, position_type position, size_type size, bool borderless, bool resizable) :
    m_mode(mode),
    m_position(position),
    m_size(size),
    m_borderless(borderless),
    m_resizable(resizable)
{
}

//------------------------------------------------------------------------------
quetzal::game::Window::Mode quetzal::game::Window::Settings::mode() const
{
    return m_mode;
}

//------------------------------------------------------------------------------
quetzal::game::Window::position_type quetzal::game::Window::Settings::position() const
{
    return m_position;
}

//------------------------------------------------------------------------------
quetzal::game::Window::size_type quetzal::game::Window::Settings::size() const
{
    return m_size;
}

//------------------------------------------------------------------------------
bool quetzal::game::Window::Settings::borderless() const
{
    return m_borderless;
}

//------------------------------------------------------------------------------
bool quetzal::game::Window::Settings::resizable() const
{
    return m_resizable;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::Settings::set_position(position_type position)
{
    m_position = position;
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::Settings::set_size(size_type size)
{
    m_size = size;
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::Settings::set_borderless(bool b)
{
    m_borderless = b;
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::Settings::set_resizable(bool b)
{
    m_resizable = b;
    return;
}

//------------------------------------------------------------------------------
quetzal::game::Window::size_type quetzal::game::Window::Settings::screen_size()
{
    GLFWmonitor* pmonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* vidmode = glfwGetVideoMode(pmonitor);
    return {vidmode->width, vidmode->height};
}

//------------------------------------------------------------------------------
quetzal::game::Window::Mode quetzal::game::Window::Settings::mode(const std::string str)
{
    if (str == "full")
    {
        return Mode::Full;
    }

    if (str == "minimized")
    {
        return Mode::Minimized;
    }

    if (str == "windowed" || str.empty())
    {
        return Mode::Windowed;
    }

    // log error ...
    return Mode::Windowed;
}

//------------------------------------------------------------------------------
quetzal::game::Window::Window(const Config& config) :
    m_glfw_sentry(),
    m_settings_full(Mode::Full, {}, Settings::screen_size()),
    m_settings_windowed(Mode::Windowed, {stoi(config["x"]), stoi(config["y"])}, {stoi(config["width"]), stoi(config["height"])}, false, true),
    m_settings_minimized(Mode::Minimized),
    m_psettings(nullptr),
    m_psettings_prev(nullptr),
    m_pwindow(nullptr),
    m_client_size()
{
}

//------------------------------------------------------------------------------
quetzal::game::Window::~Window()
{
    destroy();
}

//------------------------------------------------------------------------------
void quetzal::game::Window::create_full(const std::string& title, bool visible)
{
    m_psettings = &m_settings_full;
    m_psettings_prev = nullptr;

    size_type size{};
    GLFWmonitor* pmonitor = glfwGetPrimaryMonitor();
    if (m_psettings->mode() == Mode::Full)
    {
        size = Settings::screen_size();
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, visible ? GLFW_TRUE : GLFW_FALSE);

    m_pwindow = glfwCreateWindow(size.x, size.y, title.c_str(), pmonitor, nullptr);
    if (m_pwindow == nullptr)
    {
        throw std::runtime_error("Error: create_full " + title);
    }

    update_client_size();

    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::create_windowed(const std::string& title, position_type position, size_type size, bool borderless, bool resizable, bool visible)
{
    m_psettings = &m_settings_windowed;
    m_psettings_prev = nullptr;

    m_settings_windowed.set_position(position);
    m_settings_windowed.set_size(size);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, visible ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, borderless ? GLFW_FALSE : GLFW_TRUE);

    m_pwindow = glfwCreateWindow(size.x, size.y, title.c_str(), nullptr, nullptr);
    if (m_pwindow == nullptr)
    {
        throw std::runtime_error("Error: create_windowed " + title);
    }

    update_client_size();

    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::create_minimized(const std::string& title)
{
    m_psettings = &m_settings_windowed;
    m_psettings_prev = nullptr;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    m_pwindow = glfwCreateWindow(0, 0, title.c_str(), nullptr, nullptr);
    if (m_pwindow == nullptr)
    {
        throw std::runtime_error("Error: create_minimized " + title);
    }

    glfwIconifyWindow(m_pwindow);

    update_client_size();

    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::create(Mode mode, const std::string& title, position_type position, size_type size, bool borderless, bool resizable, bool visible)
{
    // TODO ...

    create_windowed(title, position, size, borderless, resizable, false);
    set_mode(mode);
    set_visible(mode == Mode::Minimized ? false : visible);
    return;
}

//------------------------------------------------------------------------------
quetzal::game::Window::Mode quetzal::game::Window::mode() const
{
    assert(m_psettings != nullptr);
    return m_psettings->mode();
}

//------------------------------------------------------------------------------
void quetzal::game::Window::set_mode(Mode mode)
{
    switch (mode)
    {
    case Mode::Minimized:
        assert(m_psettings->mode() != Mode::Minimized);
        assert(m_psettings_prev == nullptr);
        m_psettings_prev = m_psettings;
        m_psettings = &m_settings_minimized;

        glfwIconifyWindow(m_pwindow);
        break;

    case Mode::Windowed:
        if (m_psettings->mode() == Mode::Minimized)
        {
            assert(m_psettings_prev != nullptr);
            m_psettings = &m_settings_windowed;
            m_psettings_prev = nullptr;

            glfwRestoreWindow(m_pwindow);
        }
        else
        {
            assert(m_psettings_prev == nullptr);
            m_psettings = &m_settings_windowed;
            auto position = m_psettings->position();
            auto size = m_psettings->size();

            glfwSetWindowMonitor(m_pwindow, nullptr, position.x, position.y, size.x, size.y, GLFW_DONT_CARE);
        }
        break;

    case Mode::Full:
    {
        GLFWmonitor* pmonitor = glfwGetPrimaryMonitor();

        if (m_psettings->mode() == Mode::Minimized)
        {
            assert(m_psettings_prev != nullptr);
            m_psettings = &m_settings_full;
            m_psettings_prev = nullptr;
            auto size = m_psettings->size();

            glfwRestoreWindow(m_pwindow);
            glfwSetWindowMonitor(m_pwindow, pmonitor, 0, 0, size.x, size.y, GLFW_DONT_CARE);
        }
        else
        {
            assert(m_psettings_prev == nullptr);
            m_psettings = &m_settings_full;
            auto size = m_psettings->size();

            glfwSetWindowMonitor(m_pwindow, pmonitor, 0, 0, size.x, size.y, GLFW_DONT_CARE);
        }
        break;
    }

    default:
        assert(false);
    }

    update_client_size();
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::update_mode(Mode mode)
{
    switch (mode)
    {
    case Mode::Minimized:
        assert(m_psettings->mode() != Mode::Minimized);
        assert(m_psettings_prev == nullptr);
        m_psettings_prev = m_psettings;
        m_psettings = &m_settings_minimized;
        break;

    case Mode::Windowed:
        assert(m_psettings->mode() != Mode::Windowed);
        m_psettings = &m_settings_windowed;
        m_psettings_prev = nullptr;
        break;

    case Mode::Full:
        assert(m_psettings->mode() != Mode::Full);
        m_psettings = &m_settings_full;
        m_psettings_prev = nullptr;
        break;
    }

    update_client_size();
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::restore()
{
    if (mode() == Mode::Minimized)
    {
        assert(m_psettings_prev != nullptr);
        set_mode(m_psettings_prev->mode());
    }

    return;
}

//------------------------------------------------------------------------------
quetzal::game::Window::position_type quetzal::game::Window::position() const
{
    assert(m_psettings != nullptr);
    return m_psettings->position();
}

//------------------------------------------------------------------------------
quetzal::game::Window::size_type quetzal::game::Window::size() const
{
    assert(m_psettings != nullptr);
    return m_psettings->size();
}

//------------------------------------------------------------------------------
quetzal::game::Window::size_type quetzal::game::Window::client_size() const
{
    return m_client_size;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::set_title(const std::string& title)
{
    glfwSetWindowTitle(m_pwindow, title.c_str());
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::set_position(position_type position)
{
    assert(m_psettings != nullptr);
    m_psettings->set_position(position);

    if (mode() == Mode::Windowed)
    {
        glfwSetWindowPos(m_pwindow, position.x, position.y);
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::set_size(size_type size)
{
    assert(m_psettings != nullptr);
    m_psettings->set_size(size);

    if (mode() == Mode::Windowed)
    {
        glfwSetWindowSize(m_pwindow, size.x, size.y);
    }

    update_client_size();

    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::set_layout(position_type position, size_type size)
{
    assert(m_psettings != nullptr);
    m_psettings->set_position(position);
    m_psettings->set_size(size);

    if (mode() == Mode::Windowed)
    {
        glfwSetWindowPos(m_pwindow, position.x, position.y);
        glfwSetWindowSize(m_pwindow, size.x, size.y);
    }

    update_client_size();

    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::show()
{
    glfwShowWindow(m_pwindow);
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::hide()
{
    glfwHideWindow(m_pwindow);
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::set_visible(bool visible)
{
    visible ? glfwShowWindow(m_pwindow) : glfwHideWindow(m_pwindow);
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::poll() const
{
    glfwPollEvents();
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::wait_events() const
{
    glfwWaitEvents();
    return;
}

//------------------------------------------------------------------------------
bool quetzal::game::Window::should_close() const
{
    return glfwWindowShouldClose(m_pwindow) == GLFW_TRUE;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::set_close()
{
    glfwSetWindowShouldClose(m_pwindow, GLFW_TRUE);
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::destroy()
{
    if (m_pwindow != nullptr)
    {
        glfwDestroyWindow(m_pwindow);
        m_pwindow = nullptr;
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::set_user_pointer(void* p)
{
    glfwSetWindowUserPointer(m_pwindow, p);
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::set_close_callback(GLFWwindowclosefun callback)
{
    glfwSetWindowCloseCallback(m_pwindow, callback);
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::set_focus_callback(GLFWwindowfocusfun callback)
{
    glfwSetWindowFocusCallback(m_pwindow, callback);
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::set_position_callback(GLFWwindowposfun callback)
{
    glfwSetWindowPosCallback(m_pwindow, callback);
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::set_size_callback(GLFWwindowsizefun callback)
{
    glfwSetWindowSizeCallback(m_pwindow, callback);
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::set_iconify_callback(GLFWwindowiconifyfun callback)
{
    glfwSetWindowIconifyCallback(m_pwindow, callback);
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::set_maximize_callback(GLFWwindowmaximizefun callback)
{
    glfwSetWindowMaximizeCallback(m_pwindow, callback);
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::set_framebuffer_size_callback(GLFWframebuffersizefun callback)
{
    glfwSetFramebufferSizeCallback(m_pwindow, callback);
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::set_refresh_callback(GLFWwindowrefreshfun callback)
{
    glfwSetWindowRefreshCallback(m_pwindow, callback);
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::set_key_callback(GLFWkeyfun callback)
{
    glfwSetKeyCallback(m_pwindow, callback);
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::set_char_callback(GLFWcharfun callback)
{
    glfwSetCharCallback(m_pwindow, callback);
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::set_cursor_position_callback(GLFWcursorposfun callback)
{
    glfwSetCursorPosCallback(m_pwindow, callback);
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::set_cursor_enter_callback(GLFWcursorenterfun callback)
{
    glfwSetCursorEnterCallback(m_pwindow, callback);
    return;
}
void quetzal::game::Window::set_mouse_button_callback(GLFWmousebuttonfun callback)
{
    glfwSetMouseButtonCallback(m_pwindow, callback);
    return;
}

//------------------------------------------------------------------------------
void quetzal::game::Window::set_mouse_scroll_callback(GLFWscrollfun callback)
{
    glfwSetScrollCallback(m_pwindow, callback);
    return;
}

//------------------------------------------------------------------------------
int quetzal::game::Window::get_key_state(int key) const
{
    return glfwGetKey(m_pwindow, key);
}

//------------------------------------------------------------------------------
quetzal::game::Window::cursor_position_type quetzal::game::Window::get_cursor_position() const
{
    cursor_position_type pos;
    glfwGetCursorPos(m_pwindow, &pos[0], &pos[1]);
    return pos;
}

//------------------------------------------------------------------------------
quetzal::game::Window::size_type quetzal::game::Window::framebuffer_size() const
{
    int width;
    int height;
    glfwGetFramebufferSize(m_pwindow, &width, &height);
    return {width, height};
}

//------------------------------------------------------------------------------
void quetzal::game::Window::update_client_size()
{
    if (m_psettings->mode() == Mode::Windowed)
    {
        int w = 0;
        int h = 0;
        glfwGetWindowSize(m_pwindow, &w, &h);
        m_client_size = {w, h};
    }
    else
    {
        m_client_size = m_psettings->size();
    }

    return;
}
