#if !defined(STRATUM_GODOT_REGISTER_TYPES_HPP)
#define STRATUM_GODOT_REGISTER_TYPES_HPP
//------------------------------------------------------------------------------
// stratum_godot
// register_types.hpp
//------------------------------------------------------------------------------

#include <godot_cpp/core/class_db.hpp>

void initialize_stratum_module(godot::ModuleInitializationLevel level);
void uninitialize_stratum_module(godot::ModuleInitializationLevel level);

#endif // STRATUM_GODOT_REGISTER_TYPES_HPP
