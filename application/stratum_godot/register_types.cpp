//------------------------------------------------------------------------------
// stratum_godot
// register_types.cpp
//------------------------------------------------------------------------------

#include "register_types.hpp"
#include "StratumWorld.hpp"
#include "quetzal/log/FileLogSink.hpp"
#include "quetzal/log/Log.hpp"
#include "quetzal/log/timestamped_files.hpp"
#include "quetzal/settings/Settings.hpp"
#include "quetzal/stratum_sim/Config.hpp"
#include "quetzal/stratum_sim/PersistentSettings.hpp"
#include "quetzal/stratum_sim/setting_definitions.hpp"
#include <gdextension_interface.h>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <cassert>

namespace
{

    // From the module's initialization to its uninitialization
    std::optional<quetzal::stratum_sim::PersistentSettings> g_user_settings;
    std::optional<quetzal::stratum_format::GameManifestDocument> g_game_manifest;

    constexpr std::string_view manifest_argument_prefix = "manifest=";

    //--------------------------------------------------------------------------
    // Reads the manifest that the manifest= argument after -- on Godot's command line refers to, relative to the assets
    // directory. Throws std::runtime_error if there is no such argument, or the manifest lists a view other than the tile view
    quetzal::stratum_format::GameManifestDocument load_game_manifest_argument(const std::filesystem::path& assets_directory)
    {
        const godot::PackedStringArray user_args = godot::OS::get_singleton()->get_cmdline_user_args();
        for (int64_t i = 0; i < user_args.size(); ++i)
        {
            const godot::CharString utf8 = user_args[i].utf8();
            const std::string_view arg(utf8.get_data(), static_cast<size_t>(utf8.length()));
            if (!arg.starts_with(manifest_argument_prefix))
            {
                continue;
            }

            const std::u8string manifest_file(reinterpret_cast<const char8_t*>(arg.data() + manifest_argument_prefix.size()),
                arg.size() - manifest_argument_prefix.size());
            quetzal::stratum_format::GameManifestDocument manifest = quetzal::stratum_format::read_game_manifest(assets_directory / manifest_file);

            for (const std::string& view_name : manifest.views)
            {
                if (view_name != "tile")
                {
                    throw std::runtime_error(std::format("the Godot host has no view named '{}'", view_name));
                }
            }

            return manifest;
        }

        throw std::runtime_error("no manifest= argument follows -- on Godot's command line");
    }

    //--------------------------------------------------------------------------
    // Errors and warnings as Godot's, so the editor marks them, and the rest as printed lines
    void write_to_godot_output(const quetzal::log::LogEntry& entry)
    {
        godot::String line = quetzal::log::format_entry(entry).c_str();

        if (entry.level >= quetzal::log::LogLevel::Error)
        {
            godot::UtilityFunctions::push_error(line);
        }
        else if (entry.level == quetzal::log::LogLevel::Warning)
        {
            godot::UtilityFunctions::push_warning(line);
        }
        else
        {
            godot::UtilityFunctions::print(line);
        }

        return;
    }

} // namespace

//------------------------------------------------------------------------------
void initialize_stratum_module(godot::ModuleInitializationLevel level)
{
    if (level != godot::MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }

    quetzal::log::logger().add_sink(write_to_godot_output);
    const quetzal::stratum_sim::Config config = quetzal::stratum_sim::load_config(project_directory());
    const std::filesystem::path log_base_filepath = config.logs_directory / "stratum_godot.log";
    quetzal::log::logger().add_sink(quetzal::log::FileLogSink(quetzal::log::timestamped_filepath(log_base_filepath, std::chrono::system_clock::now())));
    for (const auto& unremoved_file : quetzal::log::remove_oldest_timestamped_files(log_base_filepath, config.log_file_count_max))
    {
        quetzal::log::logger().warning("log", "The old log file {} was not removed: {}", unremoved_file.filepath.generic_string(),
            unremoved_file.error.message());
    }

    quetzal::stratum_sim::load_setting_definitions(config.assets_directory, quetzal::settings::store());
    g_user_settings.emplace(config.user_settings_filepath, quetzal::stratum_sim::user_setting_scope, quetzal::settings::store());
    g_user_settings->load();

    // An exception that escapes into Godot ends the process without its message, and a module's initialization cannot fail.
    // quick_exit, since destroying the module's statics while Godot is initializing crashes
    if (!godot::Engine::get_singleton()->is_editor_hint())
    {
        try
        {
            g_game_manifest = load_game_manifest_argument(config.assets_directory);
        }
        catch (const std::exception& e)
        {
            quetzal::log::logger().error("stratum_godot", "The game manifest was not loaded: {}", e.what());
            std::quick_exit(EXIT_FAILURE);
        }
    }

    GDREGISTER_CLASS(StratumWorld);

    return;
}

//------------------------------------------------------------------------------
void uninitialize_stratum_module(godot::ModuleInitializationLevel level)
{
    if (level != godot::MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }

    try
    {
        g_user_settings->save_if_changed();
    }
    catch (const std::exception& e)
    {
        quetzal::log::logger().error("settings", "The user settings file was not saved: {}", e.what());
    }

    g_user_settings.reset();
    g_game_manifest.reset();
    quetzal::log::logger().clear_sinks();

    return;
}

//------------------------------------------------------------------------------
const quetzal::stratum_format::GameManifestDocument& game_manifest()
{
    assert(g_game_manifest.has_value());
    return *g_game_manifest;
}

extern "C"
{

//------------------------------------------------------------------------------
// Named by entry_symbol in stratum.gdextension, and the only symbol Godot looks up
GDExtensionBool GDE_EXPORT stratum_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization* r_initialization)
{
    godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

    init_obj.register_initializer(initialize_stratum_module);
    init_obj.register_terminator(uninitialize_stratum_module);
    init_obj.set_minimum_library_initialization_level(godot::MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_obj.init();
}

} // extern "C"
