//------------------------------------------------------------------------------
// stratum_sim_test
// scheduling.cpp
//------------------------------------------------------------------------------

#include "test_support.hpp"
#include "quetzal/stratum_sim/Command.hpp"
#include "quetzal/stratum_sim/CommandQueue.hpp"
#include "quetzal/stratum_sim/Event.hpp"
#include "quetzal/stratum_sim/EventQueue.hpp"
#include "quetzal/stratum_sim/Intent.hpp"
#include "quetzal/stratum_sim/ContinuousLayout.hpp"
#include "quetzal/stratum_sim/DiscreteLayout.hpp"
#include "quetzal/stratum_sim/DiscretePlacementIndex.hpp"
#include "quetzal/stratum_sim/commands.hpp"
#include "quetzal/stratum_sim/Clock.hpp"
#include "quetzal/stratum_sim/Scheduler.hpp"
#include "quetzal/stratum_sim/Session.hpp"
#include "quetzal/stratum_sim/Game.hpp"
#include "quetzal/stratum_sim/attachments.hpp"
#include "quetzal/stratum_sim/World.hpp"
#include "quetzal/testing/named_checks.hpp"
#include <doctest/doctest.h>
#include <algorithm>
#include <set>
#include <string>

using namespace std;
using namespace quetzal;
using namespace stratum_sim_test;
using quetzal::stratum_sim::CardinalDirection;
using quetzal::stratum_sim::cardinal_direction;

namespace
{

    TEST_SUITE_BEGIN("scheduling");

    // Substitute command types, sim defines no commands of its own
    //--------------------------------------------------------------------------
    struct Move
    {
        int dx = 0;
        int dy = 0;
    };

    //--------------------------------------------------------------------------
    struct Wait
    {
    };

    // Substitute event types, sim defines no events of its own
    //--------------------------------------------------------------------------
    struct EntityMoved
    {
        id_type entity_id = nullid;
        int x = 0;
        int y = 0;
    };

    //--------------------------------------------------------------------------
    struct DoorOpened
    {
        id_type entity_id = nullid;
    };

