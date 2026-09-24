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
    // The player moved to the entry of the zone's entry point of the type. False when the zone has no access point of
    // the type
    bool place_player(quetzal::stratum_sim::World& world, flecs::entity zone, flecs::entity access_type, flecs::entity player)
    {
        using namespace quetzal::stratum_sim;

        flecs::entity entry = entry_point(zone, access_type);

        if (!entry)
        {
            return false;
        }

        world.relocate(player, zone, entry.get<AccessPoint>().entry);
        return true;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_sim::Session::Session(World& world, Player& player) :
    m_world(world),
    m_player(player),
    m_scheduler(),
    m_clock(m_world, m_scheduler),
    m_command_pending(),
    m_zone()
{
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Session::create(const Generator& generator, flecs::entity access_type)
{
    enter(m_world.create_zone(generator), access_type);
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Session::load(const std::filesystem::path& filepath, flecs::entity access_type)
{
    flecs::entity zone = m_world.load_zone(filepath);

    try
    {
        enter(zone, access_type);
    }
    catch (const std::runtime_error& e)
    {
        throw std::runtime_error(std::format("{}: {}", e.what(), filepath.string()));
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Session::enter(flecs::entity zone, flecs::entity access_type)
{
    assert(zone && zone.has<Zone>());
    assert(access_type);

    if (!place_player(m_world, zone, access_type, m_player.entity()))
    {
        throw std::runtime_error(std::format("zone has no access point of the type '{}'", access_type.name().c_str()));
    }

    // After the player has moved, so no zone it is in can unload
    m_world.trim_zones(zone);

    m_zone = zone;
    m_scheduler = Scheduler(); // actual session state ...
    m_command_pending = Command(); // actual session state ...

    m_world.entities().query_builder<const Speed>()
        .with(flecs::ChildOf, zone)
        .build()
        .each([&](flecs::entity e, const Speed& speed) { m_scheduler.add(e.id(), speed.value); });

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Session::set_driver(Driver driver, double units_per_second)
{
    m_clock.set_driver(driver, units_per_second); // actual session state ...
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
flecs::entity quetzal::stratum_sim::Session::player() const
{
    return m_player.entity();
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Session::set_intent(const Intent& intent)
{
    assert(loaded());
    m_player.entity().set<Intent>(intent);

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Session::submit(const Command& command)
{
    m_command_pending = command;
    return;
}

//------------------------------------------------------------------------------
quetzal::id_type quetzal::stratum_sim::Session::advance(EventQueue& events, Clock::duration_type elapsed)
{
    id_type actor_controlled = static_cast<id_type>(m_player.entity().id());

    // Every actor but the player decides once per call; an actor fast enough to act twice repeats its decision
    think(m_world, actor_controlled);

    id_type actor_id = m_clock.advance(events, actor_controlled, elapsed);

    for (;;)
    {
        if (actor_id == nullid)
        {
            return nullid;
        }

        if (m_command_pending.empty())
        {
            return actor_id;
        }

        int cost = apply(m_world, m_command_pending, events);
        m_command_pending = Command();

        if (cost > 0)
        {
            m_scheduler.spend(actor_id, cost);
        }

        // Zero elapsed time, as this frame's time has already been granted
        actor_id = m_clock.advance(events, actor_controlled, Clock::duration_type::zero());
    }
}
