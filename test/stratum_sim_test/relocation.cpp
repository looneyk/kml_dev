//------------------------------------------------------------------------------
// stratum_sim_test
// relocation.cpp
//------------------------------------------------------------------------------

#include "test_support.hpp"
#include "quetzal/stratum_sim/Command.hpp"
#include "quetzal/stratum_sim/CommandQueue.hpp"
#include "quetzal/stratum_sim/Event.hpp"
#include "quetzal/stratum_sim/EventQueue.hpp"
#include "quetzal/stratum_sim/Intent.hpp"
#include "quetzal/stratum_sim/ContinuousLayout.hpp"
#include "quetzal/stratum_sim/DiscreteLayout.hpp"
#include "quetzal/stratum_sim/DiscreteProximityIndex.hpp"
#include "quetzal/stratum_sim/Name.hpp"
#include "quetzal/stratum_sim/PatternResolver.hpp"
#include "quetzal/stratum_sim/RegionSpec.hpp"
#include "quetzal/stratum_sim/load_layout.hpp"
#include "quetzal/stratum_sim/make_document.hpp"
#include "quetzal/stratum_sim/commands.hpp"
#include "quetzal/stratum_sim/Clock.hpp"
#include "quetzal/stratum_sim/Scheduler.hpp"
#include "quetzal/stratum_sim/Session.hpp"
#include "quetzal/stratum_sim/Game.hpp"
#include "quetzal/stratum_sim/access_points.hpp"
#include "quetzal/stratum_sim/attachments.hpp"
#include "quetzal/stratum_sim/seeds.hpp"
#include "quetzal/stratum_sim/specs.hpp"
#include "quetzal/stratum_sim/World.hpp"
#include "quetzal/stratum_sim/WorldDefinition.hpp"
#include "quetzal/stratum_format/ZoneDocument.hpp"
#include "quetzal/stratum_format/read_zone.hpp"
#include "quetzal/stratum_core/AccessPoint.hpp"
#include "quetzal/stratum_core/spec_keys.hpp"
#include "quetzal/stratum_format/WorldDefinitionDocument.hpp"
#include "quetzal/stratum_format/write_zone.hpp"
#include "quetzal/testing/named_checks.hpp"
#include <doctest/doctest.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

using namespace std;
using namespace quetzal;
using namespace stratum_sim_test;
using quetzal::stratum_sim::CardinalDirection;
using quetzal::stratum_sim::cardinal_direction;

namespace
{

    TEST_SUITE_BEGIN("relocation");

