#if !defined(QUETZAL_GAME_APPLICATION_HPP)
#define QUETZAL_GAME_APPLICATION_HPP
//------------------------------------------------------------------------------
// quetzal game
// Application.hpp
//------------------------------------------------------------------------------

#include "Window.hpp"
#include "quetzal/common/Timer.hpp"
#include "quetzal/common/Timestamp.hpp"

namespace quetzal::game
{

    //--------------------------------------------------------------------------
    class Application
    {
    public:

        using position_type = Window::position_type;
        using size_type = Window::size_type;

        Application(Window& window);
        Application(const Application&) = delete;
        Application(Application&&) = delete;
        virtual ~Application() = default;

        Application& operator=(const Application&) = delete;
        Application& operator=(Application&&) = delete;

        int run();

        float frame_rate() const;

    protected:

        void set_window_event_interface(); // this should set up required window state callbacks, override set_window_event_interface_ for input callbacks, maybe set_<input> with callback to set it instead of setting all ...

        virtual void update(float dt) = 0;
        virtual void draw() = 0;

        // GLFW input callbacks
        virtual void on_key(GLFWwindow* pwindow, int key, int scancode, int action, int mods);
        virtual void on_char(GLFWwindow* pwindow, unsigned int codepoint);
        virtual void on_cursor_position(GLFWwindow* pwindow, double x, double y);
        virtual void on_cursor_enter(GLFWwindow* pwindow, int entered);
        virtual void on_mouse_button(GLFWwindow* pwindow, int button, int action, int mods);
        virtual void on_mouse_scroll(GLFWwindow* pwindow, double xoffset, double yoffset);

        Window& m_window;
        bool m_bSuspended;
        bool m_bMinimized;
        bool m_bSizing;
        Timer m_timer;
        Timestamp m_tsMessage;
        size_t m_nFrame;
        float m_tStatisticsPrev;
        float m_frame_rate;

    private:

        // GLFW window state callbacks
        void on_close(GLFWwindow* pwindow);
        void on_focus(GLFWwindow* pwindow, int focused);
        void on_position(GLFWwindow* pwindow, int xpos, int ypos);
        void on_size(GLFWwindow* pwindow, int width, int height);
        void on_iconify(GLFWwindow* pwindow, int iconified);
        void on_maximize(GLFWwindow* pwindow, int maximized);
        void on_framebuffer_size(GLFWwindow* pwindow, int width, int height);
        void on_refresh(GLFWwindow* pwindow);

        void on_frame();
        void on_suspend();
        void on_resume();

        virtual void on_frame_();
        virtual void on_suspend_();
        virtual void on_resume_();

        virtual bool on_close_();
        virtual void on_position_(position_type position);
        virtual void on_size_(size_type size);
        virtual void on_framebuffer_size_(size_type size);
        virtual void on_refresh_();
    };

} // namespace quetzal::game

#endif // QUETZAL_GAME_APPLICATION_HPP
