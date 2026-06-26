//------------------------------------------------------------------------------
// Light Matrix
// main.cpp
//------------------------------------------------------------------------------

#include "Application.hpp"
#include "quetzal/common/Config.hpp"
#include <exception>
#include <iostream>

using namespace std;

//------------------------------------------------------------------------------
int main()
{
    quetzal::Config config;
    config.insert("application_name", "lightmatrix");
    config.insert("application_version", "0");
    config.insert("engine_name", "engine");
    config.insert("engine_version", "0");
    config.insert("mode", "windowed");
    config.insert("title", "Light Matrix");
    config.insert("x", "100");
    config.insert("y", "100");
    config.insert("width", "800");
    config.insert("height", "800");
    config.insert("cursor", "disabled");
    config.insert("swap_frame_count", "2");
    config.insert("assets_path", ".");
    config.insert("shared_assets_path", "");
    config.insert("shaders_path", "../../library/quetzal/scene_graph/shaders");

    try
    {
        quetzal::lightmatrix::Application application(config);
        application.run();
    }
    catch (const exception& e)
    {
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
