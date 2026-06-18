//------------------------------------------------------------------------------
// Cubes
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
    config.insert("application_name", "cubes");
    config.insert("application_version", "0");
    config.insert("engine_name", "engine");
    config.insert("engine_version", "0");
    config.insert("mode", "windowed");
    config.insert("title", "Cubes");
    config.insert("x", "0");
    config.insert("y", "0");
    config.insert("width", "800");
    config.insert("height", "600");
    config.insert("cursor", "disabled");
    config.insert("swap_frame_count", "2");
    config.insert("assets_path", "C:/Users/Keith/Documents/kml/dev/application/cubes");
    config.insert("shared_assets_path", "C:/Users/Keith/Documents/kml/dev/application/assets");
    config.insert("shaders_path", "C:/Users/Keith/Documents/kml/dev/library/quetzal/scene_graph/shaders");

    try
    {
        quetzal::cubes::Application application(config);
        application.run();
    }
    catch (const exception& e)
    {
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
