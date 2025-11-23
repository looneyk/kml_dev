
#include <string>

class IWidget
{
public:

    virtual void invalidate() = 0;
};

class Widget : virtual public IWidget
{
public:

    void invalidate()
    {
        return;
    }
};

class Text : virtual public IWidget
{
    void set_text(const std::string& text)
    {
        text;

        invalidate();

        return;
    };
};

class WidgetText : public Widget, public Text
{
};

int main(int argc, char* argv[])
{
    argc;
    argv;

    WidgetText x;

    return 0;
}
