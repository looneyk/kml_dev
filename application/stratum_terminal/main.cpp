//------------------------------------------------------------------------------
// stratum_terminal
// main.cpp
//------------------------------------------------------------------------------

#include "Application.hpp"
#include <exception>
#include <iostream>
#include <string>

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    std::string filepath_zone = std::string(KML_DEV_ASSETS_ROOT) + "/zones/dungeon.zone";
    std::string script;
    bool incremental = false;
    stratum_terminal::View view = stratum_terminal::View::Null;

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
        else if (arg == "--cell")
        {
            view = stratum_terminal::View::Cell;
        }
        else if (arg == "--wall")
        {
            view = stratum_terminal::View::Wall;
        }
        else if (arg.starts_with("--"))
        {
            std::cerr << "Usage: stratum_terminal [zone] [--cell|--wall] [--script keys] [--incremental]" << std::endl;
            return 1;
        }
        else
        {
            filepath_zone = arg;
        }
    }

    try
    {
        stratum_terminal::Application application(filepath_zone, view, script, incremental || script.empty());
        return application.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 1;
}
