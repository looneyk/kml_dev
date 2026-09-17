//------------------------------------------------------------------------------
// flecs_test
// main.cpp
//
// Checks that flecs supports the structural decisions in doc/sim_zone_model.md:
// Zones as entities with their contents as children, attachment and containment as
// relationships, migration between Zones without renumbering, unload as a subtree
// delete, per-Zone serialisation, and deterministic iteration order.
//------------------------------------------------------------------------------

#include <flecs.h>
#include <format>
#include <iostream>
#include <string>

using namespace std;

namespace
{

    static constexpr auto escape_reset_string = "\x1b[0m";
    static constexpr auto escape_rendition_pattern = "\x1b[38;5;{}m";

    size_t g_case_count = 0;
    size_t g_failure_count = 0;
    string g_fingerprint;

    //--------------------------------------------------------------------------
    void report_group(const string& name)
    {
        cout << name << endl;
        return;
    }

    //--------------------------------------------------------------------------
    void report_case(const string& name, bool b, const string& detail = {})
    {
        ++g_case_count;

        if (b)
        {
            cout << format(escape_rendition_pattern, 34) << "    ok   " << escape_reset_string << name << endl;
            return;
        }

        ++g_failure_count;
        cout << format(escape_rendition_pattern, 196) << "    FAIL " << escape_reset_string << name;
        if (!detail.empty())
        {
            cout << ": " << detail;
        }
        cout << endl;
        return;
    }

    //--------------------------------------------------------------------------
    void report_case_equal(const string& name, size_t value, size_t value_expected)
    {
        report_case(name, value == value_expected, format("expected {}, got {}", value_expected, value));
        return;
    }

    //--------------------------------------------------------------------------
    int report_results()
    {
        if (g_failure_count == 0)
        {
            cout << format(escape_rendition_pattern, 34) << format("OK: {} cases", g_case_count) << escape_reset_string << endl;
            return 0;
        }

        cout << format(escape_rendition_pattern, 196) << format("Error: {} of {} cases failed", g_failure_count, g_case_count) << escape_reset_string << endl;
        return 1;
    }

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
    void test_basics()
    {
        report_group("basics");

        flecs::world world;

        flecs::entity a = world.entity();
        flecs::entity b = world.entity();

        report_case("entities have distinct ids", a.id() != b.id());
        report_case("a new entity is alive", a.is_alive());

        a.set<Placement>({1.0, 2.0, 3.0});
        a.set<Health>({10});

        report_case("has reports a set component", a.has<Placement>());
        report_case("get returns the component", a.get<Placement>().x == 1.0);
        report_case("try_get is null for an absent component", b.try_get<Placement>() == nullptr);

        return;
    }

    //--------------------------------------------------------------------------
    void test_multi_component_query()
    {
        report_group("multi component query");

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

        report_case_equal("a two component query visits only entities having both", count, 1);
        report_case("the visited entity is the one with both", id_visited == both.id());

        return;
    }

    //--------------------------------------------------------------------------
    void test_zone_partitioning()
    {
        report_group("zone partitioning");

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

        report_case_equal("a Zone's contents are queryable", contents(zone_a), 2);
        report_case_equal("another Zone sees only its own", contents(zone_b), 1);

        size_t zone_count = 0;
        world.query_builder<>().with<Zone>().build().each([&](flecs::entity) { ++zone_count; });
        report_case_equal("Zones themselves are queryable", zone_count, 2);

        return;
    }

    //--------------------------------------------------------------------------
    void test_relationships()
    {
        report_group("relationships");

        flecs::world world;

        flecs::entity zone = world.entity().add<Zone>();
        flecs::entity facing = world.entity().child_of(zone).add<Facing>();
        flecs::entity alcove = world.entity().child_of(zone).add<AttachedTo>(facing);
        flecs::entity coin = world.entity().child_of(zone).add<ContainedIn>(alcove);

        report_case("an attachment is a relationship pair", alcove.has<AttachedTo>(facing));
        report_case("containment is a relationship pair", coin.has<ContainedIn>(alcove));

        // Reverse traversal, asked from the facing's side rather than the alcove's
        size_t attached_count = 0;
        flecs::entity_t id_attached = 0;
        world.query_builder<>().with<AttachedTo>(facing).build().each([&](flecs::entity e)
        {
            ++attached_count;
            id_attached = e.id();
        });

        report_case_equal("a facing can find what is attached to it", attached_count, 1);
        report_case("the attachment found is the alcove", id_attached == alcove.id());

        // Destroying the facing is what mining a wall amounts to
        facing.destruct();

        report_case("destroying a facing leaves the alcove alive to be dealt with", alcove.is_alive());
        report_case("the destroyed facing's attachment pair is cleaned up", !alcove.has<AttachedTo>(facing));
        report_case("what the alcove contained is untouched", coin.is_alive() && coin.has<ContainedIn>(alcove));

        return;
    }

