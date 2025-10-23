#if !defined(QUETZAL_GAME_WINDOW_HPP)
#define QUETZAL_GAME_WINDOW_HPP
//------------------------------------------------------------------------------
// quetzal game
// Window.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/Config.hpp"
#include "GLFW/glfw3.h"
#include <array>
#include <functional>
#include <string>

namespace quetzal::game
{

    //--------------------------------------------------------------------------
    class Window
    {
    public:

        struct PositionSize2
        {
            int x;
            int y;
        };

        using position_type = PositionSize2;
        using size_type = PositionSize2;
        using cursor_position_type = std::array<double, 2>;

        //----------------------------------------------------------------------
        enum class Mode
        {
            Full,
            Windowed,
            Minimized,
        };

        //----------------------------------------------------------------------
        class Settings
        {
        public:

            Settings() = default;
            explicit Settings(Mode mode, position_type position = {}, size_type size = {}, bool borderless = true, bool resizable = false);
            Settings(const Settings&) = default;
            Settings(Settings&&) = default;
            virtual ~Settings() = default;

            Settings& operator=(const Settings&) = default;
            Settings& operator=(Settings&&) = default;

            Mode mode() const;
            position_type position() const;
            size_type size() const;
            bool borderless() const;
            bool resizable() const;

            void set_position(position_type position);
            void set_size(size_type size);
            void set_borderless(bool b);
            void set_resizable(bool b);

            static size_type screen_size();
            static Mode mode(const std::string str);

        protected:

            Mode m_mode;
            position_type m_position;
            size_type m_size;
            bool m_borderless;
            bool m_resizable;
        };

        Window(const Config& config);
        Window(const Window&) = delete;
        Window(Window&&) = delete;
        virtual ~Window();

        Window& operator=(const Window&) = delete;
        Window& operator=(Window&&) = delete;

        void create_full(const std::string& title, bool visible = true);
        void create_windowed(const std::string& title, position_type position, size_type size, bool borderless = false, bool resizable = true, bool visible = true);
        void create_minimized(const std::string& title);

        void create(Mode mode, const std::string& title, position_type position, size_type size, bool borderless = false, bool resizable = true, bool visible = true);

        Mode mode() const;
        void set_mode(Mode mode);
        void update_mode(Mode mode); // Used to track outside mode changes
        void restore();

        position_type position() const;
        size_type size() const;
        size_type client_size() const;

        // These still set values for Windowed settings even if the window is not currently Windowed
        void set_title(const std::string& title);
        void set_position(position_type position);
        void set_size(size_type size);
        void set_layout(position_type position, size_type size);

        void show();
        void hide();
        void set_visible(bool visible);

        void poll() const;
        void wait_events() const;
        bool should_close() const;

        void set_close();

        void destroy();

        void set_user_pointer(void* p);

        void set_close_callback(GLFWwindowclosefun callback);
        void set_focus_callback(GLFWwindowfocusfun callback);
        void set_position_callback(GLFWwindowposfun callback);
        void set_size_callback(GLFWwindowsizefun callback);
        void set_iconify_callback(GLFWwindowiconifyfun callback);
        void set_maximize_callback(GLFWwindowmaximizefun callback);
        void set_framebuffer_size_callback(GLFWframebuffersizefun callback);
        void set_refresh_callback(GLFWwindowrefreshfun callback);

        void set_key_callback(GLFWkeyfun callback);
        void set_char_callback(GLFWcharfun callback);
        void set_cursor_position_callback(GLFWcursorposfun callback);
        void set_cursor_enter_callback(GLFWcursorenterfun callback);
        void set_mouse_button_callback(GLFWmousebuttonfun callback);
        void set_mouse_scroll_callback(GLFWscrollfun callback);

        int get_key_state(int key) const;
        cursor_position_type get_cursor_position() const;

        size_type framebuffer_size() const;

    protected:

        void update_client_size();

        // not just sentry, but full framework abstraction? ...
        // that's pretty much what Window is ...
        // do more with this or get rid of it ...
        struct FrameworkSentry
        {
            FrameworkSentry();
            ~FrameworkSentry();
        };

        FrameworkSentry m_glfw_sentry;

        Settings m_settings_full;
        Settings m_settings_windowed;
        Settings m_settings_minimized;
        Settings* m_psettings;
        Settings* m_psettings_prev;

        GLFWwindow* m_pwindow;

        size_type m_client_size;
    };

} // namespace quetzal::game

#endif // QUETZAL_GAME_WINDOW_HPP
