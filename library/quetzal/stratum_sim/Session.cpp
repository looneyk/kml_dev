//------------------------------------------------------------------------------
// stratum_sim
// Session.cpp
//------------------------------------------------------------------------------

#include "Session.hpp"
#include "ai.hpp"
#include "load_layout.hpp"
#include <format>
#include <optional>
#include <stdexcept>
#include <cassert>

namespace
{

    static constexpr int player_speed = 100;

    //--------------------------------------------------------------------------
    // The lowest open cell, where the controlled entity starts. Empty when the layout has
    // no open cell at all
    std::optional<quetzal::stratum_sim::GridPosition> first_open_cell(const quetzal::stratum_sim::Layout& layout)
    {
        const quetzal::stratum_sim::Grid& grid = layout.grid();

        for (long j = 0; j < grid.extent.y(); ++j)
        {
            for (long i = 0; i < grid.extent.x(); ++i)
            {
                if (layout.open(quetzal::stratum_sim::GridPosition {i, j}))
                {
                    return quetzal::stratum_sim::GridPosition {i, j};
                }
            }
        }

        return {};
    }

    //--------------------------------------------------------------------------
    // The highest open cell, as far from the controlled entity as this cares to look
    std::optional<quetzal::stratum_sim::GridPosition> last_open_cell(const quetzal::stratum_sim::Layout& layout)
    {
        const quetzal::stratum_sim::Grid& grid = layout.grid();

        for (long j = grid.extent.y(); j-- > 0; )
        {
            for (long i = grid.extent.x(); i-- > 0; )
            {
                if (layout.open(quetzal::stratum_sim::GridPosition {i, j}))
                {
                    return quetzal::stratum_sim::GridPosition {i, j};
                }
            }
        }

        return {};
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_sim::Session::Session() :
    m_world(),
    m_scheduler(),
    m_clock(m_world, m_scheduler),
    m_command_pending(),
    m_zone(),
    m_player()
{
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Session::load(const std::filesystem::path& filepath, Driver driver, double units_per_second)
{
    // Built alongside and moved in only once the whole load has succeeded. The clock's
    // references are to these members rather than to what is moved from, so they stand
    World world;
    flecs::entity zone = world.create_zone("zone");
    load_layout(filepath, world.layout(zone));

    std::optional<GridPosition> start = first_open_cell(world.layout(zone));
    if (!start)
    {
        throw std::runtime_error(std::format("level has no open cell: {}", filepath.string()));
    }

    flecs::entity player = world.create_entity(zone,
        placement_type(world.layout(zone).cell_position(*start), vector_type(value_type(1), value_type(0), value_type(0))));
    player.add<Blocking>(); // Stands in the way of anything else that moves

    Scheduler scheduler;
    scheduler.add(player.id(), player_speed);

    // One pursuer, as far off as this bothers to look, so that something other than the host
    // decides for itself. Placed here until levels carry content of their own
    std::optional<GridPosition> start_other = last_open_cell(world.layout(zone));
    if (start_other && start_other != start)
    {
        flecs::entity other = world.create_entity(zone,
            placement_type(world.layout(zone).cell_position(*start_other), vector_type(value_type(-1), value_type(0), value_type(0))));

        other.add<Blocking>();
        other.set<Pursue>(Pursue {static_cast<id_type>(player.id()), point_type(), false});
        scheduler.add(other.id(), player_speed);
    }

    m_world = std::move(world);
    m_scheduler = std::move(scheduler);
    m_command_pending = Command();
    m_zone = zone;
    m_player = player;
    m_clock.set_driver(driver, units_per_second);

    return;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::Session::loaded() const
{
    return static_cast<bool>(m_zone);
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
    return m_player;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Session::set_intent(const Intent& intent)
{
    assert(loaded());
    m_player.set<Intent>(intent);

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
    id_type actor_controlled = static_cast<id_type>(m_player.id());

    // Everything but the controlled entity decides for itself, and does so before the clock
    // reads what any of them are asking for
    // One decision per call: an actor fast enough to act twice between them acts twice on the
    // one it made, which is where this hook moves if that ever matters
    think(m_world, actor_controlled);

    id_type actor_id = m_clock.advance(events, actor_controlled, elapsed);

    for (;;)
    {
        if (actor_id == nullid)
        {
            return nullid;
        }

        // The controlled entity is up asking for nothing. A one-shot action is the other thing
        // a host can have asked for, an Intent holding only movement and facing
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

        // No elapsed time this pass: what the step earned has already been given out, and a
        // real-time driver would otherwise be paid twice for one frame
        actor_id = m_clock.advance(events, actor_controlled, Clock::duration_type::zero());
    }
}
