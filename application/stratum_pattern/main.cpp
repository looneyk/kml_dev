//------------------------------------------------------------------------------
// stratum_pattern
// main.cpp
//------------------------------------------------------------------------------

#include "Application.hpp"
#include <exception>
#include <iostream>
#include <string>

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    std::string filepath_level = "data/dungeon.level";
    std::string script;
    bool incremental = false;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "--script" && i + 1 < argc)
        {
            script = argv[++i];
        }
        else if (arg == "--incremental")
        {
            incremental = true;
        }
        else if (arg.starts_with("--"))
        {
            std::cerr << "Usage: stratum_pattern [level] [--script keys] [--incremental]" << std::endl;
            return 1;
        }
        else
        {
            filepath_level = arg;
        }
    }

    try
    {
        stratum_pattern::Application application(filepath_level, script, incremental || script.empty());
        return application.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 1;
}
