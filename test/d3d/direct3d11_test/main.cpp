//------------------------------------------------------------------------------
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/direct3d11/Application.hpp"

using namespace std;

//------------------------------------------------------------------------------
int APIENTRY wWinMain(_In_ HINSTANCE hinstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    quetzal::direct3d11::Application a(hinstance, "d3d11_test");
    a.run();
    return 0;
}
