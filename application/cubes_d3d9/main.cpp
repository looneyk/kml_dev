//------------------------------------------------------------------------------
//
// Copyright (c) 2010 Keith Michael Looney. All Rights Reserved.
//
//------------------------------------------------------------------------------
//
// Cubes
// main.cpp
//
//------------------------------------------------------------------------------

#include <windows.h>
#include "application.hpp"

//------------------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    hPrevInstance;
    lpCmdLine;
    nCmdShow;

#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    quetzal::cubes::application application(hInstance, L"Cubes", D3DDEVTYPE_HAL, D3DCREATE_HARDWARE_VERTEXPROCESSING);

    return application.run();
}
