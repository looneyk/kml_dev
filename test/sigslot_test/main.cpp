//------------------------------------------------------------------------------
// main.cpp
//------------------------------------------------------------------------------

#include <string>
#include <functional>
#include <iostream>
#include <sigc++/sigc++.h>
#include <sigc++/functors/mem_fun.h>

using namespace std;

//namespace
//{

    //--------------------------------------------------------------------------
    class EventArgs
    {
    };

    //--------------------------------------------------------------------------
    class Widget : public sigc::trackable
    {
    public:

        Widget();
        virtual ~Widget() = default;

        virtual void mouse_down() = 0;

    private:
    };

    //--------------------------------------------------------------------------
    Widget::Widget()
    {
    }

    //--------------------------------------------------------------------------
    class Button : public Widget
    {
    public:

        Button();
        virtual ~Button() = default;

        template<typename T>
        void set_handler(const string& event, T* pwidget, function<bool()> f);

        virtual void mouse_down();

    private:

        sigc::signal<bool> m_signal_pressed;
    };

    //--------------------------------------------------------------------------
    Button::Button() :
        Widget(),
        m_signal_pressed()
    {
    }

    //--------------------------------------------------------------------------
    template<typename T>
    void Button::set_handler(const string& event, T* pwidget, function<bool()> f)
    {
        // find signal from event ...
//        m_signal_pressed.connect(sigc::mem_fun(pwidget, f));

        m_signal_pressed.connect(sigc::mem_fun(pwidget, f));

        return;
    }

    //--------------------------------------------------------------------------
    void Button::mouse_down()
    {
        m_signal_pressed.emit();
        return;
    }

    //--------------------------------------------------------------------------
    class Dialog : public Widget
    {
    public:

        Dialog();
        virtual ~Dialog() = default;

        virtual void mouse_down();

        bool on_button_pressed();

    private:

        Button m_button;
    };

    //--------------------------------------------------------------------------
    Dialog::Dialog() :
        Widget(),
        m_button()
    {
        m_button.set_handler("pressed", this, &Dialog::on_button_pressed);
    }

    //--------------------------------------------------------------------------
    void Dialog::mouse_down()
    {
        m_button.mouse_down();
        return;
    }

    //--------------------------------------------------------------------------
    bool on_button_pressed()
    {
        cout << "button pressed" << endl;
        return true;
    }

//} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    Dialog dialog;

    dialog.mouse_down();

    return 0;
}