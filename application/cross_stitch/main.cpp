//------------------------------------------------------------------------------
// main.cpp
//
//------------------------------------------------------------------------------

#include "CrossStitch.hpp"
#include <exception>

using namespace std;

//------------------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    hPrevInstance;
    pCmdLine;

    HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0);

    try
    {
        quetzal::CrossStitch application(hInstance, nCmdShow);

        application.run();
    }
    catch (exception& e)
    {
        e; // ...
    }

    return 0;
}
