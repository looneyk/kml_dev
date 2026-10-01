//------------------------------------------------------------------------------
// stratum_sim
// Session.cpp
//------------------------------------------------------------------------------

#include "Session.hpp"
#include "ai.hpp"
#include "attachments.hpp"
#include "DiscreteLayout.hpp"
#include "Site.hpp"
#include <format>
#include <stdexcept>
#include <cassert>

namespace
{

    //--------------------------------------------------------------------------
    // Moves the character to the placement of the zone's entry point. Returns false if the zone has none
    bool place_character(quetzal::stratum_sim::World& world, flecs::entity zone, flecs::entity access_type, quetzal::stratum_sim::level_type offset,
        flecs::entity character)
    {
        using namespace quetzal::stratum_sim;

        flecs::entity entry = entry_point(zone, access_type, offset);

        if (!entry)
        {
            return false;
        }

        world.relocate(character, zone, entry.get<AccessPoint>().placement);
        return true;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_sim::Session::Session(World& world, Character& character) :
    m_world(world),
    m_character(character),
    m_scheduler(),
    m_clock(m_world, m_scheduler),
    m_zone()
{
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Session::clear()
{
    // ...

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Session::start()
{
    // ...

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Session::pause()
{
    // ...

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Session::resume()
{
    // ...

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Session::enter(flecs::entity zone, flecs::entity access_type, level_type offset)
{
    assert(zone && zone.has<Zone>());
    assert(access_type);

    if (!place_character(m_world, zone, access_type, offset, m_character.entity()))
    {
        throw std::runtime_error(std::format("zone has no access point of the type '{}' whose target is {:+}", access_type.name().c_str(), offset));
    }

    play(zone);
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Session::set_driver(Driver driver, double units_per_second)
{
    m_clock.set_driver(driver, units_per_second);
    return;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::Session::loaded() const
{
    return static_cast<bool>(m_zone);
}

//------------------------------------------------------------------------------
quetzal::stratum_format::Notation quetzal::stratum_sim::Session::notation() const
{
    assert(loaded());
    return m_zone.get<ZoneNotation>().notation;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::World& quetzal::stratum_sim::Session::world()
{
    return m_world;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::World& quetzal::stratum_sim::Session::world() const
{
    return m_world;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Layout& quetzal::stratum_sim::Session::layout()
{
    assert(loaded());
    return m_world.layout(m_zone);
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Layout& quetzal::stratum_sim::Session::layout() const
{
    assert(loaded());
    return m_world.layout(m_zone);
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Scheduler& quetzal::stratum_sim::Session::scheduler()
{
    return m_scheduler;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Scheduler& quetzal::stratum_sim::Session::scheduler() const
{
    return m_scheduler;
}

//------------------------------------------------------------------------------
flecs::entity quetzal::stratum_sim::Session::zone() const
{
    return m_zone;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Character& quetzal::stratum_sim::Session::character()
{
    return m_character;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Character& quetzal::stratum_sim::Session::character() const
{
    return m_character;
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::stratum_sim::Session::advance(EventQueue& events, Clock::duration_type elapsed)
{
    id_type actor_controlled = static_cast<id_type>(m_character.entity().id());

    // Every actor but the character decides once per call; an actor fast enough to act twice repeats its decision
    think(m_world, actor_controlled);

    id_type actor_id = m_clock.advance(events, actor_controlled, elapsed);

    // The character went through a relocator into another zone
    if (flecs::entity zone = zone_of(m_character.entity()); zone != m_zone)
    {
        play(zone);
    }

    return actor_id;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Session::play(flecs::entity zone)
{
    assert(zone_of(m_character.entity()) == zone);

    // After the character has moved, so no zone it is in can unload
    m_world.trim_zones(zone);

    m_zone = zone;
    m_scheduler = Scheduler();
    m_character.entity().remove<PendingCommand>();

    with_in_zone(m_world.entities().query_builder<const Speed>(), zone)
        .build()
        .each([&](flecs::entity e, const Speed& speed) { m_scheduler.add(e.id(), speed.value); });

    return;
}
