#if !defined(QUETZAL_FRAMEWORK_TEST_APPLICATION_HPP)
#define QUETZAL_FRAMEWORK_TEST_APPLICATION_HPP
//------------------------------------------------------------------------------
// framework_test
// Application.hpp
//------------------------------------------------------------------------------

#include "quetzal/framework/Application.hpp"

namespace quetzal
{

namespace framework_test
{

    //----------------------------------------------------------------------
    class Application : public quetzal::framework::Application
    {
    public:

        Application();
        Application(const Application&) = delete;
        virtual ~Application() = default;

        Application& operator=(const Application&) = delete;

    protected:

        virtual void update(float dt);
        virtual void render();

//        virtual void on_key_down(Timestamp ts, input::key::Modifiers modifiers, input::key::Code code);
//        virtual void on_key_up(Timestamp ts, input::key::Modifiers modifiers, input::key::Code code);
//        virtual void on_char(Timestamp ts, input::key::Modifiers modifiers, const std::string& utf8);
//        virtual void on_mouse_down(Timestamp ts, input::key::Modifiers modifiers, Position position, input::key::Code code);
//        virtual void on_mouse_up(Timestamp ts, input::key::Modifiers modifiers, Position position, input::key::Code code);
//        virtual void on_mouse_move(Timestamp ts, input::key::Modifiers modifiers, Position position);
//        virtual void on_mouse_scroll(Timestamp ts, input::key::Modifiers modifiers, Position position);

    private:

//        virtual void on_frame_();
//        virtual void on_size_(Size size);
//        virtual void on_suspend_();
//        virtual void on_resume_();
//        virtual bool on_close_();
//
//        member variables ...
    };

} // namespace framework_test

} // namespace quetzal

#endif // QUETZAL_FRAMEWORK_TEST_APPLICATION_HPP