    //--------------------------------------------------------------------------
    TEST_CASE("command queue")
    {
        stratum_sim::CommandQueue queue;

        CHECK_CASE("new queue is empty", queue.empty() && queue.size() == 0);

        queue.push(7, Move {1, 0});
        queue.push(7, Wait {});
        queue.push(9, Move {0, -1});

        CHECK_CASE_EQUAL("push grows the queue", queue.size(), 3);
        CHECK_CASE("a populated queue is not empty", !queue.empty());

        const stratum_sim::Command& command = queue.front();
        CHECK_CASE("front is the first command submitted", command.is<Move>());
        CHECK_CASE_EQUAL("a command names its entity id", command.entity_id(), 7);
        CHECK_CASE("get returns the command", command.get<Move>().dx == 1 && command.get<Move>().dy == 0);
        CHECK_CASE("try_get is null for another command type", command.try_get<Wait>() == nullptr);

        queue.pop();
        CHECK_CASE("pop removes the front command", queue.front().is<Wait>());
        CHECK_CASE_EQUAL("pop shrinks the queue", queue.size(), 2);

        queue.pop();
        CHECK_CASE_EQUAL("commands are consumed in submission order", queue.front().entity_id(), 9);

        stratum_sim::Command command_prebuilt(4, Move {0, 1});
        queue.push(command_prebuilt);
        CHECK_CASE_EQUAL("a prebuilt command can be pushed", queue.size(), 2);
        CHECK_CASE("a prebuilt command keeps its type", queue.commands().back().is<Move>());
        CHECK_CASE_EQUAL("a prebuilt command keeps its entity id", queue.commands().back().entity_id(), 4);

        stratum_sim::Command command_default;
        CHECK_CASE("a default command is empty", command_default.empty() && command_default.entity_id() == nullid);

        queue.clear();
        CHECK_CASE("clear empties the queue", queue.empty());

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("event queue")
    {
        stratum_sim::EventQueue queue;

        CHECK_CASE("new queue is empty", queue.empty() && queue.size() == 0);

        queue.push(EntityMoved {7, 3, 4});
        queue.push(DoorOpened {9});

        CHECK_CASE_EQUAL("push grows the queue", queue.size(), 2);

        const stratum_sim::Event& event = queue.front();
        CHECK_CASE("front is the first event pushed", event.is<EntityMoved>());
        CHECK_CASE("get returns the event", event.get<EntityMoved>().x == 3 && event.get<EntityMoved>().y == 4);
        CHECK_CASE_EQUAL("an event payload has its own fields", event.get<EntityMoved>().entity_id, 7);
        CHECK_CASE("try_get is null for another event type", event.try_get<DoorOpened>() == nullptr);

        queue.pop();
        CHECK_CASE("pop removes the front event", queue.front().is<DoorOpened>());
        CHECK_CASE_EQUAL("pop shrinks the queue", queue.size(), 1);

        queue.push(EntityMoved {9, 0, 0});
        CHECK_CASE("events iterate in occurrence order", queue.events().front().is<DoorOpened>() && queue.events().back().is<EntityMoved>());

        // Guards the constrained converting constructor, an unconstrained one would win
        // over the copy constructor here and produce an Event containing an Event
        stratum_sim::Event event_source(DoorOpened {5});
        stratum_sim::Event event_copy(event_source);
        CHECK_CASE("copying an event does not nest it", event_copy.is<DoorOpened>());
        CHECK_CASE_EQUAL("a copied event keeps its payload", event_copy.get<DoorOpened>().entity_id, 5);

        stratum_sim::Event event_default;
        CHECK_CASE("a default event is empty", event_default.empty());

        queue.clear();
        CHECK_CASE("clear empties the queue", queue.empty());

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("scheduler")
    {
        stratum_sim::Scheduler scheduler;

        CHECK_CASE_EQUAL("a new scheduler has no actors", scheduler.count(), 0);
        CHECK_CASE("with nothing to schedule there is no next actor", scheduler.next() == nullid);

        scheduler.add(1, 100);
        CHECK_CASE("an added actor is present", scheduler.contains(1));
        CHECK_CASE_EQUAL("count follows", scheduler.count(), 1);
        CHECK_CASE_EQUAL("it starts with no energy", scheduler.energy(1), 0);

        CHECK_CASE("a lone actor is next", scheduler.next() == 1);
        CHECK_CASE_EQUAL("it waited one unit for a full action", scheduler.elapsed(), 1);

        scheduler.spend(1);
        CHECK_CASE_EQUAL("spending leaves it empty", scheduler.energy(1), 0);

        // Twice the speed acts twice as often
        scheduler.clear();
        scheduler.add(1, 100);
        scheduler.add(2, 200);

        size_t acts_1 = 0;
        size_t acts_2 = 0;
        for (size_t n = 0; n < 30; ++n)
        {
            id_type actor = scheduler.next();
            acts_1 += actor == 1 ? 1 : 0;
            acts_2 += actor == 2 ? 1 : 0;
            scheduler.spend(actor);
        }

        CHECK_CASE_EQUAL("the faster actor acts twice as often", acts_2, acts_1 * 2);
        CHECK_CASE_EQUAL("and every turn went to one of them", acts_1 + acts_2, 30);

        // Equal speeds alternate, lowest id first
        scheduler.clear();
        scheduler.add(7, 100);
        scheduler.add(4, 100);

        id_type first = scheduler.next();
        scheduler.spend(first);
        id_type second = scheduler.next();
        scheduler.spend(second);

        CHECK_CASE("equal actors break ties by id", first == 4 && second == 7);

        // A costlier action delays the next turn
        scheduler.clear();
        scheduler.add(1, 100);
        scheduler.add(2, 100);

        id_type slow = scheduler.next();
        scheduler.spend(slow, stratum_sim::Scheduler::action_cost * 2);
        CHECK_CASE("a double cost action yields the next two turns", scheduler.next() != slow);

        // Speed zero never accumulates
        scheduler.clear();
        scheduler.add(1, 0);
        CHECK_CASE("an actor with no speed never acts", scheduler.next() == nullid);
        CHECK_CASE("and time does not advance for it", scheduler.elapsed() == 0);

        scheduler.set_speed(1, 50);
        CHECK_CASE("setting its speed lets it act", scheduler.next() == 1);
        CHECK_CASE_EQUAL("at half speed it waits two units", scheduler.elapsed(), 2);

        scheduler.remove(1);
        CHECK_CASE("a removed actor is gone", !scheduler.contains(1) && scheduler.count() == 0);

        // advance grants time without skipping ahead, so no actor need be ready
        scheduler.clear();
        scheduler.add(1, 100);
        scheduler.add(2, 25);

        CHECK_CASE("nobody is ready before any time is granted", scheduler.ready() == nullid);

        scheduler.advance(1);
        CHECK_CASE_EQUAL("granting a unit advances elapsed", scheduler.elapsed(), 1);
        CHECK_CASE("the fast actor is ready", scheduler.ready() == 1);
        CHECK_CASE_EQUAL("and the slow one has only part of a turn", scheduler.energy(2), 25);

        scheduler.spend(1);
        CHECK_CASE("after spending, nobody is ready", scheduler.ready() == nullid);

        scheduler.advance(3);
        CHECK_CASE_EQUAL("the slow actor reaches a full turn in its own time", scheduler.energy(2), 100);
        CHECK_CASE("but the fast one, having banked three, is readier", scheduler.ready() == 1);
        CHECK_CASE_EQUAL("with elapsed counting every granted unit", scheduler.elapsed(), 4);

        // next arrives where granting one unit at a time would
        scheduler.clear();
        scheduler.add(1, 25);

        stratum_sim::Scheduler granted;
        granted.add(1, 25);
        for (int i = 0; i < 4; ++i)
        {
            granted.advance(1);
        }

        CHECK_CASE("next agrees with granting a unit at a time",
            scheduler.next() == granted.ready() && scheduler.elapsed() == granted.elapsed());

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    TEST_CASE_FIXTURE(WorldFixture, "clock")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        TestWorld world(id_allocator);
        flecs::entity zone = load_zone_unpopulated(world, "data/corridor.zone");

        stratum_sim::DiscreteLayout& layout = zone_layout(world, zone);
        flecs::entity character = world.create_entity(zone,
            stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {1, 1}),
                stratum_sim::vector_type(1.0f, 0.0f, 0.0f)));

        stratum_sim::Scheduler scheduler;
        scheduler.add(character.id(), 100);

        stratum_sim::Clock clock(world, scheduler);
        stratum_sim::EventQueue events;

        id_type controlled = static_cast<id_type>(character.id());

        auto intend = [&](stratum_sim::value_type dx, stratum_sim::value_type dy)
        {
            character.set<stratum_sim::Intent>(stratum_sim::Intent {
                stratum_sim::vector_type(dx, dy, 0.0f), 1.0f, stratum_sim::vector_type()});
        };

        CHECK_CASE("with no time granted nobody runs", clock.run(events, controlled) == nullid);

        clock.grant_turn();
        CHECK_CASE("granting a turn makes an actor ready", scheduler.ready() == controlled);

        // With no Intent, the controlled actor is returned
        id_type waiting = clock.run(events, controlled);
        CHECK_CASE("the controlled actor asking for nothing is returned", waiting == controlled);
        CHECK_CASE_EQUAL("and nothing has happened", events.size(), 0);

        // An intent runs and charges the turn
        intend(1.0f, 0.0f);
        id_type after = clock.run(events, controlled);
        CHECK_CASE("running the only actor leaves nobody ready", after == nullid);
        CHECK_CASE("and it moved", layout.cell_position_at(stratum_sim::placement_of(character).position()) == stratum_sim::discrete_point_type {2, 1});
        CHECK_CASE_EQUAL("its turn was charged", scheduler.energy(character.id()), 0);
        CHECK_CASE("the intent was spent", character.get<stratum_sim::Intent>().direction.x() == 0.0f);

        // An intent lasts one action, so a second turn on the same one does nothing
        clock.grant_turn();
        CHECK_CASE("a spent intent is not acted on again", clock.run(events, controlled) == controlled);
        CHECK_CASE("and it did not move", layout.cell_position_at(stratum_sim::placement_of(character).position()) == stratum_sim::discrete_point_type {2, 1});

        // Facing is taken before movement, so the two are one action each
        events.clear();
        character.set<stratum_sim::Intent>(stratum_sim::Intent {
            stratum_sim::vector_type(1.0f, 0.0f, 0.0f), 1.0f, stratum_sim::vector_type(0.0f, 1.0f, 0.0f)});

        clock.run(events, controlled);
        CHECK_CASE("facing is taken first", stratum_sim::placement_of(character).direction().y() == 1.0f);
        CHECK_CASE("and the movement asked for is still owed", character.get<stratum_sim::Intent>().direction.x() == 1.0f);
        CHECK_CASE("which remains until a turn is granted for it",
            layout.cell_position_at(stratum_sim::placement_of(character).position()) == stratum_sim::discrete_point_type {2, 1});

        clock.grant_turn();
        clock.run(events, controlled);
        CHECK_CASE("the movement follows on the next turn", layout.cell_position_at(stratum_sim::placement_of(character).position()) == stratum_sim::discrete_point_type {3, 1});

        // An action that achieves nothing costs no turn, so walking into a wall is free
        clock.grant_turn();
        while (layout.cell_position_at(stratum_sim::placement_of(character).position()).i() > 1)
        {
            intend(-1.0f, 0.0f);
            clock.run(events, controlled);
            clock.grant_turn();
        }

        long long elapsed_before = scheduler.elapsed();
        intend(-1.0f, 0.0f);
        id_type balked = clock.run(events, controlled);
        CHECK_CASE("an actor that achieved nothing is returned", balked == controlled);
        CHECK_CASE("its turn was not charged", scheduler.energy(character.id()) >= stratum_sim::Scheduler::action_cost);
        CHECK_CASE_EQUAL("and no time passed", scheduler.elapsed(), static_cast<size_t>(elapsed_before));

        // Any other actor with no Intent waits. A world of its own, so the energies above do not decide the order
        {
            TestWorld world_two(id_allocator);
            flecs::entity zone_two = load_zone_unpopulated(world_two, "data/corridor.zone");

            const stratum_sim::DiscreteLayout& layout_two = zone_layout(world_two, zone_two);
            stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);

            flecs::entity driven = world_two.create_entity(zone_two,
                stratum_sim::placement_type(layout_two.position(stratum_sim::discrete_point_type {1, 1}), east));
            flecs::entity other = world_two.create_entity(zone_two,
                stratum_sim::placement_type(layout_two.position(stratum_sim::discrete_point_type {3, 1}), east));

            stratum_sim::Scheduler scheduler_two;
            scheduler_two.add(driven.id(), 100);
            scheduler_two.add(other.id(), 200); // Faster, so it is reached first

            stratum_sim::Clock clock_two(world_two, scheduler_two);
            stratum_sim::EventQueue events_two;

            scheduler_two.advance(1);
            driven.set<stratum_sim::Intent>(stratum_sim::Intent {east, 1.0f, stratum_sim::vector_type()});

            id_type ran = clock_two.run(events_two, static_cast<id_type>(driven.id()));

            CHECK_CASE("every ready actor ran", ran == nullid);
            CHECK_CASE("the one asking for nothing waited", scheduler_two.energy(other.id()) < stratum_sim::Scheduler::action_cost);
            CHECK_CASE("and the controlled actor acted even though it was the less ready",
                layout_two.cell_position_at(stratum_sim::placement_of(driven).position()) == stratum_sim::discrete_point_type {2, 1});
        }

