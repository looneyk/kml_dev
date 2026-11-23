//------------------------------------------------------------------------------
// flecs_test
// main.cpp
//
// Checks that flecs supports the structural decisions in doc/sim_zone_model.md:
// Zones as entities with their contents as children, attachment and containment as
// relationships, migration between Zones without renumbering, unload as a subtree
// delete, per-Zone serialization, and deterministic iteration order.
//------------------------------------------------------------------------------

#include "quetzal/testing/named_checks.hpp"
#include <doctest/doctest.h>

// flecs 4.2.0's has_component returns false after an if constexpr that returns, which MSVC reports as C4702 whatever the
// external warning level, since the warning comes from code generation
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4702)
#endif
#include <flecs.h>
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include <format>
#include <string>

using namespace std;

namespace format_example
{

    // Shaped as stratum_core::Notation: an enum class in a named namespace
    enum class Notation
    {
        Block,
        Wall
    };

} // namespace format_example

namespace
{

    // Stand-ins for the level model
    //--------------------------------------------------------------------------
    struct Placement
    {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
    };

    struct Health
    {
        int value = 0;
    };

    struct Zone {};   // Tag, marks an entity as a Zone
    struct Facing {};  // Tag, marks an entity as a wall facing

    struct AttachedTo {};  // Relationship, an alcove attached to a facing
    struct ContainedIn {}; // Relationship, an item inside a container

