//------------------------------------------------------------------------------
// framework_test
// main.cpp
//------------------------------------------------------------------------------

#include "Application.hpp"

//------------------------------------------------------------------------------
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    hInstance;
    hPrevInstance;
    lpCmdLine;
    nCmdShow;

    quetzal::framework_test::Application application;

    application.run();

    return 0;
}
