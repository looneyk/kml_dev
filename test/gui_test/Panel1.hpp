//------------------------------------------------------------------------------
//  Panel1.hpp
//
//------------------------------------------------------------------------------

#if !defined(GUI_TEST_PANEL1_HPP)
#define GUI_TEST_PANEL1_HPP

#include "quetzal/gui/widgets/Panel.hpp"

//------------------------------------------------------------------------------
class Panel1 : public quetzal::gui::Panel
{
public:

    Panel1();
	Panel1(const Panel1&) = delete;
	~Panel1();

	Panel1& operator=(const Panel1&) = delete;

    virtual quetzal::gui::id_type type() const;

    // constructor, function, ... to initialize event mapping

// inherited ...
//    void open(Resource& resource, const std::string& strPanel);
//    bool present(bool bModal);

protected:

    bool on_text_char(quetzal::gui::EventArgs& ea);
    bool on_ok_click(quetzal::gui::EventArgs& ea);
    bool on_cancel_click(quetzal::gui::EventArgs& ea);

private:
};

#endif // GUI_TEST_PANEL1_HPP
