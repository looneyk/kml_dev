//------------------------------------------------------------------------------
// stratum_sim
// Session.cpp
//------------------------------------------------------------------------------

#include "Session.hpp"
#include "ai.hpp"
#include "attachments.hpp"
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
    std::optional<quetzal::stratum_sim::discrete_point_type> first_open_cell(const quetzal::stratum_sim::DiscreteLayout& layout)
    {
        const quetzal::stratum_sim::GridMapping& grid = layout.grid();

        for (long j = 0; j < grid.extent.y(); ++j)
        {
            for (long i = 0; i < grid.extent.x(); ++i)
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
    // The highest open cell, as far from the controlled entity as this cares to look
    std::optional<quetzal::stratum_sim::discrete_point_type> last_open_cell(const quetzal::stratum_sim::DiscreteLayout& layout)
    {
        const quetzal::stratum_sim::GridMapping& grid = layout.grid();

        for (long j = grid.extent.y(); j-- > 0; )
        {
            for (long i = grid.extent.x(); i-- > 0; )
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
    // The controlled entity, at the first open cell. An empty entity where the layout has no
    // open cell at all, which is the caller's to resolve rather than something to decide here
    // Where a zone's starting contents come from is what this substitutes for: a level specifies
    // its contents and a placement pass puts them there. Until one does, this is the whole of
    // what a loaded zone contains
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

        player.add<Blocking>(); // Obstructs anything else that moves
        scheduler.add(player.id(), player_speed);

        return player;
    }

    //--------------------------------------------------------------------------
    // What else goes in the zone. One pursuer as far off as this bothers to look, so that
    // something other than the host decides for itself
    // This is where monster selection and placement belong: which monsters a zone gets and
    // where they are placed, drawn against the zone rather than fixed as they are here
    void create_monsters(quetzal::stratum_sim::World& world, flecs::entity zone,
        const quetzal::stratum_sim::DiscreteLayout& layout, quetzal::stratum_sim::Scheduler& scheduler,
        flecs::entity player)
    {
        using namespace quetzal::stratum_sim;

        std::optional<discrete_point_type> start = last_open_cell(layout);

        if (!start || layout.grid_position_at(player.get<placement_type>().position()) == *start)
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
    // A generator reports the walls a door is attached to rather than placing one, having no world
    // to place it in, which is the same seam the loader reports across
    door_walls_type doors;

    std::unique_ptr<Layout> layout = generator.generate(
        [&](const std::vector<id_type>& walls) { doors.push_back(walls); });

    populate(std::move(layout), doors, generator.format(), driver, units_per_second);
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Session::populate(std::unique_ptr<Layout> layout, const door_walls_type& doors,
    Format format, Driver driver, double units_per_second)
{
    // Built alongside and moved in only once the whole of it has succeeded. The clock's
    // references are to these members rather than to what is moved from, so they remain valid
    World world;

    flecs::entity zone = world.create_zone("zone", std::move(layout));

    // A door is attached to the walls it was told, and nothing may be crossed until they are built
    create_doors(world, zone, doors);

    // Where a zone's contents go is asked in cells, which is the layout's own vocabulary
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
    m_format = format;
    m_clock.set_driver(driver, units_per_second);

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Session::load(const std::filesystem::path& filepath, Driver driver, double units_per_second)
{
    // The loader reports the walls a door is attached to rather than placing one, having no world
    // to place it in
    door_walls_type doors;
    Format format = Format::Block;

    std::unique_ptr<DiscreteLayout> layout = load_layout(filepath, value_type(1),
        [&](const std::vector<id_type>& walls) { doors.push_back(walls); }, &format);

    try
    {
        populate(std::move(layout), doors, format, driver, units_per_second);
    }
    catch (const std::runtime_error&)
    {
        // Reported here, where there is a file to report; populate has only the zone it was handed
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
quetzal::stratum_sim::Format quetzal::stratum_sim::Session::format() const
{
    return m_format;
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

        // The controlled entity is up asking for nothing. A one-shot action is the other form
        // a host can have asked for, an Intent limited to movement and facing
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