    //--------------------------------------------------------------------------
    // What a door and its door facings need of the Parent storage: a door per zone has two
    // children, which is the small structured hierarchy Parent is for, where ChildOf would put
    // each door's children in a table of their own
    void test_parent_hierarchy()
    {
        report_group("parent hierarchy");

        flecs::world world;
        register_reflection(world);

        flecs::entity zone = world.entity("zone").add<Zone>();
        flecs::entity door = world.entity("door").child_of(zone).set<Health>({4});
        flecs::entity facing_west = world.entity(flecs::Parent {door}, "facing_west").add<Facing>();
        flecs::entity facing_east = world.entity(flecs::Parent {door}, "facing_east").add<Facing>().set<Health>({1});

        report_case("a child knows its parent", facing_west.parent() == door && facing_east.parent() == door);

        size_t child_count = 0;
        door.children([&](flecs::entity) { ++child_count; });
        report_case_equal("a parent iterates its children", child_count, 2);

        // A door facing is in the zone through its door rather than directly, an entity having one
        // storage or the other and never both
        size_t zone_child_count = 0;
        world.query_builder<>().with(flecs::ChildOf, zone).build().each([&](flecs::entity) { ++zone_child_count; });
        report_case_equal("the zone's own children are the doors, not their facings", zone_child_count, 1);
        report_case("a facing is not a ChildOf the zone", !facing_west.has(flecs::ChildOf, zone));

        // Every facing of every door at once, which crossable and the view ask for
        size_t facing_count = 0;
        world.query_builder<const Facing>().build().each([&](flecs::entity, const Facing&) { ++facing_count; });
        report_case_equal("children of many parents are queryable together", facing_count, 2);

        // Flecs 4.1.6 writes a Parent hierarchy and cannot read one back: from_json aborts the process
        // on a world holding one, a bare parent and child being enough to do it. Nothing here may rely
        // on flecs JSON for a hierarchy, and a saved zone writes each child's parent itself
        string json_world = world.to_json().c_str();

        report_case("a parent hierarchy serialises", json_world.find("flecs.core.Parent") != string::npos);
        report_case("a child's components serialise with it", json_world.find("\"value\":1") != string::npos);

        // Nesting to any depth, one entity inside the next: the restriction is that an entity has one
        // parent in one storage, not that a child cannot be a parent
        flecs::entity chest = world.entity("chest").child_of(zone);
        flecs::entity box = world.entity(flecs::Parent {chest}, "box");
        flecs::entity tin = world.entity(flecs::Parent {box}, "tin");
        flecs::entity coin = world.entity(flecs::Parent {tin}, "coin");

        report_case("a child of a child of a child knows its parent", coin.parent() == tin && tin.parent() == box && box.parent() == chest);
        report_case("each level iterates its own children", [&]
        {
            size_t count = 0;
            box.children([&](flecs::entity e) { ++count; if (e != tin) { count = 0; } });
            return count == 1;
        }());

        report_case("the whole chain is reached by path", world.lookup("zone::chest::box::tin::coin") == coin);

        door.destruct();

        report_case("deleting a parent deletes its children", !facing_west.is_alive() && !facing_east.is_alive());
        report_case("the zone is untouched", zone.is_alive());

        // Deleting the zone reaches what is nested below it through both storages
        zone.destruct();

        report_case("deleting a zone deletes its own children", !chest.is_alive());
        report_case("and everything nested below them", !box.is_alive() && !tin.is_alive() && !coin.is_alive());

        return;
    }

    //--------------------------------------------------------------------------
    void test_migration()
    {
        report_group("migration between Zones");

        flecs::world world;

        flecs::entity zone_a = world.entity().add<Zone>();
        flecs::entity zone_b = world.entity().add<Zone>();

        flecs::entity monster = world.entity().child_of(zone_a).set<Health>({12});
        flecs::entity_t id_before = monster.id();

        // A quest, a leash, or a summoner holding the monster by id
        flecs::entity_t id_reference = monster.id();

        monster.child_of(zone_b);

        report_case("migration does not renumber the entity", monster.id() == id_before);
        report_case("a reference held by id still resolves", world.entity(id_reference).is_alive());
        report_case("component state survives migration", monster.get<Health>().value == 12);
        report_case("the entity is no longer in the old Zone", !monster.has(flecs::ChildOf, zone_a));
        report_case("the entity is in the new Zone", monster.has(flecs::ChildOf, zone_b));

        return;
    }

