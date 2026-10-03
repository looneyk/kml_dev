#if !defined(QUETZAL_STRATUM_SIM_DECODE_COMPONENTS_HPP)
#define QUETZAL_STRATUM_SIM_DECODE_COMPONENTS_HPP
//------------------------------------------------------------------------------
// stratum_sim
// decode_components.hpp
//------------------------------------------------------------------------------

#include "PersistentId.hpp"
#include "quetzal/stratum_format/types.hpp"
#include <flecs.h>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace quetzal::stratum_sim
{

    // Returns the reference for an id that a component object refers to
    using resolve_function_type = std::function<EntityReference(stratum_format::stored_entity_id_type)>;

    // Returns the name and value of each component object, or nothing if there are no components
    std::vector<std::pair<std::string, const stratum_format::raw_values_type*>> component_objects(const stratum_format::raw_values_type& components);

    // The path to a component object in a file, for the messages its reader writes
    std::string component_path(const std::string& path, const std::string& name);

    // Checks, by decoding each into a value that is then discarded, that the component objects can be decoded. Returns the
    // first reason one cannot, or none: a component object that has no component registered under its name or does not
    // fit that component. Flecs' error logging is the caller's to silence
    std::optional<std::string> check_decodable(const flecs::world& world, const stratum_format::raw_values_type& components, const std::string& path);

    // Sets each component object on the entity as the component registered under its name, which check_decodable has
    // checked. A component that inherits gets only the fields the object sets, leaving the rest to the property chain. Any
    // other component is decoded over the entity's current value, which is a copy of its type's value if it has a type
    void decode_components(flecs::entity e, const stratum_format::raw_values_type& components, const std::string& path,
        const resolve_function_type& resolve);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_DECODE_COMPONENTS_HPP
