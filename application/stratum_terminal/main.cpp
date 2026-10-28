//------------------------------------------------------------------------------
// stratum_terminal
// main.cpp
//------------------------------------------------------------------------------

#include "Application.hpp"
#include "quetzal/common/file_util.hpp"
#include "quetzal/common/KeyValueStore.hpp"
#include "quetzal/common/string_util.hpp"
#include "quetzal/stratum_format/GameManifestDocument.hpp"
#include "quetzal/stratum_sim/Config.hpp"
#include <exception>
#include <iostream>
#include <optional>
#include <string>

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    static constexpr auto usage_string = "Usage: stratum_terminal manifest=file [script=keys] [incremental=true|false]";

    quetzal::KeyValueStore tokens;
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        size_t equals = arg.find('=');
        if (equals == std::string::npos || equals == 0)
        {
            std::cerr << usage_string << std::endl;
            return 1;
        }

        tokens.insert(arg.substr(0, equals), arg.substr(equals + 1));
    }

    try
    {
        std::optional<std::string> manifest_file = tokens.value("manifest");
        if (!manifest_file)
        {
            std::cerr << usage_string << std::endl;
            return 1;
        }

        tokens.remove("manifest");

        std::string script = tokens.value_or("script", std::string());
        tokens.remove("script");

        std::optional<std::string> incremental_text = tokens.value("incremental");
        bool incremental = incremental_text ? quetzal::to_bool(*incremental_text) : script.empty();
        tokens.remove("incremental");

        if (!tokens.empty())
        {
            std::cerr << usage_string << std::endl;
            return 1;
        }

        // The manifest file is relative to the assets directory
        const quetzal::stratum_sim::Config config = quetzal::stratum_sim::load_config(quetzal::executable_directory());
        const quetzal::stratum_format::GameManifestDocument manifest = quetzal::stratum_format::read_game_manifest(config.assets_directory / *manifest_file);

        stratum_terminal::Application application(config, manifest, script, incremental);
        return application.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 1;
}
