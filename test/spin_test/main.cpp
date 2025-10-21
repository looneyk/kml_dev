//------------------------------------------------------------------------------
//  main.cpp
//
//------------------------------------------------------------------------------

#include <string>
#include <iostream>
#include <cassert>

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    int secondary_axis_distance = 3;

    for (int i = 0; i < 10; ++i)
    {
        std::cout << std::string(i + abs(i - secondary_axis_distance), '*') << std::endl;
    }

    std::cout << std::endl;

    for (int i = 0; i < 10; ++i)
    {
        std::cout << std::string(i * 2 + abs(i - secondary_axis_distance) * 2, '*') << std::endl;
    }

    std::cout << std::endl;

    for (int i = 0; i < 10; ++i)
    {
        std::cout << std::string(i * 2 + abs(i - secondary_axis_distance) * 4, '*') << std::endl;
    }

    std::cout << std::endl;

    for (int i = 0; i < 10; ++i)
    {
        std::cout << std::string(i * 2 + abs(i - secondary_axis_distance) * 1, '*') << std::endl;
    }

    std::cout << std::endl;

    for (int i = 0; i < 10; ++i)
    {
        std::cout << std::string(i * 2 + abs(i - secondary_axis_distance) * 3, '*') << std::endl;
    }

    return 0;
}