    //--------------------------------------------------------------------------
    // Without reflection to_json emits component names and no values
    void register_reflection(flecs::world& world)
    {
        world.component<Placement>()
            .member<double>("x")
            .member<double>("y")
            .member<double>("z");

        world.component<Health>()
            .member<int>("value");

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("basics")
    {
        flecs::world world;

        flecs::entity a = world.entity();
        flecs::entity b = world.entity();

        CHECK_CASE("entities have distinct ids", a.id() != b.id());
        CHECK_CASE("a new entity is alive", a.is_alive());

        a.set<Placement>({1.0, 2.0, 3.0});
        a.set<Health>({10});

        CHECK_CASE("has reports a set component", a.has<Placement>());
        CHECK_CASE("get returns the component", a.get<Placement>().x == 1.0);
        CHECK_CASE("try_get is null for an absent component", b.try_get<Placement>() == nullptr);

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("multi component query")
    {
        flecs::world world;

        flecs::entity both = world.entity().set<Placement>({1.0, 0.0, 0.0}).set<Health>({5});
        world.entity().set<Placement>({2.0, 0.0, 0.0});
        world.entity().set<Health>({7});

        size_t count = 0;
        flecs::entity_t id_visited = 0;
        world.query<Placement, Health>().each([&](flecs::entity e, Placement&, Health&)
        {
            ++count;
            id_visited = e.id();
        });

        CHECK_CASE_EQUAL("a two component query visits only entities having both", count, 1);
        CHECK_CASE("the visited entity is the one with both", id_visited == both.id());

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("zone partitioning")
    {
        flecs::world world;

        flecs::entity zone_a = world.entity("zone_a").add<Zone>();
        flecs::entity zone_b = world.entity("zone_b").add<Zone>();

        world.entity().child_of(zone_a).set<Placement>({1.0, 0.0, 0.0});
        world.entity().child_of(zone_a).set<Placement>({2.0, 0.0, 0.0});
        world.entity().child_of(zone_b).set<Placement>({3.0, 0.0, 0.0});

        auto contents = [&](flecs::entity zone)
        {
            size_t count = 0;
            world.query_builder<>().with(flecs::ChildOf, zone).build().each([&](flecs::entity) { ++count; });
            return count;
        };

        CHECK_CASE_EQUAL("a Zone's contents are queryable", contents(zone_a), 2);
        CHECK_CASE_EQUAL("another Zone sees only its own", contents(zone_b), 1);

        size_t zone_count = 0;
        world.query_builder<>().with<Zone>().build().each([&](flecs::entity) { ++zone_count; });
        CHECK_CASE_EQUAL("Zones themselves are queryable", zone_count, 2);

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("relationships")
    {
        flecs::world world;

        flecs::entity zone = world.entity().add<Zone>();
        flecs::entity facing = world.entity().child_of(zone).add<Facing>();
        flecs::entity alcove = world.entity().child_of(zone).add<AttachedTo>(facing);
        flecs::entity coin = world.entity().child_of(zone).add<ContainedIn>(alcove);

        CHECK_CASE("an attachment is a relationship pair", alcove.has<AttachedTo>(facing));
        CHECK_CASE("containment is a relationship pair", coin.has<ContainedIn>(alcove));

        // Reverse traversal, from the facing to what is attached to it
        size_t attached_count = 0;
        flecs::entity_t id_attached = 0;
        world.query_builder<>().with<AttachedTo>(facing).build().each([&](flecs::entity e)
        {
            ++attached_count;
            id_attached = e.id();
        });

        CHECK_CASE_EQUAL("a facing can find what is attached to it", attached_count, 1);
        CHECK_CASE("the attachment found is the alcove", id_attached == alcove.id());

        // Destroying the facing, as mining a wall does
        facing.destruct();

        CHECK_CASE("destroying a facing leaves the alcove alive to be dealt with", alcove.is_alive());
        CHECK_CASE("the destroyed facing's attachment pair is cleaned up", !alcove.has<AttachedTo>(facing));
        CHECK_CASE("what the alcove contained is untouched", coin.is_alive() && coin.has<ContainedIn>(alcove));

        return;
    }

    //--------------------------------------------------------------------------
    // A door and its two door facings in Parent storage, which is for small structured hierarchies; ChildOf would give each
    // door's children a table of their own
    TEST_CASE("parent hierarchy")
    {
        flecs::world world;
        register_reflection(world);

        flecs::entity zone = world.entity("zone").add<Zone>();
        flecs::entity door = world.entity("door").child_of(zone).set<Health>({4});
        flecs::entity facing_west = world.entity(flecs::Parent{door}, "facing_west").add<Facing>();
        flecs::entity facing_east = world.entity(flecs::Parent{door}, "facing_east").add<Facing>().set<Health>({1});

        CHECK_CASE("a child knows its parent", facing_west.parent() == door && facing_east.parent() == door);

        size_t child_count = 0;
        door.children([&](flecs::entity) { ++child_count; });
        CHECK_CASE_EQUAL("a parent iterates its children", child_count, 2);

        // A door facing is in the zone through its door, as an entity has one storage or the other
        size_t zone_child_count = 0;
        world.query_builder<>().with(flecs::ChildOf, zone).build().each([&](flecs::entity) { ++zone_child_count; });
        CHECK_CASE_EQUAL("the zone's own children are the doors, not their facings", zone_child_count, 1);
        CHECK_CASE("a facing is not a ChildOf the zone", !facing_west.has(flecs::ChildOf, zone));

        // Every facing of every door at once
        size_t facing_count = 0;
        world.query_builder<const Facing>().build().each([&](flecs::entity, const Facing&) { ++facing_count; });
        CHECK_CASE_EQUAL("children of many parents are queryable together", facing_count, 2);

        // Flecs 4.1.6 writes a Parent hierarchy, but from_json aborts the process on reading one back, so a saved zone
        // writes each child's parent itself
        string json_world = world.to_json().c_str();

        CHECK_CASE("a parent hierarchy serializes", json_world.find("flecs.core.Parent") != string::npos);
        CHECK_CASE("a child's components serialize with it", json_world.find("\"value\":1") != string::npos);

        // Nested entities, one inside the next: an entity has one parent in one storage, and a child may be a parent
        flecs::entity chest = world.entity("chest").child_of(zone);
        flecs::entity box = world.entity(flecs::Parent{chest}, "box");
        flecs::entity tin = world.entity(flecs::Parent{box}, "tin");
        flecs::entity coin = world.entity(flecs::Parent{tin}, "coin");

        CHECK_CASE("a child of a child of a child knows its parent", coin.parent() == tin && tin.parent() == box && box.parent() == chest);
        CHECK_CASE("each level iterates its own children", [&]
        {
            size_t count = 0;
            box.children([&](flecs::entity e) { ++count; if (e != tin) { count = 0; } });
            return count == 1;
        }());

        CHECK_CASE("the whole chain is reached by path", world.lookup("zone::chest::box::tin::coin") == coin);

        door.destruct();

        CHECK_CASE("deleting a parent deletes its children", !facing_west.is_alive() && !facing_east.is_alive());
        CHECK_CASE("the zone is untouched", zone.is_alive());

        // Deleting the zone reaches what is nested below it through both storages
        zone.destruct();

        CHECK_CASE("deleting a zone deletes its own children", !chest.is_alive());
        CHECK_CASE("and everything nested below them", !box.is_alive() && !tin.is_alive() && !coin.is_alive());

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("migration between Zones")
    {
        flecs::world world;

        flecs::entity zone_a = world.entity().add<Zone>();
        flecs::entity zone_b = world.entity().add<Zone>();

        flecs::entity monster = world.entity().child_of(zone_a).set<Health>({12});
        flecs::entity_t id_before = monster.id();

        // A quest, a leash, or a summoner holding the monster by id
        flecs::entity_t id_reference = monster.id();

        monster.child_of(zone_b);

        CHECK_CASE("migration does not renumber the entity", monster.id() == id_before);
        CHECK_CASE("a reference held by id still resolves", world.entity(id_reference).is_alive());
        CHECK_CASE("component state survives migration", monster.get<Health>().value == 12);
        CHECK_CASE("the entity is no longer in the old Zone", !monster.has(flecs::ChildOf, zone_a));
        CHECK_CASE("the entity is in the new Zone", monster.has(flecs::ChildOf, zone_b));

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("unload as subtree delete")
    {
        flecs::world world;

        flecs::entity zone_a = world.entity().add<Zone>();
        flecs::entity zone_b = world.entity().add<Zone>();

        flecs::entity in_a = world.entity().child_of(zone_a).set<Health>({1});
        flecs::entity nested = world.entity().child_of(in_a);
        flecs::entity in_b = world.entity().child_of(zone_b).set<Health>({2});

        zone_a.destruct();

        CHECK_CASE("deleting a Zone deletes its contents", !in_a.is_alive());
        CHECK_CASE("deletion reaches nested contents", !nested.is_alive());
        CHECK_CASE("another Zone is untouched", in_b.is_alive() && in_b.get<Health>().value == 2);

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("serialization")
    {
        flecs::world world;
        register_reflection(world);

        flecs::entity zone_a = world.entity("zone_a").add<Zone>();
        world.entity("zone_b").add<Zone>();

        world.entity("goblin").child_of(zone_a).set<Placement>({1.0, 2.0, 3.0}).set<Health>({12});
        world.entity("rat").child_of(zone_a).set<Health>({3});

        string json_entity = world.lookup("zone_a::goblin").to_json().c_str();
        CHECK_CASE("an entity serializes its component values", json_entity.find("12") != string::npos, json_entity);

        // One zone's contents alone, as unloading writes them
        string json_zone = world.query_builder<>().with(flecs::ChildOf, zone_a).build().to_json().c_str();
        CHECK_CASE("one Zone's contents serialize on their own", !json_zone.empty() && json_zone.find("goblin") != string::npos, json_zone);

        // Whole world round trip
        string json_world = world.to_json().c_str();
        CHECK_CASE("the world serializes to non-empty JSON", !json_world.empty());

        flecs::world world_restored;
        register_reflection(world_restored);
        world_restored.from_json(json_world.c_str());

        flecs::entity restored = world_restored.lookup("zone_a::goblin");
        CHECK_CASE("a serialized entity is found again after restore", restored.is_alive());

        const Health* phealth = restored.is_alive() ? restored.try_get<Health>() : nullptr;
        CHECK_CASE("component values survive the round trip", phealth != nullptr && phealth->value == 12);

        const Placement* pplacement = restored.is_alive() ? restored.try_get<Placement>() : nullptr;
        CHECK_CASE("multi member components survive the round trip", pplacement != nullptr && pplacement->x == 1.0 && pplacement->z == 3.0);

        flecs::entity zone_restored = world_restored.lookup("zone_a");
        CHECK_CASE("the parent relationship survives the round trip", restored.is_alive() && zone_restored.is_alive() && restored.has(flecs::ChildOf, zone_restored));

        return;
    }

    //--------------------------------------------------------------------------
    // Built identically every time, so that any variation in iteration order shows up
    void build_reference_world(flecs::world& world)
    {
        register_reflection(world);

        flecs::entity zone_a = world.entity("zone_a").add<Zone>();
        flecs::entity zone_b = world.entity("zone_b").add<Zone>();

        for (int i = 0; i < 16; ++i)
        {
            flecs::entity e = world.entity().child_of(i % 2 == 0 ? zone_a : zone_b);
            e.set<Placement>({double(i), 0.0, 0.0});

            // Split the entities across tables, so ordering depends on more than one table
            if (i % 3 == 0)
            {
                e.set<Health>({i});
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    string iteration_fingerprint(flecs::world& world)
    {
        string s;
        world.query<Placement>().each([&](flecs::entity e, Placement& placement)
        {
            s += format("{}:{},", e.id(), int(placement.x));
        });

        return s;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("determinism")
    {
        flecs::world world_a;
        build_reference_world(world_a);

        flecs::world world_b;
        build_reference_world(world_b);

        string fingerprint_a = iteration_fingerprint(world_a);
        string fingerprint_b = iteration_fingerprint(world_b);

        CHECK_CASE("iteration visits every matching entity", !fingerprint_a.empty());
        CHECK_CASE("repeated iteration of one world is stable", iteration_fingerprint(world_a) == fingerprint_a);
        CHECK_CASE("two identically built worlds iterate identically", fingerprint_a == fingerprint_b,
            format("{} vs {}", fingerprint_a, fingerprint_b));

        flecs::world world_c;
        build_reference_world(world_c);
        CHECK_CASE("entity ids are assigned in the same sequence", iteration_fingerprint(world_c) == fingerprint_a);

        // Printed so that separate runs can be compared for ordering differences across processes
        MESSAGE(format("fingerprint {}", fingerprint_a));
        return;
    }

    //--------------------------------------------------------------------------
    // An enum as a component as it is, with no wrapper
    TEST_CASE("enum components")
    {
        using format_example::Notation;

        flecs::world world;
        flecs::entity zone = world.entity().add(Notation::Wall);

        CHECK_CASE("an enum constant is added as it is", zone.has<Notation>() && zone.has(Notation::Wall));
        CHECK_CASE("and read back as its value", zone.get_constant<Notation>() == Notation::Wall);

        zone.add(Notation::Block);
        CHECK_CASE("adding another constant replaces it", zone.get_constant<Notation>() == Notation::Block
            && !zone.has(Notation::Wall));

        world.entity().add(Notation::Wall);

        size_t wall_count = 0;
        world.query_builder().with(Notation::Wall).build().each([&](flecs::entity) { ++wall_count; });
        CHECK_CASE_EQUAL("entities are queryable by constant", wall_count, 1);

        return;
    }

} // namespace
