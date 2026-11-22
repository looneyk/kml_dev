#if !defined(QUETZAL_STRATUM_SIM_FILE_COMPONENTS_HPP)
#define QUETZAL_STRATUM_SIM_FILE_COMPONENTS_HPP
//------------------------------------------------------------------------------
// stratum_sim
// file_components.hpp
//------------------------------------------------------------------------------

#include <flecs.h>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Set on the entity registered under a file component's name. It refers to the component that the file component's
    // object in a zone file decodes into
    struct FileComponent
    {
        flecs::entity_t component = 0;
    };

    //--------------------------------------------------------------------------
    // Makes the file component with this name in a zone file load as the component. The component's reflection, as a struct,
    // an enumeration, or an opaque type, must already be registered with flecs, unless it is a tag, which a zone file writes as an empty
    // object. A component with optional fields also inherits (register_inheritance)
    flecs::entity register_file_component(flecs::world& entities, std::string_view name, flecs::entity_t component);

    template<typename T>
    flecs::entity register_file_component(flecs::world& entities, std::string_view name);

    // Returns zero if no component is registered under the name
    flecs::entity_t file_component(const flecs::world& entities, std::string_view name);

    // Returns each registered file component, with its name, in name order
    std::vector<std::pair<std::string, flecs::entity_t>> file_components(const flecs::world& entities);

    // Returns true if the component is a tag, which has no value
    bool is_tag(const flecs::world& entities, flecs::entity_t component);

} // namespace quetzal::stratum_sim

//------------------------------------------------------------------------------
template<typename T>
flecs::entity quetzal::stratum_sim::register_file_component(flecs::world& entities, std::string_view name)
{
    return register_file_component(entities, name, entities.id<T>());
}

#endif // QUETZAL_STRATUM_SIM_FILE_COMPONENTS_HPP
