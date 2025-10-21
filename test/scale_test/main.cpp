//------------------------------------------------------------------------------
//  main.cpp
//
//------------------------------------------------------------------------------

#include <string>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <cassert>

namespace
{

    const std::string note[12] =
    {
        "A",
        "A#",
        "B",
        "C",
        "C#",
        "D",
        "D#",
        "E",
        "F",
        "F#",
        "G",
        "G#"
    };

}  // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    double d = pow(2., 1. / 12.);

    for (int i = 0; i <= 12; ++i)
    {
        std::cout << std::setw(2) << std::right << i << "  " << std::setw(2) << std::left << note[i % 12] << "  " << pow(d, i) << std::endl;
    }

    std::cout << std::endl;

    return 0;
}
