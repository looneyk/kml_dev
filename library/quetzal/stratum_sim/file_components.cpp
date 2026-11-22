//------------------------------------------------------------------------------
// stratum_sim
// file_components.cpp
//------------------------------------------------------------------------------

#include "file_components.hpp"
#include "properties.hpp"
#include <algorithm>
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
    assert(is_tag(entities, component) || ecs_has(entities.c_ptr(), component, EcsStruct) || ecs_has(entities.c_ptr(), component, EcsEnum)
        || ecs_has(entities.c_ptr(), component, EcsOpaque));

    register_inheritance(entities, component);
    return entities.entity(file_component_path(name).c_str()).set<FileComponent>(FileComponent{component});
}

//------------------------------------------------------------------------------
flecs::entity_t quetzal::stratum_sim::file_component(const flecs::world& entities, std::string_view name)
{
    flecs::entity registered = entities.lookup(file_component_path(name).c_str());
    return registered && registered.has<FileComponent>() ? registered.get<FileComponent>().component : 0;
}

//------------------------------------------------------------------------------
std::vector<std::pair<std::string, flecs::entity_t>> quetzal::stratum_sim::file_components(const flecs::world& entities)
{
    std::vector<std::pair<std::string, flecs::entity_t>> components;

    entities.each([&](flecs::entity registered, const FileComponent& file_component)
    {
        components.emplace_back(registered.name().c_str(), file_component.component);
    });

    std::ranges::sort(components);
    return components;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::is_tag(const flecs::world& entities, flecs::entity_t component)
{
    return ecs_get_type_info(entities.c_ptr(), component) == nullptr;
}