        return;
    }

    //--------------------------------------------------------------------------
    // The clock's driver
    TEST_CASE_FIXTURE(WorldFixture, "time driver")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        TestWorld world(id_allocator);
        flecs::entity zone = load_zone_unpopulated(world, "data/corridor.zone");

        stratum_sim::DiscreteLayout& layout = zone_layout(world, zone);
        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);

        flecs::entity character = world.create_entity(zone,
            stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {1, 1}), east));

        stratum_sim::Scheduler scheduler;
        scheduler.add(character.id(), 100);

        stratum_sim::Clock clock(world, scheduler);
        stratum_sim::EventQueue events;
        id_type controlled = static_cast<id_type>(character.id());

        auto restart = [&](double units_per_second)
        {
            scheduler.clear();
            scheduler.add(character.id(), 100);
            clock.set_driver(stratum_sim::Driver::RealTime, units_per_second);
        };

        CHECK_CASE("a clock is turn-based until told otherwise", clock.driver() == stratum_sim::Driver::TurnBased);

        // Turn-based skips ahead, so it needs no elapsed time to make an actor ready
        CHECK_CASE("turn-based skips ahead to the next actor", clock.advance(events, controlled) == controlled);
        CHECK_CASE_EQUAL("and passed the time it took to get there", scheduler.elapsed(), 1);

        // Two units a second, so an actor of speed 100 gets two actions a second
        restart(2.0);
        CHECK_CASE("the driver is what it was set to", clock.driver() == stratum_sim::Driver::RealTime);
        CHECK_CASE("and keeps the rate with it", clock.units_per_second() == 2.0);

        clock.advance(events, controlled, stratum_sim::Clock::duration_type(0.5));
        CHECK_CASE_EQUAL("half a second at two units a second is one unit", scheduler.elapsed(), 1);
        CHECK_CASE_EQUAL("which is one action at that speed", scheduler.energy(character.id()), stratum_sim::Scheduler::action_cost);

        // The fraction is kept, so short frames add up
        restart(2.0);
        clock.advance(events, controlled, stratum_sim::Clock::duration_type(0.125));
        CHECK_CASE_EQUAL("a frame too short to earn a unit grants none", scheduler.elapsed(), 0);

        for (int n = 0; n < 3; ++n)
        {
            clock.advance(events, controlled, stratum_sim::Clock::duration_type(0.125));
        }

        CHECK_CASE_EQUAL("but the remainder carries, so four of them earn one", scheduler.elapsed(), 1);

        // Real time does not skip ahead
        restart(2.0);
        CHECK_CASE("real time does not skip ahead to make an actor ready",
            clock.advance(events, controlled, stratum_sim::Clock::duration_type::zero()) == nullid);
        CHECK_CASE_EQUAL("so no time passed", scheduler.elapsed(), 0);

        // Once elapsed time has made it ready, it acts on its intent
        character.set<stratum_sim::Intent>(stratum_sim::Intent {east, 1.0f, stratum_sim::vector_type()});
        clock.advance(events, controlled, stratum_sim::Clock::duration_type(0.5));
        CHECK_CASE("an actor made ready by elapsed time acts",
            layout.cell_position_at(stratum_sim::placement_of(character).position()) == stratum_sim::discrete_point_type(2, 1));

        auto place = [&](const stratum_sim::discrete_point_type& cell, const stratum_sim::vector_type& direction)
        {
            stratum_sim::set_placement(character, stratum_sim::placement_type(layout.position(cell), east));
            character.set<stratum_sim::Intent>(stratum_sim::Intent {direction, 1.0f, stratum_sim::vector_type()});
        };

        // A held direction keeps the actor moving
        restart(2.0);
        place(stratum_sim::discrete_point_type {1, 1}, east);

        for (int n = 0; n < 3; ++n)
        {
            clock.advance(events, controlled, stratum_sim::Clock::duration_type(0.5));
        }

        CHECK_CASE("a direction held keeps stepping as the time is earned",
            layout.cell_position_at(stratum_sim::placement_of(character).position()) == stratum_sim::discrete_point_type(4, 1));
        CHECK_CASE("and is still being asked for", character.get<stratum_sim::Intent>().direction.x() == 1.0f);

        // Turn-based clears the Intent after one move
        scheduler.clear();
        scheduler.add(character.id(), 100);
        clock.set_driver(stratum_sim::Driver::TurnBased);
        place(stratum_sim::discrete_point_type {1, 1}, east);

        for (int n = 0; n < 3; ++n)
        {
            clock.advance(events, controlled);
        }

        CHECK_CASE("turn-based takes one step from one intent",
            layout.cell_position_at(stratum_sim::placement_of(character).position()) == stratum_sim::discrete_point_type(2, 1));

        // Against a wall the Intent is cleared; the case is that this returns at all
        restart(2.0);
        place(stratum_sim::discrete_point_type {1, 1}, stratum_sim::vector_type(-1.0f, 0.0f, 0.0f));
        clock.advance(events, controlled, stratum_sim::Clock::duration_type(0.5));

        CHECK_CASE("a mover held against a wall does not move",
            layout.cell_position_at(stratum_sim::placement_of(character).position()) == stratum_sim::discrete_point_type(1, 1));
        CHECK_CASE("and stops asking for what it could not have",
            character.get<stratum_sim::Intent>().direction.x() == 0.0f);

        // A host calls advance the same way under either driver
        stratum_sim::Game game_turn(g_test_config, g_test_manifest);
        stratum_sim::Session& session_turn = game_turn.session();
        enter_zone_file(game_turn, "data/corridor.zone");

        stratum_sim::Game game_real(g_test_config, g_test_manifest);
        stratum_sim::Session& session_real = game_real.session();
        session_real.set_driver(stratum_sim::Driver::RealTime);
        enter_zone_file(game_real, "data/corridor.zone");

        stratum_sim::EventQueue session_events;
        CHECK_CASE("a turn-based session hands control out with no elapsed time given",
            session_turn.advance(session_events) == static_cast<id_type>(session_turn.character().entity().id()));
        CHECK_CASE("a real-time one does not, having been given none",
            session_real.advance(session_events) == nullid);

        // The character acts through its own intent and commands, the door east of the cell below the staircase it enters at
        stratum_sim::Game game_door(g_test_config, g_test_manifest);
        stratum_sim::Session& session_door = game_door.session();
        enter_zone_file(game_door, "data/door.zone");
        session_door.advance(session_events);

        stratum_sim::Character& character_door = session_door.character();
        game_door.world().relocate(character_door.entity(), session_door.zone(), stratum_sim::placement_type(
            stratum_sim::layout_as<stratum_sim::DiscreteLayout>(session_door.layout()).position(stratum_sim::discrete_point_type {1, 1}),
            stratum_sim::vector_type(0.0f, -1.0f, 0.0f)));
        character_door.set_intent(stratum_sim::Intent {stratum_sim::vector_type(), 1.0f, stratum_sim::vector_type(1.0f, 0.0f, 0.0f)});
        session_door.advance(session_events);
        CHECK_CASE("the character takes the intent set on it", stratum_sim::placement_of(character_door.entity()).direction().x() == 1.0f);

        character_door.submit(stratum_sim::Interact {});
        CHECK_CASE("a command the character submits waits on it", character_door.entity().has<stratum_sim::PendingCommand>());

        session_door.advance(session_events);
        flecs::entity door = stratum_sim::access_entity(stratum_sim::way_through(session_door.zone(),
            stratum_sim::layout_as<stratum_sim::DiscreteLayout>(session_door.layout()).wall_toward(stratum_sim::discrete_point_type {1, 1},
                cardinal_direction(CardinalDirection::East))));
        CHECK_CASE("and is taken the next time the character is ready", stratum_sim::access(door) == stratum_sim::AccessState::Open
            && !character_door.entity().has<stratum_sim::PendingCommand>());

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE_FIXTURE(WorldFixture, "commands")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        TestWorld world(id_allocator);
        flecs::entity zone = load_zone_unpopulated(world, "data/room.zone");

        stratum_sim::DiscreteLayout& layout = zone_layout(world, zone);
        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        stratum_sim::vector_type north(0.0f, 1.0f, 0.0f);

        flecs::entity actor = world.create_entity(zone, stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {1, 2}), east));

        stratum_sim::CommandQueue commands;
        stratum_sim::EventQueue events;

        auto cell_of = [&]() { return layout.cell_position_at(stratum_sim::placement_of(actor).position()); };

        // A move in the actor's facing direction
        commands.push(actor.id(), stratum_sim::Step {stratum_sim::vector_type(0.0f, 0.0f, 0.0f), 1.0f});
        stratum_sim::apply(world, commands, events);

        CHECK_CASE("a step moves the actor", cell_of() == stratum_sim::discrete_point_type(2, 2));
        CHECK_CASE_EQUAL("and reports one event", events.size(), 1);
        CHECK_CASE("which is a move", events.front().is<stratum_sim::Moved>());
        CHECK_CASE("naming the actor", events.front().get<stratum_sim::Moved>().entity_id == actor.id());
        CHECK_CASE("commands are consumed", commands.empty());

        events.clear();

        // Walking into a wall reports what stopped it and does not move
        commands.push(actor.id(), stratum_sim::Step {north, 5.0f});
        stratum_sim::apply(world, commands, events);

        CHECK_CASE("a blocked step does not move", cell_of() == stratum_sim::discrete_point_type(2, 3));
        CHECK_CASE("it moved as far as it could", events.front().is<stratum_sim::Moved>());
        CHECK_CASE("and reported what stopped it", events.events().back().is<stratum_sim::Blocked>());
        CHECK_CASE("what stopped it is a wall of the layout", events.events().back().get<stratum_sim::Blocked>().blocking_wall_id != nullid);

        events.clear();

        // Held against a wall, nothing happens and nothing is spent
        stratum_sim::Command held(actor.id(), stratum_sim::Step {north, 1.0f});
        CHECK_CASE_EQUAL("a step that cannot move costs nothing", static_cast<size_t>(stratum_sim::apply(world, held, events)), 0);
        CHECK_CASE("and reports only the block", events.size() == 1 && events.front().is<stratum_sim::Blocked>());

        events.clear();

        // A move costs in proportion to the distance traveled
        auto place = [&](const stratum_sim::discrete_point_type& cell, const stratum_sim::vector_type& direction)
        {
            stratum_sim::set_placement(actor, stratum_sim::placement_type(layout.position(cell), direction));
        };

        place(stratum_sim::discrete_point_type {1, 1}, east);
        stratum_sim::Command one(actor.id(), stratum_sim::Step {east, 1.0f});
        CHECK_CASE_EQUAL("a one cell step costs one action",
            static_cast<size_t>(stratum_sim::apply(world, one, events)), static_cast<size_t>(stratum_sim::Scheduler::action_cost));

        events.clear();

        place(stratum_sim::discrete_point_type {1, 1}, east);
        stratum_sim::Command two(actor.id(), stratum_sim::Step {east, 2.0f});
        CHECK_CASE_EQUAL("a two cell step costs two",
            static_cast<size_t>(stratum_sim::apply(world, two, events)), static_cast<size_t>(2 * stratum_sim::Scheduler::action_cost));
        CHECK_CASE("and crossed them in one command", cell_of() == stratum_sim::discrete_point_type(3, 1));

        events.clear();

        // Stopped short, it is charged for the distance traveled
        place(stratum_sim::discrete_point_type {1, 1}, east);
        stratum_sim::Command overreach(actor.id(), stratum_sim::Step {east, 4.0f});
        int cost_short = stratum_sim::apply(world, overreach, events);

        CHECK_CASE("a step stopped short costs less than the full distance",
            cost_short > 0 && cost_short < 4 * stratum_sim::Scheduler::action_cost);
        CHECK_CASE("and stopped where the wall is", cell_of() == stratum_sim::discrete_point_type(3, 1));
        CHECK_CASE_EQUAL("charged an action per cell it did reach",
            static_cast<size_t>(cost_short), static_cast<size_t>(2 * stratum_sim::Scheduler::action_cost));

        events.clear();
        place(stratum_sim::discrete_point_type {2, 3}, east);

        // Turning
        commands.push(actor.id(), stratum_sim::TurnTo {
            layout.turn(stratum_sim::placement_of(actor), stratum_sim::Rotation::CW).direction()});
        stratum_sim::apply(world, commands, events);

        CHECK_CASE("turning right from east faces south", stratum_sim::placement_of(actor).direction().y() == -1.0f);
        CHECK_CASE("and reports a turn", events.front().is<stratum_sim::Turned>());
        CHECK_CASE("turning does not move", cell_of() == stratum_sim::discrete_point_type(2, 3));

        events.clear();

        // Interact with the wall or door the actor faces
        stratum_sim::Command interact_floor(actor.id(), stratum_sim::Interact {});
        CHECK_CASE_EQUAL("interacting with open floor does nothing", static_cast<size_t>(stratum_sim::apply(world, interact_floor, events)), 0);
        CHECK_CASE("and reports nothing", events.empty());

        // Face the doorway and interact
        stratum_sim::set_placement(world.entities().entity(actor.id()),
            stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {3, 2}), east));
        stratum_sim::Command interact_door(actor.id(), stratum_sim::Interact {});

        CHECK_CASE("interacting with a wall costs an action", stratum_sim::apply(world, interact_door, events) == stratum_sim::Scheduler::action_cost);
        CHECK_CASE("and names the wall", events.front().is<stratum_sim::Interacted>() && events.front().get<stratum_sim::Interacted>().wall_id != nullid);

        events.clear();

        // A command for an entity that no longer exists is ignored
        stratum_sim::Command orphan(9999, stratum_sim::Step {east, 1.0f});
        CHECK_CASE_EQUAL("a command for a dead entity costs nothing", static_cast<size_t>(stratum_sim::apply(world, orphan, events)), 0);
        CHECK_CASE("and produces no events", events.empty());

        return;
    }

    TEST_SUITE_END();

} // namespace
