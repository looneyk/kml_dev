//------------------------------------------------------------------------------
// stratum_sim
// attachments.cpp
//------------------------------------------------------------------------------

#include "attachments.hpp"
#include "file_components.hpp"
#include "layout_entities.hpp"
#include "properties.hpp"
#include "Site.hpp"
#include "Zone.hpp"
#include <algorithm>

//------------------------------------------------------------------------------
void quetzal::stratum_sim::register_attachment_components(flecs::world& entities)
{
    entities.component<AccessState>();
    entities.component<Access>().member(&Access::state, "state");
    entities.component<DetectionState>();
    entities.component<ActorDetection>().member(&ActorDetection::actor, "actor").member(&ActorDetection::state, "state");
    register_vector<ActorDetection>(entities);
    entities.component<Detection>().member(&Detection::state, "state").member(&Detection::actor_states, "actor_states");

    register_file_component<Access>(entities, "access");
    register_file_component<Detection>(entities, "detection");

    return;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::access_entity(flecs::entity e)
{
    for (flecs::entity holder = e; holder && !holder.has<Zone>(); holder = holder.parent())
    {
        if (holder.has<Access>())
        {
            return holder;
        }
    }

    return flecs::entity();
}

//------------------------------------------------------------------------------
std::optional<quetzal::stratum_sim::AccessState> quetzal::stratum_sim::access(flecs::entity e)
{
    flecs::entity holder = access_entity(e);
    return holder ? std::optional<AccessState>(holder.get<Access>().state) : std::nullopt;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::DetectionState quetzal::stratum_sim::detection(flecs::entity e, flecs::entity actor)
{
    for (flecs::entity holder = e; holder && !holder.has<Zone>(); holder = holder.parent())
    {
        if (const auto* detection = holder.try_get<Detection>())
        {
            if (const auto* actor_id = actor ? actor.try_get<PersistentId>() : nullptr)
            {
                auto i = std::ranges::find(detection->actor_states, *actor_id, &ActorDetection::actor);
                if (i != detection->actor_states.end())
                {
                    return i->state;
                }
            }

            return detection->state;
        }
    }

    return DetectionState::Visible;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::find_access_attachment(flecs::entity zone, id_type wall_id)
{
    flecs::entity wall = wall_entity(zone, wall_id);
    const auto* attached = wall ? wall.try_get<AttachedEntities>() : nullptr;

    if (attached == nullptr)
    {
        return flecs::entity();
    }

    auto i = std::find_if(attached->entities.begin(), attached->entities.end(), [](flecs::entity e) { return static_cast<bool>(access_entity(e)); });
    return i != attached->entities.end() ? *i : flecs::entity();
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::crossable(flecs::entity zone, id_type wall_id, flecs::entity actor)
{
    flecs::entity wall = wall_entity(zone, wall_id);
    const auto* attached = wall ? wall.try_get<AttachedEntities>() : nullptr;

    if (attached == nullptr)
    {
        return false;
    }

    return std::any_of(attached->entities.begin(), attached->entities.end(), [&](flecs::entity e)
    {
        return access(e) == AccessState::Open && detection(e, actor) != DetectionState::Concealed;
    });
}
