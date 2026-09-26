#if !defined(QUETZAL_STRATUM_SIM_FILE_COMPONENTS_HPP)
#define QUETZAL_STRATUM_SIM_FILE_COMPONENTS_HPP
//------------------------------------------------------------------------------
// stratum_sim
// file_components.hpp
//------------------------------------------------------------------------------

#include <flecs.h>
#include <string_view>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // On the entity registered under a file component's name: the component its object in a zone file decodes into
    struct FileComponent
    {
        flecs::entity_t component = 0;
    };

    //--------------------------------------------------------------------------
    // Makes name, in a zone file's components, load as this component. The component's fields must already be registered
    // with flecs; a component with optional fields also inherits (register_inheritance)
    flecs::entity register_file_component(flecs::world& entities, std::string_view name, flecs::entity_t component);

    template<typename T>
    flecs::entity register_file_component(flecs::world& entities, std::string_view name);

    // Zero where no component is registered under the name
    flecs::entity_t file_component(const flecs::world& entities, std::string_view name);

} // namespace quetzal::stratum_sim

//------------------------------------------------------------------------------
template<typename T>
flecs::entity quetzal::stratum_sim::register_file_component(flecs::world& entities, std::string_view name)
{
    return register_file_component(entities, name, entities.id<T>());
}

#endif // QUETZAL_STRATUM_SIM_FILE_COMPONENTS_HPP
