//------------------------------------------------------------------------------
//  Panel1.hpp
//
//------------------------------------------------------------------------------

#include "Panel1.hpp"

using namespace std;

//------------------------------------------------------------------------------
Panel1::Panel1() :
    quetzal::gui::Panel("Panel1")
{
    subscribe("Text", "char", &Panel1::on_text_char);
    subscribe("OK", "press", &Panel1::on_ok_click);
    subscribe("Cancel", "press", &Panel1::on_cancel_click);
}

//------------------------------------------------------------------------------
Panel1::~Panel1()
{
}

//------------------------------------------------------------------------------
quetzal::gui::id_type Panel1::type() const
{
    return "Panel1";
}

//------------------------------------------------------------------------------
bool Panel1::on_text_char(quetzal::gui::EventArgs& ea)
{
    ea;

    return false;
}

//------------------------------------------------------------------------------
bool Panel1::on_ok_click(quetzal::gui::EventArgs& ea)
{
    ea;

    return false;
}

//------------------------------------------------------------------------------
bool Panel1::on_cancel_click(quetzal::gui::EventArgs& ea)
{
    ea;

    return false;
}
