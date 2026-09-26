//------------------------------------------------------------------------------
// stratum_sim
// file_components.cpp
//------------------------------------------------------------------------------

#include "file_components.hpp"
#include "properties.hpp"
#include <format>
#include <string>
#include <cassert>

namespace
{

    //--------------------------------------------------------------------------
    // File component names are entities in their own scope, separate from everything else in the world
    std::string file_component_path(std::string_view name)
    {
        return std::format("components::{}", name);
    }

} // namespace

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::register_file_component(flecs::world& entities, std::string_view name, flecs::entity_t component)
{
    assert(file_component(entities, name) == 0);
    assert(ecs_has(entities.c_ptr(), component, EcsStruct));

    register_inheritance(entities, component);
    return entities.entity(file_component_path(name).c_str()).set<FileComponent>(FileComponent {component});
}

//------------------------------------------------------------------------------
flecs::entity_t quetzal::stratum_sim::file_component(const flecs::world& entities, std::string_view name)
{
    flecs::entity registered = entities.lookup(file_component_path(name).c_str());
    return registered && registered.has<FileComponent>() ? registered.get<FileComponent>().component : 0;
}
