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
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <chrono>
#include <exception>
#include <filesystem>
#include <optional>

namespace
{

    // From the module's initialization to its uninitialization
    std::optional<quetzal::stratum_sim::PersistentSettings> g_user_settings;

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
    quetzal::log::logger().clear_sinks();

    return;
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