    //--------------------------------------------------------------------------
    void test_unload()
    {
        report_group("unload as subtree delete");

        flecs::world world;

        flecs::entity zone_a = world.entity().add<Zone>();
        flecs::entity zone_b = world.entity().add<Zone>();

        flecs::entity in_a = world.entity().child_of(zone_a).set<Health>({1});
        flecs::entity nested = world.entity().child_of(in_a);
        flecs::entity in_b = world.entity().child_of(zone_b).set<Health>({2});

        zone_a.destruct();

        report_case("deleting a Zone deletes its contents", !in_a.is_alive());
        report_case("deletion reaches nested contents", !nested.is_alive());
        report_case("another Zone is untouched", in_b.is_alive() && in_b.get<Health>().value == 2);

        return;
    }

    //--------------------------------------------------------------------------
    void test_serialization()
    {
        report_group("serialisation");

        flecs::world world;
        register_reflection(world);

        flecs::entity zone_a = world.entity("zone_a").add<Zone>();
        world.entity("zone_b").add<Zone>();

        world.entity("goblin").child_of(zone_a).set<Placement>({1.0, 2.0, 3.0}).set<Health>({12});
        world.entity("rat").child_of(zone_a).set<Health>({3});

        string json_entity = world.lookup("zone_a::goblin").to_json().c_str();
        report_case("an entity serialises its component values", json_entity.find("12") != string::npos, json_entity);

        // One Zone's contents on their own, which is what unloading has to write
        string json_zone = world.query_builder<>().with(flecs::ChildOf, zone_a).build().to_json().c_str();
        report_case("one Zone's contents serialise on their own", !json_zone.empty() && json_zone.find("goblin") != string::npos, json_zone);

        // Whole world round trip
        string json_world = world.to_json().c_str();
        report_case("the world serialises to non-empty JSON", !json_world.empty());

        flecs::world world_restored;
        register_reflection(world_restored);
        world_restored.from_json(json_world.c_str());

        flecs::entity restored = world_restored.lookup("zone_a::goblin");
        report_case("a serialised entity is found again after restore", restored.is_alive());

        const Health* phealth = restored.is_alive() ? restored.try_get<Health>() : nullptr;
        report_case("component values survive the round trip", phealth != nullptr && phealth->value == 12);

        const Placement* pplacement = restored.is_alive() ? restored.try_get<Placement>() : nullptr;
        report_case("multi member components survive the round trip", pplacement != nullptr && pplacement->x == 1.0 && pplacement->z == 3.0);

        flecs::entity zone_restored = world_restored.lookup("zone_a");
        report_case("the parent relationship survives the round trip", restored.is_alive() && zone_restored.is_alive() && restored.has(flecs::ChildOf, zone_restored));

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
    void test_determinism()
    {
        report_group("determinism");

        flecs::world world_a;
        build_reference_world(world_a);

        flecs::world world_b;
        build_reference_world(world_b);

        string fingerprint_a = iteration_fingerprint(world_a);
        string fingerprint_b = iteration_fingerprint(world_b);

        report_case("iteration visits every matching entity", !fingerprint_a.empty());
        report_case("repeated iteration of one world is stable", iteration_fingerprint(world_a) == fingerprint_a);
        report_case("two identically built worlds iterate identically", fingerprint_a == fingerprint_b,
            format("{} vs {}", fingerprint_a, fingerprint_b));

        flecs::world world_c;
        build_reference_world(world_c);
        report_case("entity ids are assigned in the same sequence", iteration_fingerprint(world_c) == fingerprint_a);

        g_fingerprint = fingerprint_a;
        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main()
{
    test_basics();
    test_multi_component_query();
    test_zone_partitioning();
    test_relationships();
    test_parent_hierarchy();
    test_migration();
    test_unload();
    test_serialization();
    test_determinism();

    // Printed so that separate runs can be compared, which is the only way to see
    // whether ordering varies across processes rather than within one
    cout << "fingerprint " << g_fingerprint << endl;

    return report_results();
}
