//------------------------------------------------------------------------------
// stratum_godot
// register_types.cpp
//------------------------------------------------------------------------------

#include "register_types.hpp"
#include "StratumWorld.hpp"
#include "quetzal/log/FileLogSink.hpp"
#include "quetzal/log/Log.hpp"
#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <filesystem>

namespace
{

    //--------------------------------------------------------------------------
    // Temporary, until the config file is read: the log is in a logs directory beside the saves and working directories
    std::filesystem::path temporary_log_filepath()
    {
        return std::filesystem::temp_directory_path() / "KML/Stratum/logs/stratum_godot.log";
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
    quetzal::log::logger().add_sink(quetzal::log::FileLogSink(temporary_log_filepath()));

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
