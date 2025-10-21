//------------------------------------------------------------------------------
// main.cpp
//------------------------------------------------------------------------------

#include "Application.hpp"
#include "quetzal/string_util.hpp"

using namespace std;

//------------------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    hPrevInstance;
    lpCmdLine;
    nCmdShow;

    try
    {
        quetzal::direct3d11_box::Application application(hinstance, "d3d11_box_test");
        application.run();
    }
    catch (exception& e)
    {
        MessageBox(nullptr, quetzal::to_type<wstring>(e.what()).c_str(), L"Exception", MB_OK);
    }

    return 0;
}
