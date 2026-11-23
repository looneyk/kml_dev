//------------------------------------------------------------------------------
// stratum_sim
// load_character.cpp
//------------------------------------------------------------------------------

#include "load_character.hpp"
#include "decode_components.hpp"
#include "EntityReference.hpp"
#include "Site.hpp"
#include "World.hpp"
#include "quetzal/stratum_format/CharacterDocument.hpp"
#include <format>
#include <unordered_map>
#include <cassert>

//------------------------------------------------------------------------------
std::optional<std::string> quetzal::stratum_sim::check_buildable(const flecs::world& world, const stratum_format::CharacterDocument& document)
{
    // Flecs' error logging is silenced, since the returned error is the report
    int log_level = ecs_log_set_level(-4); // Errors log at -3
    std::optional<std::string> error = check_decodable(world, document.components, "character");

    for (size_t i = 0; i < document.inventory.size() && !error.has_value(); ++i)
    {
        error = check_decodable(world, document.inventory[i].components, std::format("character.inventory[{}]", i));
    }

    ecs_log_set_level(log_level);
    return error;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::load_character(flecs::entity character, const stratum_format::CharacterDocument& document)
{
    assert(character);

    flecs::world world = character.world();
    character.set<PersistentId>(PersistentId{document.id});

    std::unordered_map<stratum_format::stored_entity_id_type, flecs::entity> entities{{document.id, character}}; // <file_id, entity>

    // Every entity is created first, so each has its persistent id before a parent refers to it
    for (const stratum_format::Entity& entity : document.inventory)
    {
        flecs::entity e = world.entity().set<PersistentId>(PersistentId{entity.id});

        // A type that is not loaded is made with no components, so the entity keeps its type
        if (!entity.type.empty())
        {
            flecs::entity type = entity_type(world, entity.type);
            e.is_a(type ? type : create_entity_type(world, entity.type));
        }

        entities.emplace(entity.id, e);
    }

    for (const stratum_format::Entity& entity : document.inventory)
    {
        flecs::entity e = entities.at(entity.id);
        set_parent(e, entities.at(*entity.parent_id));

        if (entity.site.has_value())
        {
            Site site{entity.site->relation == stratum_format::Relation::On ? Relation::On : Relation::In, {}, {}};

            if (entity.site->position.has_value())
            {
                site.placement.position() = point_type(entity.site->position->x(), entity.site->position->y(), value_type(0));
            }

            if (entity.site->orientation.has_value())
            {
                site.placement.direction() = vector_type(static_cast<value_type>(entity.site->orientation->x()),
                    static_cast<value_type>(entity.site->orientation->y()), value_type(0));
            }

            e.set<Site>(site);
        }
    }

    // Every id is persistent, and a reference is resolved when it is first followed
    auto resolve = [](stratum_format::stored_entity_id_type id) { return EntityReference{PersistentId{id}, flecs::entity()}; };

    decode_components(character, document.components, "character", resolve);

    for (size_t i = 0; i < document.inventory.size(); ++i)
    {
        decode_components(entities.at(document.inventory[i].id), document.inventory[i].components, std::format("character.inventory[{}]", i), resolve);
    }

    return;
}
