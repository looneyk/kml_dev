#if !defined(STRATUM_GODOT_REGISTER_TYPES_HPP)
#define STRATUM_GODOT_REGISTER_TYPES_HPP
//------------------------------------------------------------------------------
// stratum_godot
// register_types.hpp
//------------------------------------------------------------------------------

#include "quetzal/stratum_format/GameManifestDocument.hpp"
#include <godot_cpp/core/class_db.hpp>

void initialize_stratum_module(godot::ModuleInitializationLevel level);
void uninitialize_stratum_module(godot::ModuleInitializationLevel level);

// Returns the game manifest that the module loaded from the manifest= user argument. Only a game has one, not the editor
const quetzal::stratum_format::GameManifestDocument& game_manifest();

#endif // STRATUM_GODOT_REGISTER_TYPES_HPP
