//------------------------------------------------------------------------------
// testing
// main.cpp
//------------------------------------------------------------------------------

// The one translation unit that compiles doctest's implementation, for every test executable that links this library
#define DOCTEST_CONFIG_IMPLEMENT
#include "reference_files.hpp"
#include <doctest/doctest.h>
#include <string_view>
#include <vector>

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    std::vector<char*> arguments;

    for (int i = 0; i < argc; ++i)
    {
        if (std::string_view(argv[i]) == "--update-references")
        {
            quetzal::testing::detail::set_updating_references(true);
            continue;
        }

        arguments.push_back(argv[i]);
    }

    doctest::Context context(static_cast<int>(arguments.size()), arguments.data());
    return context.run();
}
