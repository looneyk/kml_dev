//------------------------------------------------------------------------------
// roguelike
// main.cpp
//------------------------------------------------------------------------------

#include "Application.hpp"
#include "quetzal/string_util.hpp"

using namespace std;

//------------------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    hPrevInstance;
    lpCmdLine;
    nCmdShow;

    try
    {
        quetzal::roguelike::Application application(hInstance, "roguelike");
        application.run();
    }
    catch (exception& e)
    {
        MessageBox(nullptr, quetzal::to_type<wstring>(e.what()).c_str(), L"Exception", MB_OK);
    }

    return 0;
}

/*
    auto item_factory = ItemFactory::instance();

    load template resources

    new
        generate entry level
    open
        load saved dungeon

    for (;;)
    {
        bool b = update();
        if (!b)
        {
            break;
        }
        render();
    }
*/

bool update()
{
/*
    if dialog shown
    {
        dialog.update
        return true
    }

    check input
    if wasd/arrow key
        character.move
    j:
        journal dialog
    m:
        map dialog
    i:
        inventory dialog
    x:
        if not saved
            ask save
            if save
                save
            if cancel
                break;
        return false;

    attack/shoot
    block
    reload
    quaff
    eat
    open (door/chest)
    use ability
    use item
*/
    return true;
}
