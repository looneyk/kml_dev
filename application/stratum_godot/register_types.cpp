//------------------------------------------------------------------------------
// stratum_godot
// register_types.cpp
//------------------------------------------------------------------------------

#include "register_types.hpp"
#include "StratumWorld.hpp"
#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

//------------------------------------------------------------------------------
void initialize_stratum_module(godot::ModuleInitializationLevel level)
{
    if (level != godot::MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
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
