//------------------------------------------------------------------------------
//  main.cpp
//
//------------------------------------------------------------------------------

#include "quetzal/gui/System.hpp"
#include "quetzal/gui/Widgets.hpp"
#include "quetzal/gui/widgets/ButtonToggle.hpp"
#include "quetzal/gui/widgets/Entry.hpp"
#include "quetzal/Exception.hpp"
#include "Panel1.hpp"
#include <iostream>
#include <cassert>

using namespace std;
using namespace quetzal;
using namespace quetzal::gui;

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    string filename = "gui_test.xml";

    if (argc >= 2)
    {
        filename = argv[1];
    }

    try
    {
        Panel1 panel1;

        Entry e1("e1");

        ButtonToggle bt2("bt2");
        Widget* pw2 = &bt2;

        std::shared_ptr<Widget> pwx3 = System::Instance().widget_factory().create(TypeEntry, "e3");
        assert(pwx3);
        Widget* pw3 = pwx3.get();

        std::shared_ptr<Widget> pwx4 = System::Instance().widget_factory().create(TypeButtonToggle, "bt4");
        ButtonToggle* pbt4 = static_cast<ButtonToggle*>(pwx4.get());
        assert(pbt4);
        Widget* pw4 = pbt4;

        cout << "e1 type: " << e1.type() << endl;
        cout << "e1 id: " << e1.id() << endl;
        cout << "e1 size: " << e1.size() << endl;
		e1.set_size({ 10, 10 });
        cout << "e1 size: " << e1.size() << endl;

        cout << "w3 type: " << pw3->type() << endl;
        cout << "w3 id: " << pw3->id() << endl;

        cout << "bt2 type: " << bt2.type() << endl;
        cout << "bt2 id: " << bt2.id() << endl;
        cout << "bt2 marked: " << bt2.marked() << endl;
        bt2.set_marked(true);
        cout << "bt2 marked: " << bt2.marked() << endl;
        cout << "bt2 marked: " << bt2.property("Marked") << endl;
        bt2.set_property("Marked", "false");
        cout << "bt2 marked: " << bt2.marked() << endl;

        cout << "bt4 type: " << pbt4->type() << endl;
        cout << "bt4 id: " << pbt4->id() << endl;
        cout << "bt4 marked: " << pbt4->marked() << endl;
        pbt4->set_property("Marked", "true");
        cout << "bt4 marked: " << pbt4->marked() << endl;
        cout << "bt4 marked: " << pbt4->property("Marked") << endl;
        pbt4->set_marked(false);
        cout << "bt4 marked: " << pbt4->marked() << endl;

        cout << "w2 type: " << pw2->type() << endl;
        cout << "w2 id: " << pw2->id() << endl;
        cout << "w2 marked: " << pw2->property("Marked") << endl;
        pw2->set_property("Marked", "true");
        cout << "w2 marked: " << pw2->property("Marked") << endl;
        pw2->set_property("Marked", "false");
        cout << "w2 marked: " << pw2->property("Marked") << endl;

        cout << "w4 type: " << pw4->type() << endl;
        cout << "w4 id: " << pw4->id() << endl;
        cout << "w4 marked: " << pw4->property("Marked") << endl;
        pw4->set_property("Marked", "true");
        cout << "w4 marked: " << pw4->property("Marked") << endl;
        pw4->set_property("Marked", "false");
        cout << "w4 marked: " << pw4->property("Marked") << endl;

        // First copy

        FormManager& fm = System::Instance().form_manager();

        fm.load(filename);

        Widget* pw = fm.find("panel1").get();
        assert(pw);

        cout << pw->content().size() << endl;

        fm.store("gui_test_1.xml");

        // Second copy

        fm.clear();

        fm.load("gui_test_1.xml");

        pw = fm.find("panel1").get();
        assert(pw);

        cout << pw->content().size() << endl;

        fm.store("gui_test_2.xml");
    }
    catch (Exception& e)
    {
        cout << "Exception:" << endl;
        cout << e.what() << endl;
        cout << e.file() << ", " << e.line() << endl;
    }
    catch (exception& e)
    {
        cout << "exception: " << e.what() << endl;
    }

//    cout << "Press a key to exit." << endl;
//
//    char c;
//    cin >> c;

    return 0;
}

/*
//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    try
    {
        Resource r;
        r.open("test.xml");

        Screen screen1;
        screen1.open(640, 480);

        Panel1 panel1(screen1);
        panel1.open(r, "Panel1"); // screen1 ...
        panel1.present(true); // screen1 ...
    }
    catch (exception& e)
    {
        cout << e.what();
    }

    return 0;
}

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    // creation by factory from template info ...
    WidgetPanel panel("Panel");
    WidgetButton button("Button");

    ppanel.add(pbutton)

    ppanel->subscribe(pbutton, "EventReleased", Panel1::on_button_released);

    return 0;
}

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    gui::System& system = gui::System::Instance();

    system.load("test.xml");

    Widget* ppanel = system.widget("Panel1");

    ppanel->subscribe(pbutton, "EventReleased", Panel1::on_button_released);

    int code = ppanel->present(); // display, show, open, ...

    return 0;
}

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    Application a;

    a.init(...);

    a.run();

    return 0;
}

//------------------------------------------------------------------------------
void Application::run()
{
    while (!done)
    {
        check_input();
        ai();
        move_actors();
        resolve_collisions();
        draw();
        play_sounds();
    }

    return;
}

//------------------------------------------------------------------------------
void Application::check_input()
{
    gui::System::instance::poll_devices();

    return;
}

//------------------------------------------------------------------------------
void Application::draw()
{

    return;
}

//------------------------------------------------------------------------------
void gui::System::poll_devices()
{
    for (each registered input device)
    {
        poll_device(pdevice);
    }

    return;
}

//------------------------------------------------------------------------------
void gui::System::poll_device(Device* pdevice)
{
    for (each event queued by this device)
    {
        Screen* pscreen = find_screen_for(this input);
        call the apropriate screen (widget) input function so it can route it to the appropriate widget
    }

    return;
}

//------------------------------------------------------------------------------
EventArgs* Device::poll()
{
    for (each input type)
    {
        check for input
        for (each input)
        {
            return new eventargs(...); // and event type/code
        }
    }

    return 0;
}
*/
