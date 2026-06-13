//------------------------------------------------------------------------------
// gallery_vk
// main.cpp
//------------------------------------------------------------------------------

#include "Application.hpp"
#include "quetzal/common/Config.hpp"
#include <exception>
#include <iostream>

using namespace std;

int main()
{
    quetzal::Config config;
    config.insert("application_name", "gallery_vk");
    config.insert("application_version", "0");
    config.insert("engine_name", "engine");
    config.insert("engine_version", "0");
    config.insert("mode", "windowed");
    config.insert("title", "gallery_vk");
    config.insert("x", "0");
    config.insert("y", "0");
    config.insert("width", "800");
    config.insert("height", "600");
    config.insert("cursor", "disabled");
    config.insert("swap_frame_count", "2");

    quetzal::gallery_vk::Application application(config);

    try
    {
        application.run();
    }
    catch (const exception& e)
    {
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
