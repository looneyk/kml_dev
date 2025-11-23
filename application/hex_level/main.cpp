//------------------------------------------------------------------------------
// hex_level
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
        quetzal::hex_level::Application application(hInstance, "hex_level");
        application.run();
    }
    catch (exception& e)
    {
        MessageBox(nullptr, quetzal::to_type<wstring>(e.what()).c_str(), L"Exception", MB_OK);
    }

    return 0;
}
