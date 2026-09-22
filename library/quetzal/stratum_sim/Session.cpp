//------------------------------------------------------------------------------
// stratum_sim
// Session.cpp
//------------------------------------------------------------------------------

#include "Session.hpp"
#include "ai.hpp"
#include "attachments.hpp"
#include "load_layout.hpp"
#include "Site.hpp"
#include <format>
#include <optional>
#include <stdexcept>
#include <cassert>

namespace
{

    static constexpr int player_speed = 100;

    //--------------------------------------------------------------------------
    std::optional<quetzal::stratum_sim::discrete_point_type> first_open_cell(const quetzal::stratum_sim::DiscreteLayout& layout)
    {
        const quetzal::stratum_sim::GridMapping& grid = layout.grid();

        for (quetzal::stratum_sim::coord_type j = 0; j < grid.extent.y(); ++j)
        {
            for (quetzal::stratum_sim::coord_type i = 0; i < grid.extent.x(); ++i)
            {
                if (layout.open(quetzal::stratum_sim::discrete_point_type {i, j}))
                {
                    return quetzal::stratum_sim::discrete_point_type {i, j};
                }
            }
        }

        return {};
    }

    //--------------------------------------------------------------------------
    std::optional<quetzal::stratum_sim::discrete_point_type> last_open_cell(const quetzal::stratum_sim::DiscreteLayout& layout)
    {
        const quetzal::stratum_sim::GridMapping& grid = layout.grid();

        for (quetzal::stratum_sim::coord_type j = grid.extent.y(); j-- > 0; )
        {
            for (quetzal::stratum_sim::coord_type i = grid.extent.x(); i-- > 0; )
            {
                if (layout.open(quetzal::stratum_sim::discrete_point_type {i, j}))
                {
                    return quetzal::stratum_sim::discrete_point_type {i, j};
                }
            }
        }

        return {};
    }

    //--------------------------------------------------------------------------
    // The player, at the first open cell. An empty entity when the layout has no open cell
    flecs::entity create_player(quetzal::stratum_sim::World& world, flecs::entity zone,
        const quetzal::stratum_sim::DiscreteLayout& layout, quetzal::stratum_sim::Scheduler& scheduler)
    {
        using namespace quetzal::stratum_sim;

        std::optional<discrete_point_type> start = first_open_cell(layout);

        if (!start)
        {
            return {};
        }

        flecs::entity player = world.create_entity(zone,
            placement_type(layout.position(*start), vector_type(value_type(1), value_type(0), value_type(0))));

        player.add<Blocking>();
        scheduler.add(player.id(), player_speed);

        return player;
    }

    //--------------------------------------------------------------------------
    // One pursuer at the last open cell
    void create_monsters(quetzal::stratum_sim::World& world, flecs::entity zone,
        const quetzal::stratum_sim::DiscreteLayout& layout, quetzal::stratum_sim::Scheduler& scheduler,
        flecs::entity player)
    {
        using namespace quetzal::stratum_sim;

        std::optional<discrete_point_type> start = last_open_cell(layout);

        if (!start || layout.cell_position_at(placement_of(player).position()) == *start)
        {
            return;
        }

        flecs::entity other = world.create_entity(zone,
            placement_type(layout.position(*start), vector_type(value_type(-1), value_type(0), value_type(0))));

        other.add<Blocking>();
        other.set<Pursue>(Pursue {static_cast<quetzal::id_type>(player.id()), point_type(), false});
        scheduler.add(other.id(), player_speed);

        return;
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
void quetzal::stratum_sim::Session::create(const Generator& generator, Driver driver, double units_per_second)
{
    // A generator has no world to place a door in, so it reports the walls each door is attached to
    door_walls_type doors;

    std::unique_ptr<Layout> layout = generator.generate(
        [&](const std::vector<id_type>& walls) { doors.push_back(walls); });

    populate(std::move(layout), doors, generator.notation(), driver, units_per_second);
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Session::populate(std::unique_ptr<Layout> layout, const door_walls_type& doors,
    stratum_format::Notation notation, Driver driver, double units_per_second)
{
    // Built aside and moved in only on success. The clock refers to the members, which stay valid across the move
    World world;

    flecs::entity zone = world.create_zone("zone", std::move(layout));

    create_doors(world, zone, doors);

    const DiscreteLayout& discrete = layout_as<DiscreteLayout>(world.layout(zone));

    Scheduler scheduler;
    flecs::entity player = create_player(world, zone, discrete, scheduler);

    if (!player)
    {
        throw std::runtime_error("zone has no open cell");
    }

    create_monsters(world, zone, discrete, scheduler, player);

    m_world = std::move(world);
    m_scheduler = std::move(scheduler);
    m_command_pending = Command();
    m_zone = zone;
    m_player = player;
    m_notation = notation;
    m_clock.set_driver(driver, units_per_second);

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Session::load(const std::filesystem::path& filepath, Driver driver, double units_per_second)
{
    door_walls_type doors;
    stratum_format::Notation notation = stratum_format::Notation::Block;

    std::unique_ptr<DiscreteLayout> layout = load_layout(filepath,
        [&](const std::vector<id_type>& walls) { doors.push_back(walls); }, &notation);

    try
    {
        populate(std::move(layout), doors, notation, driver, units_per_second);
    }
    catch (const std::runtime_error&)
    {
        throw std::runtime_error(std::format("level has no open cell: {}", filepath.string()));
    }

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
    return m_notation;
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