    //--------------------------------------------------------------------------
    TEST_CASE("specs")
    {
        const stratum_sim::RegionSpec spec{.store =quetzal::KeyValueStore{{"type", "surface"}, {"name", "The Downs"}, {"zone.position.z", "-1"},
            {"zone.parameters.denizens", "few"}, {"zones", "3"}}};

        CHECK_CASE("no unknown keys when every key is known", stratum_sim::find_unknown_keys(spec.store,
            vector<string_view>{"type", "name", "zone.position.z", "zone.parameters.denizens", "zones", "difficulty"}).empty());
        CHECK_CASE("the unknown keys, in key order", stratum_sim::find_unknown_keys(spec.store, vector<string_view>{"name"})
            == vector<string>{"type", "zone.parameters.denizens", "zone.position.z", "zones"});

        const KeyValueStore zone_keys = stratum_sim::select_prefixed(spec.store, "zone.");
        CHECK_CASE("the entries under a prefix, with the prefix removed, and not the key zones under zone.",
            zone_keys.value("position.z") == "-1" && zone_keys.value("parameters.denizens") == "few" && !zone_keys.contains("s")
            && zone_keys.size() == 2);
        const KeyValueStore none = stratum_sim::select_prefixed(spec.store, "region.");
        CHECK_CASE("nothing under a prefix that no key has", none.empty());

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("patterns")
    {
        stratum_sim::PatternResolver resolver(KeyValueStore{{"region_name", "crypt"}, {"z", "-3"}, {"region.difficulty", "4"}},
            stratum_sim::PatternResolver::random_type(7), 1, g_test_config.pattern_limits);

        CHECK_CASE("a string pattern resolves to its value", resolver("zone.name", "&region_name .. '_' .. math.abs(z)") == "crypt_3");
        CHECK_CASE("and a number pattern to its value as text", resolver("zone.parameters.difficulty", "&region.difficulty - z") == "7");
        CHECK_CASE("a float is written without a fraction when it is whole", resolver("ratio", "&6 / 2") == "3" && resolver("ratio", "&z / 2") == "-1.5");
        CHECK_CASE("a boolean pattern resolves to true or false", resolver("deep", "&z < -2") == "true" && resolver("deep", "&z < -5") == "false");
        CHECK_CASE("a pattern can be a sequence of statements", resolver("zone.name",
            "&local depth = math.abs(z)\nif depth > 2 then return region_name .. '_deep' end\nreturn region_name") == "crypt_deep");
        CHECK_CASE("an entry a table does not have is nil", resolver("biome", "&region.biome or 'plain'") == "plain");
        CHECK_CASE("a value without & is literal text", resolver("name", "$5 off") == "$5 off");
        CHECK_CASE("and a value that starts with && is literal text that starts with &", resolver("name", "&&Co") == "&Co");
        CHECK_CASE("the pattern &nil resolves to no value", !resolver("name", "&nil").has_value());

        const string drawn = resolver("difficulty", "&random(3, 6)").value_or("");
        CHECK_CASE("random draws an integer from the inclusive interval", drawn.size() == 1 && drawn >= "3" && drawn <= "6", drawn);

        const KeyValueStore resolved = resolver(KeyValueStore{{"zone.name", "&region_name"}, {"type", "&&crypt"},
            {"zone.parameters.difficulty", "&region.difficulty * 2"}, {"name", "Fish & Chips"}, {"seed", "&nil"}});
        CHECK_CASE("a store resolves each entry, and leaves out one whose pattern is &nil", resolved.size() == 4
            && resolved.value("zone.name") == "crypt" && resolved.value("type") == "&crypt"
            && resolved.value("zone.parameters.difficulty") == "8" && resolved.value("name") == "Fish & Chips" && !resolved.contains("seed"));

        CHECK_THROWS_AS(resolver("name", "&level"), runtime_error);
        CHECK_THROWS_AS(resolver("name", "&region_name -"), runtime_error);
        CHECK_THROWS_AS(resolver("name", "&{}"), runtime_error);
        CHECK_THROWS_AS(resolver("name", "&math.abs = nil"), runtime_error);
        CHECK_THROWS_AS(resolver("name", "&while true do end"), runtime_error);
        CHECK_THROWS_AS(resolver("name", "&io"), runtime_error);
        CHECK_THROWS_AS(stratum_sim::PatternResolver(KeyValueStore{{"region", "crypt"}, {"region.difficulty", "4"}},
            stratum_sim::PatternResolver::random_type(7), 1, g_test_config.pattern_limits), runtime_error);
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("region spec patterns")
    {
        stratum_sim::PatternResolver resolver(KeyValueStore{{"parent.name", "barrow"}, {"parent.difficulty", "3"}},
            stratum_sim::PatternResolver::random_type(7), 1, g_test_config.pattern_limits);

        const stratum_sim::RegionSpec region_spec{.name = "&parent.name .. '_crypt'", .display_name = "&nil", .parent = "&parent.name",
            .region_spec = "&crypt", .store = KeyValueStore{{"difficulty", "&parent.difficulty + 1"}, {"service_level", "&nil"},
            {"zone.name", "&region_name .. '_' .. math.abs(z)"}, {"zone.parameters.difficulty", "&region.difficulty - z"}}};
        const stratum_sim::RegionSpec resolved = stratum_sim::resolve_region_spec(region_spec, resolver);

        CHECK_CASE("a region spec's name and display name are resolved, &nil leaving a member with no value",
            resolved.name == "barrow_crypt" && !resolved.display_name.has_value());
        CHECK_CASE("and its parent and region spec are not", resolved.parent == "&parent.name" && resolved.region_spec == "&crypt");
        CHECK_CASE("its store entries are resolved, &nil leaving an entry out", resolved.store.value("difficulty") == "4"
            && !resolved.store.contains("service_level"));
        CHECK_CASE("and its zone entries are not, since they are resolved for each zone",
            resolved.store.value("zone.name") == "&region_name .. '_' .. math.abs(z)"
            && resolved.store.value("zone.parameters.difficulty") == "&region.difficulty - z");
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE_FIXTURE(WorldFixture, "worldspaces and regions")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_core::PersistentId{});
        TestWorld world(id_allocator);

        stratum_core::PersistentId surface = world.create_worldspace({{.region_spec = named_region_spec(test_region_spec("staircase"), "surface")}}, test_worldspace_seed);
        stratum_core::PersistentId crypt_worldspace = world.create_worldspace({{.region_spec = named_region_spec(test_region_spec("staircase"), "crypt")}}, test_worldspace_seed);
        stratum_core::PersistentId crypt = top_level_region(world, crypt_worldspace);

        CHECK_CASE("a worldspace has a persistent id and its name", world.worldspace(surface).worldspace_id == surface
            && world.worldspace(surface).name == "surface");
        CHECK_CASE("the world has its worldspaces in the order they were created", world.worldspaces().size() == 3
            && world.worldspaces()[1].worldspace_id == surface && world.worldspaces()[2].worldspace_id == crypt_worldspace);
        CHECK_CASE("a worldspace's region tree is rooted in its top-level region, with a persistent id and the region spec it was created from",
            world.worldspace(crypt_worldspace).regions.regions().size() == 1 && crypt != crypt_worldspace
            && zone_file_of(world.region(crypt).region_spec) == "data/walls.zone");

        flecs::entity field = load_zone_unpopulated(world, "data/room.zone");
        flecs::entity chapel = world.create_zone(crypt, world.next_id());
        stratum_sim::load_layout(chapel, stratum_format::read_zone("data/corridor.zone"));

        CHECK_CASE("a zone is in its worldspace and its region", stratum_sim::worldspace_of(chapel) == crypt_worldspace
            && stratum_sim::region_of(chapel) == crypt && stratum_sim::region_of(field) == world.region_id);

        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        flecs::entity monk = world.create_entity(chapel,
            stratum_sim::placement_type(zone_layout(world, chapel).position(stratum_sim::discrete_point_type{1, 1}), east));
        stratum_sim::EntityReference monk_reference = world.make_reference(monk);

        CHECK_CASE("an entity in a zone is in that zone, and a zone is in none", stratum_sim::zone_of(monk) == chapel
            && !stratum_sim::zone_of(chapel));

        flecs::entity chapel_below = world.create_zone(crypt, world.next_id());
        stratum_sim::load_layout(chapel_below, stratum_format::read_zone("data/room.zone"));
        stratum_core::PersistentId chapel_id = chapel.get<stratum_core::PersistentId>();
        CHECK_CASE_EQUAL("a world counts the zones of every worldspace, empty or not", world.zone_count(), 3 + 3 * test_extent_zone_count);

        world.unload_worldspace(crypt_worldspace);

        CHECK_CASE("unloading a worldspace removes it and its regions, and deletes its zones",
            !ranges::contains(world.worldspaces(), crypt_worldspace, &stratum_core::Worldspace::worldspace_id) && !chapel.is_alive()
            && !chapel_below.is_alive() && !monk.is_alive());
        CHECK_CASE("and leaves the other worldspaces and their zones", world.worldspaces().size() == 2 && world.worldspaces()[1].worldspace_id == surface
            && field.is_alive() && world.zone_count() == 1 + 2 * test_extent_zone_count);
        CHECK_CASE("the world index keeps the zone of an entity something refers to, as an unloaded zone's",
            world.zone_of(monk_reference.persistent_id) == chapel_id && !world.entity(monk_reference.persistent_id));

        // A zone's position, from its file or its region
        flecs::entity leveled = load_zone_unpopulated_at(world, world.worldspace_id, stratum_core::zone_position_type{0, 0, -3}, "data/door.zone");

        const auto* leveled_position = leveled.try_get<stratum_core::zone_position_type>();
        const map<stratum_core::zone_position_type, stratum_core::PersistentId>& positioned_zone_ids = world.zone_ids(world.worldspace_id);
        CHECK_CASE("a zone loaded at a position has that position", leveled_position != nullptr
            && *leveled_position == stratum_core::zone_position_type{0, 0, -3});
        CHECK_CASE("and is in its worldspace's zone ids at that position, with a zone at each other position of the extent",
            positioned_zone_ids.size() == test_extent_zone_count
            && positioned_zone_ids.at(stratum_core::zone_position_type{0, 0, -3}) == leveled.get<stratum_core::PersistentId>());
        CHECK_CASE("and none where its file states none", field.try_get<stratum_core::zone_position_type>() == nullptr);

        stratum_format::ZoneDocument named_document = stratum_format::read_zone("data/door.zone");
        named_document.components["name"] = "The Narrow Door";
        bool named_buildable = !stratum_sim::check_buildable(world.entities(), named_document).has_value();
        flecs::entity named = world.create_zone(world.region_id, world.next_id());
        stratum_sim::load_layout(named, named_document);

        CHECK_CASE("a zone has the name its file states", named_buildable
            && named.try_get<stratum_sim::Name>() != nullptr && named.get<stratum_sim::Name>().name == "The Narrow Door");
        CHECK_CASE("and the display name derived from it", named.try_get<stratum_sim::DisplayName>() != nullptr
            && named.get<stratum_sim::DisplayName>().name == "THE NARROW DOOR");

        stratum_core::PersistentId dungeon_worldspace = world.create_worldspace({{.region_spec = named_region_spec(test_region_spec("door"), "door")}}, test_worldspace_seed);
        flecs::entity empty = world.entity(world.zone_ids(dungeon_worldspace).at(zone_position_at_level(-1)));
        CHECK_CASE("a worldspace is created with an empty zone at each position of its extent, in its top-level region",
            empty && stratum_sim::empty_zone(empty) && stratum_sim::region_of(empty) == top_level_region(world, dungeon_worldspace)
            && world.region(top_level_region(world, dungeon_worldspace)).zone_ids.size() == test_extent_zone_count);

        flecs::entity made = world.load_zone_at(dungeon_worldspace, zone_position_at_level(-1));
        CHECK_CASE("loading the zone at a position generates its content from its region spec", made == empty
            && level_of(made) == -1 && made.has<stratum_sim::Layout>());

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("region trees")
    {
        auto make_region = [](stratum_core::PersistentId::type id, string name)
        {
            stratum_core::Region region;
            region.region_id = stratum_core::PersistentId{id};
            region.name = std::move(name);
            return region;
        };

        stratum_core::RegionTree regions(make_region(1, "dungeon"));
        regions.add_subregion(stratum_core::PersistentId{1}, make_region(2, "crypt"));
        regions.add_subregion(stratum_core::PersistentId{2}, make_region(3, "ossuary"));
        regions.add_subregion(stratum_core::PersistentId{1}, make_region(4, "mine"));

        vector<stratum_core::PersistentId::type> region_ids;
        for (const stratum_core::Region& region : regions.regions())
        {
            region_ids.push_back(region.region_id.id);
        }

        CHECK_CASE("a region tree is rooted in its top-level region, which has no parent",
            regions.root().region_id == stratum_core::PersistentId{1} && !regions.root().parent_id.has_value());
        CHECK_CASE("a subregion has its parent, and its parent has it as a subregion", regions.region(stratum_core::PersistentId{3}).parent_id
            == stratum_core::PersistentId{2} && regions.region(stratum_core::PersistentId{1}).subregion_ids
            == vector<stratum_core::PersistentId>{stratum_core::PersistentId{2}, stratum_core::PersistentId{4}});
        CHECK_CASE("the regions are in the order they were added, each after its parent",
            region_ids == vector<stratum_core::PersistentId::type>{1, 2, 3, 4});
        CHECK_CASE("a tree contains its own regions",
            regions.contains(stratum_core::PersistentId{3}) && !regions.contains(stratum_core::PersistentId{5}));
        CHECK_CASE("a region's extended name is the name of each region from the top-level region down to it, each preceded by '/'",
            regions.extended_name(stratum_core::PersistentId{3}) == "/dungeon/crypt/ossuary"
            && regions.extended_name(stratum_core::PersistentId{1}) == "/dungeon");

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE_FIXTURE(WorldFixture, "names")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_core::PersistentId{});
        TestWorld world(id_allocator);

        auto name_of = [](flecs::entity e) { return e.get<stratum_sim::Name>().name; };

        stratum_core::PersistentId dungeon_worldspace = world.create_worldspace({{.region_spec = named_region_spec(test_region_spec("door"), "dungeon")}}, test_worldspace_seed);
        stratum_core::PersistentId dungeon = top_level_region(world, dungeon_worldspace);
        stratum_core::PersistentId unnamed_worldspace = world.create_worldspace({{.region_spec = test_region_spec("door")}}, test_worldspace_seed);
        stratum_core::PersistentId second_dungeon_worldspace = world.create_worldspace({{.region_spec = named_region_spec(test_region_spec("door"), "dungeon")}},
            test_worldspace_seed);

        CHECK_CASE("a worldspace has its root region spec's name, and so does its top-level region",
            world.worldspace(dungeon_worldspace).name == "dungeon" && world.region(dungeon).name == "dungeon");
        CHECK_CASE("and one whose root region spec has no name is named for the worldspace count",
            world.worldspace(unnamed_worldspace).name == "worldspace_2" && world.region(top_level_region(world, unnamed_worldspace)).name == "worldspace_2");
        CHECK_CASE("and the name of a root region spec is used as it is, even if another worldspace has it",
            world.worldspace(second_dungeon_worldspace).name == "dungeon");

        flecs::entity empty = world.entity(world.zone_ids(dungeon_worldspace).at(zone_position_at_level(-1)));
        flecs::entity made = world.load_zone_at(dungeon_worldspace, zone_position_at_level(-1));

        CHECK_CASE("an empty zone has its top-level region's name and its count in worldspace position order",
            name_of(world.entity(world.zone_ids(dungeon_worldspace).at(zone_position_at_level(-10)))) == "dungeon_0"
            && name_of(empty) == "dungeon_9");
        CHECK_CASE("and keeps that name when its content is generated", made == empty && name_of(made) == "dungeon_9");

        const KeyValueStore& root_store = world.region(dungeon).region_spec.store;
        CHECK_CASE("a created worldspace's entry_zone is replaced by entry_zone_id, the id of the zone of that name",
            !root_store.contains(stratum_core::entry_zone_key) && name_of(world.entity(stratum_core::PersistentId{
                root_store.value<stratum_core::PersistentId::type>(stratum_core::entry_zone_id_key).value()})) == "dungeon_10");

        stratum_sim::RegionSpec unentered_spec = test_region_spec("door");
        unentered_spec.store.insert(string(stratum_core::entry_zone_key), "nowhere");
        CHECK_THROWS_AS(world.create_worldspace({{.region_spec = named_region_spec(unentered_spec, "unentered")}}, test_worldspace_seed), runtime_error);

        auto zone_name_at = [&](stratum_core::PersistentId worldspace_id, const stratum_core::zone_position_type& position)
        {
            return name_of(world.entity(world.zone_ids(worldspace_id).at(position)));
        };

        stratum_sim::RegionSpec patterned_spec = test_region_spec("door");
        patterned_spec.store.insert("zone.name", "&region_name .. '_' .. math.abs(z) + region.difficulty");
        patterned_spec.store.insert("difficulty", "3");
        patterned_spec.store.remove(stratum_core::entry_zone_key);
        CHECK_CASE("an empty zone in a region whose spec's zone.name is a pattern has the pattern's value, a region entry being a number",
            zone_name_at(world.create_worldspace({{.region_spec = named_region_spec(patterned_spec, "crypt")}}, test_worldspace_seed), zone_position_at_level(-2)) == "crypt_5");

        stratum_sim::RegionSpec literal_spec = test_region_spec("door");
        literal_spec.store.insert("zone.name", "vault");
        literal_spec.store.remove(stratum_core::entry_zone_key);
        const stratum_core::PersistentId vault_worldspace = world.create_worldspace({{.region_spec = named_region_spec(literal_spec, "vault")}}, test_worldspace_seed);
        CHECK_CASE("and in one whose zone.name is literal has that name, which every zone of the region shares",
            zone_name_at(vault_worldspace, zone_position_at_level(-10)) == "vault"
                && zone_name_at(vault_worldspace, zone_position_at_level(-9)) == "vault");

        stratum_sim::RegionSpec unnamed_spec = test_region_spec("door");
        unnamed_spec.store.insert("zone.name", "&nil");
        unnamed_spec.store.remove(stratum_core::entry_zone_key);
        CHECK_CASE("and in one whose zone.name is &nil has its region's name and its coordinates",
            zone_name_at(world.create_worldspace({{.region_spec = named_region_spec(unnamed_spec, "ossuary")}}, test_worldspace_seed), zone_position_at_level(-2)) == "ossuary_0_0_-2");

        // The assets directory is two levels up from the test executable's directory
        const map<string, stratum_sim::RegionSpec> region_specs = stratum_sim::load_region_specs("../../assets");
        CHECK_CASE("the game's region spec file has the surface and the dungeon",
            region_specs.contains("surface") && region_specs.contains("dungeon"));
        CHECK_CASE("and a surface zone is named for the surface and its x and y",
            zone_name_at(world.create_worldspace({{.region_spec = named_region_spec(region_specs.at("surface"), "surface")}}, test_worldspace_seed), stratum_core::zone_position_type{3, 4, 0}) == "surface_3_4");

        const stratum_sim::WorldDefinition world_definition = stratum_sim::load_world_definition("../../assets");
        CHECK_CASE("the game's world definition file starts in the surface tree, its root spec based on the surface",
            world_definition.starting_region == "surface"
            && world_definition.region_spec_trees.at("surface").front().region_spec.region_spec == "surface"
            && world_definition.region_spec_trees.contains("dungeon"));

        auto loaded_definition = [](const string& starting_region, const string& trees)
        {
            const filesystem::path assets_directory = "out/world_definition";
            filesystem::create_directories(assets_directory / "data");
            ofstream(assets_directory / stratum_format::WorldDefinitionDocument::filepath, ios::binary)
                << format("{{ \"format\": \"quetzal.stratum_world_definition\", \"version\": \"0.1\", \"starting_region\": \"{}\", "
                    "\"region_specs\": {} }}\n", starting_region, trees);
            return stratum_sim::load_world_definition(assets_directory);
        };

        auto definition_rejected = [&loaded_definition](const string& starting_region, const string& trees)
        {
            try
            {
                loaded_definition(starting_region, trees);
            }
            catch (const runtime_error&)
            {
                return true;
            }

            return false;
        };

        CHECK_CASE("a world definition whose trees are valid loads", !definition_rejected("moor", "[[{ \"name\": \"moor\" }, "
            "{ \"name\": \"tor\", \"parent\": \"moor\" }]]"));
        CHECK_CASE("and one with a tree of two root specs is rejected", definition_rejected("moor", "[[{ \"name\": \"moor\" }, { \"name\": \"tor\" }]]"));
        CHECK_CASE("and one with a root spec with no name", definition_rejected("moor", "[[{ \"region_spec\": \"moor\" }]]"));
        CHECK_CASE("and one with two trees of the same root", definition_rejected("moor", "[[{ \"name\": \"moor\" }], [{ \"name\": \"moor\" }]]"));
        CHECK_CASE("and one whose starting region is not a tree's root", definition_rejected("fen", "[[{ \"name\": \"moor\" }]]"));
        CHECK_CASE("and one with a spec with no name", definition_rejected("moor", R"([[{ "name": "moor" }, { "parent": "moor" }]])"));
        CHECK_CASE("and one with a parent that is not in the tree", definition_rejected("moor",
            R"([[{ "name": "moor" }, { "name": "tor", "parent": "fen" }]])"));
        CHECK_CASE("and one with a parent that two specs of the tree have as their name", definition_rejected("moor",
            R"([[{ "name": "moor" }, { "name": "tor", "parent": "moor" }, { "name": "barrow", "parent": "moor" },
            { "name": "cellar", "parent": "tor" }, { "name": "cellar", "parent": "barrow" }, { "name": "vault", "parent": "cellar" }]])"));
        CHECK_CASE("and one with two specs of the same name and parent", definition_rejected("moor",
            R"([[{ "name": "moor" }, { "name": "tor", "parent": "moor" }, { "name": "tor", "parent": "moor" }]])"));
        CHECK_CASE("and one with specs that are not below the root", definition_rejected("moor",
            R"([[{ "name": "moor" }, { "name": "tor", "parent": "cairn" }, { "name": "cairn", "parent": "tor" }]])"));

        const stratum_sim::WorldDefinition::region_spec_tree_type tree = loaded_definition("moor", R"([[{ "name": "tor", "parent": "moor" },
            { "name": "moor" }, { "name": "cairn", "parent": "tor" }, { "name": "barrow", "parent": "moor" }]])").region_spec_trees.at("moor");
        auto node_is = [&tree](id_type id, const string& name, id_type parent_id, const vector<id_type>& subregion_ids)
        {
            return tree.at(id).region_spec.name == name && tree.at(id).parent_id == parent_id && tree.at(id).subregion_ids == subregion_ids;
        };

        CHECK_CASE("a region spec tree is ordered with each spec before its subregions, each spec linked to its parent and subregions by slot",
            tree.size() == 4 && node_is(0, "moor", nullid, {1, 2}) && node_is(1, "tor", 0, {3}) && node_is(2, "barrow", 0, {})
            && node_is(3, "cairn", 1, {}));

        const map<string, stratum_sim::RegionSpec> designed_specs{{"barrow", stratum_sim::RegionSpec{.display_name = "Barrow",
            .store = KeyValueStore{{"difficulty", "2"}, {"lore", "old"}}}}};
        stratum_sim::PatternResolver::random_type spec_random(test_worldspace_seed);
        const stratum_sim::WorldDefinition::region_spec_tree_type resolved = stratum_sim::resolve_region_spec_tree(loaded_definition("moor",
            R"([[{ "name": "moor", "difficulty": "3", "zone.name": "&region_name" },
            { "name": "tor", "parent": "moor", "region_spec": "barrow", "difficulty": "&parent.difficulty + 1" },
            { "name": "cairn", "parent": "tor", "lore": "&nil" }]])").region_spec_trees.at("moor"), "moor", designed_specs, spec_random, 1,
            g_test_config.pattern_limits);
        const stratum_sim::RegionSpec& tor = resolved.at(1).region_spec;
        const stratum_sim::RegionSpec& cairn = resolved.at(2).region_spec;

        CHECK_CASE("a subregion spec is its own keys over its designed spec over its parent's store, resolved with its parent's values",
            tor.store.value("difficulty") == "4" && tor.store.value("lore") == "old" && tor.display_name == "Barrow" && !tor.region_spec.has_value());
        CHECK_CASE("and inherits its parent's store but not its typed members, &nil removing an inherited entry",
            cairn.store.value("difficulty") == "4" && !cairn.store.contains("lore") && !cairn.display_name.has_value());
        CHECK_CASE("and inherits zone entries unresolved", cairn.store.value("zone.name") == "&region_name");

        auto inherited_literal = [&loaded_definition, &designed_specs, &spec_random]() -> optional<string>
        {
            try
            {
                const stratum_sim::WorldDefinition::region_spec_tree_type escaped = stratum_sim::resolve_region_spec_tree(loaded_definition("moor",
                    R"([[{ "name": "moor", "motto": "&&Co" }, { "name": "tor", "parent": "moor" }]])").region_spec_trees.at("moor"), "moor", designed_specs,
                    spec_random, 1, g_test_config.pattern_limits);
                return escaped.at(1).region_spec.store.value("motto");
            }
            catch (const runtime_error&)
            {
                return nullopt;
            }
        };

        CHECK_CASE("and inherits a resolved literal that starts with & as that literal", inherited_literal() == "&Co");

        const stratum_sim::RegionSpec root = stratum_sim::resolve_region_spec_tree(loaded_definition("moor",
            R"([[{ "name": "moor", "display_name": "&nil", "lore": "&nil", "motto": "&&Co" }]])").region_spec_trees.at("moor"), "moor", designed_specs,
            spec_random, 1, g_test_config.pattern_limits).front().region_spec;

        CHECK_CASE("a root spec's &nil removes its entry and member, and && leaves a literal that starts with &",
            !root.store.contains("lore") && !root.display_name.has_value() && root.store.value("motto") == "&Co");

        auto moor_tree = [&loaded_definition](const string& subregions)
        {
            return loaded_definition("moor", format(R"([[{{ "name": "moor", "difficulty": "1", "zone.name": "&region_name .. '_' .. x",
                "zone.parameters.difficulty": "&region.difficulty", "extent": {{ "boxes": [{{ "x": [0, 3], "y": 0, "z": 0 }}] }} }}
                {}]])", subregions)).region_spec_trees.at("moor");
        };

        const stratum_core::PersistentId moor = world.create_worldspace(moor_tree(R"(,
            { "name": "tor", "parent": "moor", "difficulty": "&parent.difficulty + 1", "extent": { "boxes": [{ "x": [1, 2] }] } },
            { "name": "cairn", "parent": "tor", "display_name": "The Cairn", "extent": { "positions": [[2, 0, 0]] } })"),
            test_worldspace_seed);
        auto zone_at_x = [&](stratum_sim::coord_type x) { return world.entity(world.zone_ids(moor).at(stratum_core::zone_position_type{x, 0, 0})); };
        auto region_name_at_x = [&](stratum_sim::coord_type x) { return world.region(stratum_sim::region_of(zone_at_x(x))).name; };
        auto zone_spec_at_x = [&](stratum_sim::coord_type x) { return zone_at_x(x).get<stratum_sim::Document>().document->zone_spec.value(); };
        const stratum_core::RegionTree& moor_regions = world.worldspace(moor).regions;

        CHECK_CASE("a worldspace has a region for each spec of its tree, with its parent, name and display name",
            moor_regions.regions().size() == 3 && moor_regions.regions()[1].name == "tor" && moor_regions.regions()[1].parent_id == moor_regions.root().region_id
            && moor_regions.regions()[2].display_name == "The Cairn" && moor_regions.extended_name(moor_regions.regions()[2].region_id) == "/moor/tor/cairn");
        CHECK_CASE("and each zone is in the most specific region whose extent contains its position",
            region_name_at_x(0) == "moor" && region_name_at_x(1) == "tor" && region_name_at_x(2) == "cairn" && region_name_at_x(3) == "moor"
            && moor_regions.root().zone_ids.size() == 2 && moor_regions.regions()[1].zone_ids.size() == 1);
        CHECK_CASE("with its name and spec resolved from that region's spec",
            name_of(zone_at_x(1)) == "tor_1" && name_of(zone_at_x(2)) == "cairn_2" && name_of(zone_at_x(3)) == "moor_3"
            && zone_spec_at_x(1).value("parameters.difficulty") == "2" && zone_spec_at_x(3).value("parameters.difficulty") == "1");

        auto created_nothing = [&](const string& subregions)
        {
            const size_t worldspace_count = world.worldspaces().size();
            const size_t zone_count = world.zone_count();
            try
            {
                world.create_worldspace(moor_tree(subregions), test_worldspace_seed);
            }
            catch (const runtime_error&)
            {
                return world.worldspaces().size() == worldspace_count && world.zone_count() == zone_count;
            }

            return false;
        };

        CHECK_CASE("a worldspace with a subregion spec with no extent is rejected, creating nothing",
            created_nothing(R"(, { "name": "tor", "parent": "moor" })"));
        CHECK_CASE("and one with a subregion whose extent reaches outside the worldspace", created_nothing(R"(,
            { "name": "tor", "parent": "moor", "extent": { "boxes": [{ "x": [3, 4] }] } })"));
        CHECK_CASE("and one with sibling regions whose extents overlap", created_nothing(R"(,
            { "name": "tor", "parent": "moor", "extent": { "boxes": [{ "x": [1, 2] }] } },
            { "name": "fen", "parent": "moor", "extent": { "boxes": [{ "x": [2, 3] }] } })"));
        CHECK_CASE("and one with a subregion whose extent is outside its parent's", created_nothing(R"(,
            { "name": "tor", "parent": "moor", "extent": { "boxes": [{ "x": [1, 2] }] } },
            { "name": "cairn", "parent": "tor", "extent": { "positions": [[3, 0, 0]] } })"));

        // data/door.zone has no name and no position
        flecs::entity first_loaded = world.load_zone(dungeon, "data/door.zone");
        flecs::entity second_loaded = world.load_zone(dungeon, "data/door.zone");

        CHECK_CASE("a zone with no name and no position has its region's name", name_of(first_loaded) == "dungeon");
        CHECK_CASE("and so does a second, since zones of a region can share a name", name_of(second_loaded) == "dungeon");

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE_FIXTURE(WorldFixture, "positions")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_core::PersistentId{});
        TestWorld world(id_allocator);

        stratum_core::PersistentId second_worldspace = world.create_worldspace({{.region_spec = named_region_spec(test_region_spec("door"), "door")}}, test_worldspace_seed);
        flecs::entity made = world.load_zone_at(second_worldspace, zone_position_at_level(-3));
        const stratum_core::zone_position_type& made_position = made.get<stratum_core::zone_position_type>();
        CHECK_CASE("a zone is at the position its spec has", made_position == stratum_core::zone_position_type{0, 0, -3});

        const stratum_format::ZoneDocument made_document = stratum_sim::make_document(made);
        CHECK_CASE("and a saved zone keeps its position", made_document.position == stratum_format::zone_position_type{0, 0, -3});

        CHECK_CASE("a coordinate a spec does not have is 0", stratum_core::ZoneSpec{KeyValueStore{{string(stratum_core::position_z_key), "5"}}}.position().x() == 0
            && stratum_core::ZoneSpec{}.position().z() == 0);
        const stratum_core::ZoneSpec compass_spec{KeyValueStore{{string(stratum_core::position_x_key), "east"}}};
        CHECK_THROWS_AS(compass_spec.position(), runtime_error);

        // A worldspace's coordinate system
        stratum_sim::RegionSpec barrow_spec = test_region_spec("door");
        barrow_spec.extent = box_extent(range_type(0, 0), range_type(0, 0), range_type(-10, -1));
        barrow_spec.zone_size = stratum_sim::discrete_vector_type(64, 32);
        barrow_spec.store.remove(stratum_core::entry_zone_key);
        stratum_core::PersistentId barrow_worldspace = world.create_worldspace({{.region_spec = named_region_spec(barrow_spec, "barrow")}}, test_worldspace_seed);
        const stratum_core::Worldspace barrow = world.worldspace(barrow_worldspace);

        using extent_type = stratum_core::Worldspace::extent_type;
        CHECK_CASE("a worldspace has the extent and zone size in its region spec, each interval's upper bound included",
            barrow.extent == extent_type(extent_type::point_type(0, 0, -10), extent_type::point_type(1, 1, 0))
            && barrow.zone_size == stratum_sim::discrete_vector_type(64, 32));

        auto creation_throws = [&](const optional<stratum_core::RegionExtent>& extent)
        {
            stratum_sim::RegionSpec spec = test_region_spec("door");
            spec.extent = extent;
            try
            {
                world.create_worldspace({{.region_spec = named_region_spec(spec, "malformed")}}, test_worldspace_seed);
            }
            catch (const runtime_error&)
            {
                return true;
            }
            return false;
        };

        stratum_core::RegionExtent two_boxes = box_extent(range_type(0, 0), range_type(0, 0), range_type(-10, 10));
        two_boxes.boxes.push_back(two_boxes.boxes.front());
        stratum_core::RegionExtent unbounded = box_extent(range_type(0, 0), range_type(0, 0), range_type(-10, 10));
        unbounded.boxes.front().y_extent.reset();
        const stratum_core::RegionExtent listed{.arrangement = stratum_core::RegionExtent::Arrangement::Positions, .boxes = {},
            .positions = {stratum_format::zone_position_type{0, 0, 0}}};

        CHECK_CASE("a worldspace with no extent is an error", creation_throws(nullopt));
        CHECK_CASE("and one whose extent has two boxes", creation_throws(two_boxes));
        CHECK_CASE("and one whose extent is a list of positions", creation_throws(listed));
        CHECK_CASE("and one whose extent's box has no range on an axis", creation_throws(unbounded));

        flecs::entity deep = world.load_zone_at(barrow_worldspace, zone_position_at_level(-4));
        auto barrow_zone_ids = [&]() { return world.zone_ids(barrow_worldspace); };
        CHECK_CASE("a worldspace has the id of each of its zones at the zone's position", barrow_zone_ids().size() == 10
            && barrow_zone_ids().at(zone_position_at_level(-4)) == deep.get<stratum_core::PersistentId>());

        size_t zone_count = world.zone_count();
        size_t filled_count = filled_zone_count(world);
        CHECK_THROWS_AS(world.load_template(barrow_worldspace, zone_position_at_level(-4), "data/door.zone", stratum_core::ZoneSpec{}), runtime_error);
        CHECK_THROWS_AS(world.load_zone_at(barrow_worldspace, zone_position_at_level(0)), runtime_error);
        CHECK_CASE("a template at a zone that is not empty, or a position outside the extent, is an error, and nothing is built",
            world.zone_count() == zone_count && filled_zone_count(world) == filled_count && barrow_zone_ids().size() == 10);

        flecs::entity same_position = world.load_zone_at(second_worldspace, zone_position_at_level(-4));
        CHECK_CASE("a zone of another worldspace may be at the same position", level_of(same_position) == -4
            && stratum_sim::worldspace_of(same_position) == second_worldspace);

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE_FIXTURE(WorldFixture, "seeds")
    {
        const array<uint64_t, 2> values{1, 3};
        const stratum_sim::seed_type derived = stratum_sim::derived_seed(7, values);
        CHECK_CASE("a derived seed is the same for the same seed and values", stratum_sim::derived_seed(7, values) == derived);
        CHECK_CASE("and differs by seed, by value, and by value order", stratum_sim::derived_seed(8, values) != derived
            && stratum_sim::derived_seed(7, array<uint64_t, 2>{1, 4}) != derived && stratum_sim::derived_seed(7, array<uint64_t, 2>{3, 1}) != derived);

        const stratum_sim::RegionSpec region_spec = test_region_spec("door");
        const stratum_core::ZoneSpec zone_spec = test_zone_spec(-3);

        vector<stratum_sim::seed_type> generated_seeds;
        stratum_sim::Generation generation = test_generation();
        generation.m_zone_generator = [&generated_seeds](const stratum_core::ZoneSpec& requested_zone_spec)
        {
            generated_seeds.push_back(*requested_zone_spec.store.value<stratum_sim::seed_type>(stratum_core::seed_key));
            stratum_format::ZoneDocument document = stratum_format::read_zone("data/door.zone");
            document.position = requested_zone_spec.position();
            document.is_unpopulated = false;
            return document;
        };

        auto generate_zone = [&generation, &zone_spec](stratum_sim::seed_type worldspace_seed,
            const stratum_sim::RegionSpec& requested_region_spec)
        {
            stratum_sim::CountingIdAllocator id_allocator(stratum_core::PersistentId{});
            stratum_sim::World world(id_allocator, generation, test_zone_count_max, g_test_config.pattern_limits);
            stratum_sim::register_game_types(world);
            world.load_zone_at(world.create_worldspace({{.region_spec = named_region_spec(requested_region_spec, "door")}}, worldspace_seed), zone_spec.position());
            return;
        };

        stratum_sim::RegionSpec zone_seeded_region_spec = region_spec;
        zone_seeded_region_spec.store.insert(format("{}{}", stratum_core::zone_prefix, stratum_core::seed_key), "12");

        generate_zone(7, region_spec);
        generate_zone(7, region_spec);
        generate_zone(8, region_spec);
        generate_zone(7, zone_seeded_region_spec);

        stratum_sim::PatternResolver::random_type worldspace_random(7);
        stratum_sim::seed_type position_order_seed = 0;
        for (stratum_sim::coord_type z = region_spec.extent->boxes.front().z_extent->lower(); z <= zone_spec.position().z(); ++z)
        {
            position_order_seed = worldspace_random.seed();
        }

        CHECK_CASE("a zone has the seed that its worldspace's random engine draws for it in worldspace position order",
            generated_seeds.size() == 4 && generated_seeds[0] == position_order_seed);
        CHECK_CASE("the same for the same worldspace seed, and differing for another", generated_seeds.size() == 4
            && generated_seeds[1] == generated_seeds[0] && generated_seeds[2] != generated_seeds[0]);
        CHECK_CASE("and the seed its region spec's zone keys have, if they have one", generated_seeds.size() == 4 && generated_seeds[3] == 12);

        auto drawn_lore = [&](optional<stratum_sim::seed_type> spec_seed, stratum_sim::seed_type worldspace_seed)
        {
            const stratum_sim::WorldDefinition::region_spec_tree_type tree{{.region_spec = stratum_sim::RegionSpec{.name = "moor", .seed = spec_seed,
                .store = KeyValueStore{{"lore", "&random(1000000000)"}}}}};
            stratum_sim::PatternResolver::random_type random(worldspace_seed);
            return stratum_sim::resolve_region_spec_tree(tree, "fen", {}, random, 1, g_test_config.pattern_limits).front().region_spec.store.value("lore");
        };

        CHECK_CASE("a region spec's patterns draw from a seed that its worldspace's random engine draws",
            drawn_lore(nullopt, 7) == drawn_lore(nullopt, 7) && drawn_lore(nullopt, 7) != drawn_lore(nullopt, 8));
        CHECK_CASE("or from the spec's seed, if it has one", drawn_lore(11, 7) == drawn_lore(11, 8) && drawn_lore(11, 7) != drawn_lore(nullopt, 7));
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE_FIXTURE(WorldFixture, "level destinations")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_core::PersistentId{});
        TestWorld world(id_allocator);

        flecs::entity staircase_type = stratum_sim::entity_type(world.entities(), "staircase");
        auto up_of = [&](flecs::entity zone) { return stratum_sim::entry_point(zone, staircase_type, 1); };
        auto down_of = [&](flecs::entity zone) { return stratum_sim::entry_point(zone, staircase_type, -1); };
        auto retarget = [](flecs::entity e, string_view text)
        {
            e.get_mut<stratum_core::AccessPoint>().destination = *stratum_core::parse_access_point_destination(text);
        };

        // data/walls.zone has one staircase leading up and one leading down
        auto load_at_level = [&](stratum_core::PersistentId worldspace_id, stratum_sim::level_type level)
        {
            return load_zone_unpopulated_at(world, worldspace_id, zone_position_at_level(level), "data/walls.zone");
        };

        auto zone_arrived_in = [](flecs::entity destination) { return destination ? stratum_sim::zone_of(destination) : flecs::entity(); };

        stratum_core::PersistentId dungeon_worldspace = world.create_worldspace({{.region_spec = named_region_spec(test_region_spec("staircase"), "staircase")}}, test_worldspace_seed);
        stratum_core::PersistentId dungeon = top_level_region(world, dungeon_worldspace);

        flecs::entity field = load_at_level(world.worldspace_id, 0);
        flecs::entity first = load_at_level(dungeon_worldspace, -1);
        flecs::entity second = load_at_level(dungeon_worldspace, -2);
        flecs::entity first_up = up_of(first);
        flecs::entity first_down = down_of(first);
        flecs::entity second_up = up_of(second);

        // Resolving does not bind, so the test case binds a destination to keep it from being found again
        auto bind = [&](flecs::entity destination)
        {
            if (destination)
            {
                destination.get_mut<stratum_core::AccessPoint>().destination = stratum_core::AccessPoint::destination_type{world.make_reference(first_up).persistent_id};
            }
        };

        CHECK_CASE("moving down finds the unbound staircase that leads back, in the zone of its worldspace at the level below",
            world.resolve_destination(first_down) == second_up);
/*
        retarget(first_down, "=-2");
        CHECK_CASE("an absolute destination finds a zone at its level", world.resolve_destination(first_down) == second_up);
        retarget(first_down, "../[-1]");
        retarget(second_up, "=-1");
        CHECK_CASE("a staircase that leads back with an absolute destination is found", world.resolve_destination(first_down) == second_up);
*/
        retarget(second_up, "../[+1]");

        retarget(first_up, "dungeon");
        size_t worldspaces_before_staircase = world.worldspaces().size();
        flecs::entity destination = world.resolve_destination(first_up);
        flecs::entity made = zone_arrived_in(destination);
        stratum_core::PersistentId region_made = made ? stratum_sim::region_of(made) : stratum_core::PersistentId{};
        CHECK_CASE("a destination with a region spec name generates its entry zone in a new worldspace created from that region spec, and named for it, though a zone at the level has a staircase that leads back", made
            && made != field && region_made != dungeon && region_made != world.region_id
            && zone_file_of(world.region(region_made).region_spec) == "data/walls.zone"
            && world.region(region_made).zone_ids.size() == test_extent_zone_count && level_of(made) == 0
            && world.worldspace(stratum_sim::worldspace_of(made)).name == "dungeon" && destination == down_of(made)
            && world.worldspaces().size() == worldspaces_before_staircase + 1);
        CHECK_CASE("and the same worldspace again", zone_arrived_in(world.resolve_destination(first_up)) == made
            && world.worldspaces().size() == worldspaces_before_staircase + 1);

        retarget(first_down, "dungeon");
        CHECK_CASE("and another access point with that destination arrives in the same worldspace",
            zone_arrived_in(world.resolve_destination(first_down)) == made && world.worldspaces().size() == worldspaces_before_staircase + 1);
        retarget(first_down, "../[-1]");

        // data/walls.zone with its staircase leading down retargeted to a worldspace created from the door spec
        stratum_format::ZoneDocument vault_document = stratum_format::read_zone("data/walls.zone");
        for (auto& entity : vault_document.entities)
        {
            const stratum_core::AccessPoint::destination_type leading_down = stratum_core::parse_access_point_destination("../[-1]").value();
            if (entity.access_point.has_value() && entity.access_point->destination == leading_down)
            {
                entity.access_point->destination = stratum_core::parse_access_point_destination("door").value();
            }
        }

        // The vault is one level above the entry zone, so the entry zone's staircase leading up leads back
        filesystem::create_directories("out");
        stratum_format::write("out/vault_down.zone", vault_document);
        flecs::entity vault_above = world.load_template(dungeon_worldspace, zone_position_at_level(1), "out/vault_down.zone", stratum_core::ZoneSpec{});
        flecs::entity vault_down;
        world.entities().query_builder<const stratum_core::AccessPoint>().with(flecs::ChildOf, vault_above).build()
            .each([&](flecs::entity e, const stratum_core::AccessPoint& access_point)
            {
                if (access_point.destination.has_value() && holds_alternative<stratum_core::RegionSpecName>(*access_point.destination))
                {
                    vault_down = e;
                }
            });
        REQUIRE(vault_down);

        auto vault = ranges::find(world.worldspaces(), "door", &stratum_core::Worldspace::name);
        CHECK_CASE("filling a zone creates the destination worldspace of each destination with a region spec name", vault != world.worldspaces().end()
            && zone_file_of(vault->regions.root().region_spec) == "data/door.zone");
        made = zone_arrived_in(world.resolve_destination(vault_down));
        CHECK_CASE("and going through the access point arrives in it", made && vault != world.worldspaces().end()
            && stratum_sim::worldspace_of(made) == vault->worldspace_id);

        retarget(first_up, "unregistered");
        bool region_spec_unregistered = false;

        try
        {
            world.resolve_destination(first_up);
        }
        catch (const runtime_error&)
        {
            region_spec_unregistered = true;
        }

        CHECK_CASE("a region spec that is not registered is an error", region_spec_unregistered);
        retarget(first_up, "../[+1]");

        destination = world.resolve_destination(down_of(second));
        made = zone_arrived_in(destination);
        CHECK_CASE("finding the zone at the level in its own worldspace empty, it generates its content and arrives at its entry point", made
            && stratum_sim::region_of(made) == dungeon && level_of(made) == -3 && destination == up_of(made));
        CHECK_CASE("the placeholder finds the zone at x 0 and y 0, at its level",
            made && made.get<stratum_core::zone_position_type>() == stratum_core::zone_position_type{0, 0, -3});

        destination = world.resolve_destination(first_up);
        made = zone_arrived_in(destination);
        CHECK_CASE("moving up looks only in its own worldspace, so the zone at level 0 in another is not found", made
            && made != field && stratum_sim::region_of(made) == dungeon && level_of(made) == 0
            && destination == down_of(made));
        bind(destination);

        auto throws = [&](flecs::entity access_point)
        {
            try
            {
                world.resolve_destination(access_point);
            }
            catch (const runtime_error&)
            {
                return true;
            }
            return false;
        };

        world.unload_zone(second);
        CHECK_CASE("the zone at the destination's position is read back if it has unloaded, which a world with no game directory cannot do",
            throws(first_down));

        // A zone made from data/door.zone above a staircase leading up has no staircase leading back down
        retarget(first_up, "door");
        CHECK_CASE("a zone made in a new worldspace with no access point that leads back is an error", throws(first_up));
        retarget(first_up, "../[+1]");

        stratum_core::PersistentId doors_worldspace = world.create_worldspace({{.region_spec = named_region_spec(test_region_spec("door"), "door")}}, test_worldspace_seed);
        flecs::entity below_doors = load_at_level(doors_worldspace, -1);
        size_t worldspace_count = world.worldspaces().size();
        size_t filled_count = filled_zone_count(world);
        flecs::entity above_doors = world.entity(world.zone_ids(doors_worldspace).at(zone_position_at_level(0)));
        CHECK_CASE("a zone generated in its own worldspace with no access point that leads back is an error, and is kept",
            throws(up_of(below_doors)) && world.worldspaces().size() == worldspace_count && below_doors.is_alive()
            && above_doors.is_alive() && !stratum_sim::empty_zone(above_doors) && filled_zone_count(world) == filled_count + 1);
        CHECK_CASE("and so is the same zone found again", throws(up_of(below_doors)) && filled_zone_count(world) == filled_count + 1);

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE_FIXTURE(WorldFixture, "access points")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_core::PersistentId{});
        TestWorld world(id_allocator);

        // Two rooms, a staircase leading up in one and one leading down in the other
        flecs::entity zone = load_zone_unpopulated(world, "data/walls.zone");
        const stratum_sim::DiscreteLayout& layout = zone_layout(world, zone);

        flecs::entity staircase_type = stratum_sim::entity_type(world.entities(), "staircase");
        flecs::entity portal_type = stratum_sim::entity_type(world.entities(), "portal");

        CHECK_CASE("the game registers the types staircase, door, portal and waypoint", staircase_type && portal_type
            && stratum_sim::entity_type(world.entities(), "door") && stratum_sim::entity_type(world.entities(), "waypoint")
            && !stratum_sim::entity_type(world.entities(), "up") && !stratum_sim::entity_type(world.entities(), "chute"));

        flecs::entity up = stratum_sim::entry_point(zone, staircase_type, 1);
        flecs::entity down = stratum_sim::entry_point(zone, staircase_type, -1);

        CHECK_CASE("the loader builds each access point of the file, of its type", up && down && up != down);
        const auto* up_cell = up ? get_if<stratum_sim::FlatCellPosition>(&*up.get<stratum_sim::Site>().address) : nullptr;
        CHECK_CASE("each sited on the floor of its stair block, as the file sites it", up_cell != nullptr
            && up_cell->cell_position == stratum_sim::discrete_point_type{1, 1}
            && up.get<stratum_core::AccessPoint>().egress == stratum_sim::discrete_vector_type{1, 0});
        CHECK_CASE("with the order the file states", up.get<stratum_core::AccessPoint>().order == 0
            && down.get<stratum_core::AccessPoint>().order == 1);
        CHECK_CASE("an actor arrives in the cell beside the stair block, facing along the egress direction",
            layout.cell_position_at(stratum_sim::destination_placement(up).position()) == stratum_sim::discrete_point_type{2, 1}
            && stratum_sim::destination_placement(up).direction().x() > 0.0f);
        CHECK_CASE("an entry point's destination is the level at the offset", !stratum_sim::entry_point(zone, staircase_type, -2)
            && stratum_sim::targets_level(up, 1) && !stratum_sim::targets_level(up, -1));

        // Of the access points whose destinations are the offset, the lowest-ordered is the entry point
        flecs::entity up_2 = stratum_sim::create_access_point(zone, staircase_type, up.get<stratum_sim::Site>(), stratum_core::AccessPoint{.order = 2,
            .egress = up.get<stratum_core::AccessPoint>().egress, .destination = stratum_core::ZoneExtendedName{"../[+1]"}});
        CHECK_CASE("the entry point is the access point of the type with the lowest order", stratum_sim::entry_point(zone, staircase_type, 1) == up
            && up_2.is_alive());

        // A portal on the floor, in the middle of a room
        stratum_sim::placement_type north(stratum_sim::point_type(0.0f, 0.0f, 0.0f), stratum_sim::vector_type(0.0f, 1.0f, 0.0f));
        flecs::entity portal = stratum_sim::create_access_point(zone, portal_type,
            stratum_sim::Site{stratum_sim::Relation::On, stratum_sim::FlatCellPosition{layout.floor_id(), stratum_sim::discrete_point_type{0, 1}}, north},
            stratum_core::AccessPoint{.destination = stratum_core::ZoneExtendedName{"../[+0]"}});
        CHECK_CASE("an access point of any type, on any surface, is an entry point", stratum_sim::entry_point(zone, portal_type, 0) == portal);
        CHECK_CASE("an actor arrives on the floor of the access point's flat cell, facing in the site's orientation",
            stratum_sim::destination_placement(portal).position() == layout.position(stratum_sim::discrete_point_type{0, 1})
            && stratum_sim::destination_placement(portal).direction().y() > 0.0f);

        // A wall id decodes back to the cell it is on and the direction it faces, for all four
        bool decoded = true;
        for (const auto& direction : stratum_sim::cardinal_directions)
        {
            id_type wall_id = layout.wall_toward(stratum_sim::discrete_point_type{0, 0}, direction);
            if (wall_id != nullid)
            {
                decoded = decoded && layout.wall_cell_position(wall_id) == stratum_sim::discrete_point_type{0, 0}
                    && layout.wall_direction(wall_id) == direction;
            }
        }

        CHECK_CASE("a wall id decodes to its cell and its direction", decoded);

        // Entering places the character on up staircase 0, which is on the floor of its cell, facing in its orientation
        stratum_sim::Game game(g_test_config, g_test_manifest);
        stratum_sim::Session& session = game.session();
        enter_zone_file(game, "data/corridor.zone");

        const stratum_sim::placement_type& start = stratum_sim::placement_of(session.character().entity());
        CHECK_CASE("the character enters on the staircase leading up",
            stratum_sim::layout_as<stratum_sim::DiscreteLayout>(session.layout()).cell_position_at(start.position()) == stratum_sim::discrete_point_type{1, 2});
        CHECK_CASE("facing in its orientation", start.direction().y() < 0.0f && start.direction().x() == 0.0f);

        // A zone with a portal alone is entered at the portal, and not by default
        flecs::entity portal_only = game.world().create_zone(stratum_sim::region_of(session.zone()), make_unique<stratum_sim::DiscreteLayout>(layout.grid().origin, 1.0f,
            layout.grid().extent));
        stratum_sim::DiscreteLayout& portal_only_layout = zone_layout(game.world(), portal_only);
        portal_only_layout.set_cell_open(stratum_sim::discrete_point_type{0, 0}, true);
        portal_only_layout.set_cell_walls(stratum_sim::discrete_point_type{0, 0}, stratum_sim::DirectionMask::West);

        flecs::entity game_portal_type = stratum_sim::entity_type(game.world().entities(), "portal");
        stratum_sim::create_access_point(portal_only, game_portal_type,
            stratum_sim::Site{stratum_sim::Relation::On, stratum_sim::FlatCellPosition{portal_only_layout.floor_id(), stratum_sim::discrete_point_type{0, 0}},
            stratum_sim::placement_type(stratum_sim::point_type(0.0f, 0.0f, 0.0f), stratum_sim::vector_type(1.0f, 0.0f, 0.0f))},
            stratum_core::AccessPoint{.destination = stratum_core::ZoneExtendedName{"../[+0]"}});

        bool thrown = false;

        try
        {
            session.enter(portal_only, test_entry_type(session), test_entry_offset);
        }
        catch (const runtime_error&)
        {
            thrown = true;
        }

        CHECK_CASE("entering a zone with no staircase leading up throws", thrown && session.zone() != portal_only);

        session.enter(portal_only, game_portal_type, 0);
        CHECK_CASE("and entering it at its portal places the character at the portal's placement", session.zone() == portal_only
            && portal_only_layout.cell_position_at(stratum_sim::placement_of(session.character().entity()).position()) == stratum_sim::discrete_point_type{0, 0});

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE_FIXTURE(WorldFixture, "relocating through an access point")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_core::PersistentId{});
        TestWorld world(id_allocator);

        flecs::entity staircase_type = stratum_sim::entity_type(world.entities(), "staircase");
        auto up_of = [&](flecs::entity zone) { return stratum_sim::entry_point(zone, staircase_type, 1); };
        auto down_of = [&](flecs::entity zone) { return stratum_sim::entry_point(zone, staircase_type, -1); };
        auto destination_of = [](flecs::entity e) { return *e.get<stratum_core::AccessPoint>().destination; };
        auto refers_to = [&](flecs::entity e, flecs::entity to)
        {
            const auto* bound = get_if<stratum_core::PersistentId>(&*e.get<stratum_core::AccessPoint>().destination);
            return bound != nullptr && *bound == to.get<stratum_core::PersistentId>();
        };

        // data/walls.zone has one staircase leading up and one leading down
        stratum_core::PersistentId dungeon_worldspace = world.create_worldspace({{.region_spec = named_region_spec(test_region_spec("staircase"), "staircase")}}, test_worldspace_seed);
        auto load_at_level = [&](stratum_sim::level_type level)
        {
            return load_zone_unpopulated_at(world, dungeon_worldspace, zone_position_at_level(level), "data/walls.zone");
        };

        flecs::entity first = load_at_level(-1);
        flecs::entity second = load_at_level(-2);
        flecs::entity first_down = down_of(first);
        flecs::entity second_up = up_of(second);
        stratum_sim::placement_type second_up_placement = stratum_sim::destination_placement(second_up);

        flecs::entity actor = world.create_entity(first, stratum_sim::destination_placement(first_down));

        flecs::entity destination = world.relocate_through(actor, first_down);
        CHECK_CASE("an actor goes through an unbound access point to the access point that leads back", destination == second_up
            && stratum_sim::zone_of(actor) == second);
        CHECK_CASE("and is placed at its destination placement", stratum_sim::placement_of(actor).position() == second_up_placement.position()
            && stratum_sim::placement_of(actor).direction() == second_up_placement.direction());
        CHECK_CASE("both ends are bound to each other", refers_to(first_down, second_up) && refers_to(second_up, first_down));
        CHECK_CASE("and a bound access point is no longer an entry point", !up_of(second));

        size_t zone_count = world.zone_count();
        destination = world.relocate_through(actor, second_up);
        CHECK_CASE("going back through the bound access point arrives at the access point gone through", destination == first_down
            && stratum_sim::zone_of(actor) == first && world.zone_count() == zone_count);

        destination = world.relocate_through(actor, first_down);
        flecs::entity second_down = down_of(second);
        destination = world.relocate_through(actor, second_down);
        flecs::entity made = destination ? stratum_sim::zone_of(destination) : flecs::entity();
        CHECK_CASE("finding the zone at the destination level empty, it goes to it with its content generated and binds its entry point", made
            && level_of(made) == -3 && stratum_sim::zone_of(actor) == made && refers_to(second_down, destination)
            && refers_to(destination, second_down));

        auto throws = [&](flecs::entity access_point)
        {
            try
            {
                world.relocate_through(actor, access_point);
            }
            catch (const runtime_error&)
            {
                return true;
            }
            return false;
        };

        flecs::entity unresolved = up_of(first);
        unresolved.get_mut<stratum_core::AccessPoint>().destination = *stratum_core::parse_access_point_destination("unregistered");
        stratum_core::AccessPoint::destination_type unresolved_destination = destination_of(unresolved);
        CHECK_CASE("a destination that is not resolved is an error, and nothing moves or binds", throws(unresolved)
            && stratum_sim::zone_of(actor) == made && destination_of(unresolved) == unresolved_destination);

        world.unload_zone(first);
        CHECK_CASE("a bound destination whose zone has unloaded, in a world with no game directory to read it back from, is an error, and nothing moves",
            throws(second_up) && stratum_sim::zone_of(actor) == made);

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE_FIXTURE(WorldFixture, "one-way access points")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_core::PersistentId{});
        TestWorld world(id_allocator);

        // A chute leads down to a chute that is arrival only
        flecs::entity chute_type = stratum_sim::create_entity_type(world.entities(), "chute");

        stratum_core::PersistentId dungeon_worldspace = world.create_worldspace({{.region_spec = named_region_spec(test_region_spec("staircase"), "staircase")}}, test_worldspace_seed);
        auto load_at_level = [&](stratum_sim::level_type level)
        {
            return load_zone_unpopulated_at(world, dungeon_worldspace, zone_position_at_level(level), "data/walls.zone");
        };

        auto on_floor = [&](flecs::entity zone, stratum_sim::discrete_point_type cell_position)
        {
            const stratum_sim::DiscreteLayout& layout = zone_layout(world, zone);
            return stratum_sim::Site{stratum_sim::Relation::On, stratum_sim::FlatCellPosition{layout.floor_id(), cell_position},
                stratum_sim::placement_type(stratum_sim::point_type(0.0f, 0.0f, 0.0f), stratum_sim::vector_type(1.0f, 0.0f, 0.0f))};
        };

        flecs::entity first = load_at_level(-1);
        flecs::entity second = load_at_level(-2);

        flecs::entity chute = stratum_sim::create_access_point(first, chute_type, on_floor(first, {2, 1}),
            stratum_core::AccessPoint{.destination = stratum_core::parse_access_point_destination("../[-1]")});
        flecs::entity landing = stratum_sim::create_access_point(second, chute_type, on_floor(second, {2, 1}), stratum_core::AccessPoint{});
        stratum_sim::create_access_point(second, chute_type, on_floor(second, {0, 1}), stratum_core::AccessPoint{.order = 1});

        flecs::entity actor = world.create_entity(first, stratum_sim::destination_placement(chute));
        flecs::entity destination = world.relocate_through(actor, chute);
        const auto* chute_bound = get_if<stratum_core::PersistentId>(&*chute.get<stratum_core::AccessPoint>().destination);

        CHECK_CASE("an actor goes through an access point to the lowest-ordered destination-only access point of its type",
            destination == landing && stratum_sim::zone_of(actor) == second);
        CHECK_CASE("which binds only the access point gone through", chute_bound != nullptr
            && *chute_bound == landing.get<stratum_core::PersistentId>() && !landing.get<stratum_core::AccessPoint>().destination.has_value());
        CHECK_CASE("and cannot be entered", !stratum_sim::can_enter(landing, actor));

        flecs::entity another = world.create_entity(first, stratum_sim::destination_placement(chute));
        CHECK_CASE("the same arrival-only access point receives every actor that comes through", world.relocate_through(another, chute) == landing
            && stratum_sim::zone_of(another) == second);

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE_FIXTURE(WorldFixture, "entering an access point")
    {
        stratum_sim::Game game(g_test_config, g_test_manifest);
        stratum_sim::Session& session = game.session();
        stratum_sim::World& world = game.world();
        game.generation() = test_generation();

        // data/walls.zone: the character enters on cell (2, 1), facing the staircase leading down to the east
        enter_zone_file(game, "data/walls.zone");
        flecs::entity character = session.character().entity();
        flecs::entity first = session.zone();

        flecs::entity staircase_type = stratum_sim::entity_type(world.entities(), "staircase");
        flecs::entity first_down = stratum_sim::entry_point(first, staircase_type, -1);

        const stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        const stratum_sim::vector_type west(-1.0f, 0.0f, 0.0f);

        auto step = [&](const stratum_sim::vector_type& direction, stratum_sim::EventQueue& events)
        {
            session.character().set_intent(stratum_sim::Intent{direction, 1.0f, direction});
            session.advance(events);
        };

        auto interact = [&](stratum_sim::EventQueue& events)
        {
            session.character().submit(stratum_sim::Interact{});
            session.advance(events);
        };

        auto relocated = [](const stratum_sim::EventQueue& events)
        {
            return ranges::any_of(events.events(), [](const stratum_sim::Event& event) { return event.is<stratum_sim::Relocated>(); });
        };

        stratum_sim::EventQueue moved_events;
        step(east, moved_events);
        CHECK_CASE("moving into a wall with an access point on it is blocked", !relocated(moved_events) && session.zone() == first
            && ranges::any_of(moved_events.events(), [](const stratum_sim::Event& event) { return event.is<stratum_sim::Blocked>(); }));

        // A closed access point is not entered
        first_down.set<stratum_sim::Access>(stratum_sim::Access{stratum_sim::AccessState::Closed});
        stratum_sim::EventQueue closed_events;
        interact(closed_events);
        CHECK_CASE("interacting with a closed access point opens it rather than entering it", !relocated(closed_events)
            && session.zone() == first && stratum_sim::access(first_down) == stratum_sim::AccessState::Open);
        first_down.remove<stratum_sim::Access>();

        stratum_sim::EventQueue down_events;
        interact(down_events);
        flecs::entity second = session.zone();
        CHECK_CASE("interacting with an access point on the wall ahead enters the access point", relocated(down_events) && second != first
            && stratum_sim::zone_of(character) == second && level_of(second) == -2);
        CHECK_CASE("and the session plays the zone arrived in, with the character in its turn order", session.scheduler().contains(character.id()));

        stratum_sim::EventQueue up_events;
        step(west, up_events);
        interact(up_events);
        CHECK_CASE("going back up arrives at the staircase gone down", relocated(up_events) && session.zone() == first
            && zone_layout(world, first).cell_position_at(stratum_sim::placement_of(character).position()) == stratum_sim::discrete_point_type{2, 1}
            && stratum_sim::placement_of(character).direction().x() < 0.0f);

        // An access point on the floor of the character's cell, arrival only at first, then bound to the staircase leading down
        // in the second zone
        stratum_sim::DiscreteLayout& layout = zone_layout(world, first);
        flecs::entity hatch = stratum_sim::create_access_point(first, stratum_sim::entity_type(world.entities(), "portal"),
            stratum_sim::Site{stratum_sim::Relation::On, stratum_sim::FlatCellPosition{layout.floor_id(), stratum_sim::discrete_point_type{2, 1}}, {}}, stratum_core::AccessPoint{});

        session.character().submit(stratum_sim::Enter{});
        stratum_sim::EventQueue arrival_only_events;
        session.advance(arrival_only_events);
        CHECK_CASE("an access point with no destination is not entered", !relocated(arrival_only_events) && session.zone() == first);

        flecs::entity second_down = stratum_sim::entry_point(second, staircase_type, -1);
        hatch.get_mut<stratum_core::AccessPoint>().destination = stratum_core::AccessPoint::destination_type{world.make_reference(second_down).persistent_id};

        session.character().submit(stratum_sim::Enter{});
        stratum_sim::EventQueue enter_events;
        session.advance(enter_events);
        CHECK_CASE("entering the access point on the floor of the character's cell relocates the character", relocated(enter_events)
            && session.zone() == second && zone_layout(world, second).cell_position_at(stratum_sim::placement_of(character).position())
                == zone_layout(world, second).cell_position_at(stratum_sim::destination_placement(second_down).position()));

        // A denizen facing the staircase leading down, in the zone the character left
        flecs::entity denizen = world.create_entity(first, stratum_sim::destination_placement(first_down));
        stratum_sim::EventQueue denizen_events;
        stratum_sim::apply(world, stratum_sim::Command(static_cast<id_type>(denizen.id()), stratum_sim::Interact{}), denizen_events);
        CHECK_CASE("an actor other than the controlled actor does not enter an access point", !relocated(denizen_events)
            && stratum_sim::zone_of(denizen) == first);

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE_FIXTURE(WorldFixture, "interacting with an access point")
    {
        stratum_sim::Game game(g_test_config, g_test_manifest);
        stratum_sim::Session& session = game.session();

        // data/walls.zone: the character enters on cell (2, 1), facing the staircase leading down to the east
        enter_zone_file(game, "data/walls.zone");
        flecs::entity staircase = stratum_sim::entry_point(session.zone(), stratum_sim::entity_type(game.world().entities(), "staircase"), -1);

        game.generation() = test_generation();
        flecs::entity first = session.zone();
        session.character().submit(stratum_sim::Interact{});
        stratum_sim::EventQueue enter_events;
        session.advance(enter_events);
        CHECK_CASE("interacting with an access point goes through it", stratum_sim::entity_type_name(staircase) == "staircase"
            && session.zone() != first
            && ranges::any_of(enter_events.events(), [](const stratum_sim::Event& event) { return event.is<stratum_sim::Relocated>(); }));

        // A game whose advance throws is quit, so this game is used only for this case
        stratum_sim::Game failing(g_test_config, g_test_manifest);
        stratum_sim::Session& failing_session = failing.session();
        enter_zone_file(failing, "data/walls.zone");
        flecs::entity unresolved = stratum_sim::entry_point(failing_session.zone(), stratum_sim::entity_type(failing.world().entities(), "staircase"), -1);
        unresolved.get_mut<stratum_core::AccessPoint>().destination = *stratum_core::parse_access_point_destination("unregistered");

        flecs::entity unmoved = failing_session.zone();
        failing_session.character().submit(stratum_sim::Interact{});
        bool threw = false;

        try
        {
            stratum_sim::EventQueue events;
            failing_session.advance(events);
        }
        catch (const runtime_error&)
        {
            threw = true;
        }

        CHECK_CASE("interacting with an access point whose destination is not resolved is an error that passes out of advance, and nothing moves",
            threw && stratum_sim::zone_of(failing_session.character().entity()) == unmoved);

        return;
    }

    TEST_SUITE_END();

} // namespace
