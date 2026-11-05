#if !defined(STRATUM_SIM_TEST_TEST_SUPPORT_HPP)
#define STRATUM_SIM_TEST_TEST_SUPPORT_HPP
//------------------------------------------------------------------------------
// stratum_sim_test
// test_support.hpp
//------------------------------------------------------------------------------

#include "quetzal/stratum_sim/Config.hpp"
#include "quetzal/stratum_sim/DiscreteLayout.hpp"
#include "quetzal/stratum_sim/Game.hpp"
#include "quetzal/stratum_sim/Generation.hpp"
#include "quetzal/stratum_sim/IdAllocator.hpp"
#include "quetzal/stratum_sim/Layout.hpp"
#include "quetzal/stratum_sim/RandomStream.hpp"
#include "quetzal/stratum_sim/Session.hpp"
#include "quetzal/stratum_sim/World.hpp"
#include "quetzal/stratum_sim/types.hpp"
#include "quetzal/stratum_format/types.hpp"
#include <cstddef>
#include <memory>
#include <string>

namespace stratum_sim_test
{

    inline constexpr size_t test_zone_count_max = 8;

    // A game's saves are in the test executable's own output directory
    extern const quetzal::stratum_sim::Config g_test_config;

    // The block notation, with the cell view only
    extern const quetzal::stratum_format::GameManifestDocument g_test_manifest;

    extern quetzal::stratum_sim::Generation g_test_generation;

    // The seed WorldFixture seeds the gameplay random engine with, so a random choice is the same on every run
    inline constexpr quetzal::stratum_sim::RandomStream::seed_type test_random_seed = 1;

    // The state a test case that builds a world starts from: the setting definitions loaded, every setting at its default, and
    // the gameplay random engine seeded with test_random_seed. The definitions are loaded once in the process
    struct WorldFixture
    {
        WorldFixture();
    };

    // Returns the test executable's registries. A region whose spec selects "door" makes zones from data/door.zone, and
    // "staircase" from data/walls.zone, which has one staircase leading up and one leading down. "next_region" chooses a new
    // region from the region spec "staircase" for every zone, and "unregistered_generator" a generator that is not
    // registered. data/door.zone has one staircase, leading up. The region spec "dungeon" starts a game at level -1, at the
    // staircase leading up, in a zone made from data/walls.zone
    quetzal::stratum_sim::Generation test_generation();

    quetzal::stratum_sim::RegionSpec test_region_spec(const std::string& generator_selector_name);

    // Creates a worldspace from test_region_spec, named for the generator selector, and returns its top-level region's id
    quetzal::stratum_sim::PersistentId create_test_region(quetzal::stratum_sim::World& world, const std::string& generator_selector_name);

    // Returns the id of the worldspace's top-level region
    quetzal::stratum_sim::PersistentId top_level_region(const quetzal::stratum_sim::World& world, quetzal::stratum_sim::PersistentId worldspace_id);

    // Returns a ZoneSpec whose position is (0, 0, level)
    quetzal::stratum_sim::ZoneSpec test_zone_spec(quetzal::stratum_sim::level_type level);

    // Returns the position of a zone at the level, (0, 0, level)
    quetzal::stratum_sim::ZonePosition zone_position_at_level(quetzal::stratum_sim::level_type level);

    // Returns zone_position_at_level(level)
    quetzal::stratum_format::zone_position_type document_position_at_level(quetzal::stratum_sim::level_type level);

    // Returns the zone's level, the z of its position
    quetzal::stratum_sim::level_type level_of(flecs::entity zone);

    // A world with the game's entity types and a worldspace's region for the test executable's zones
    struct TestWorld : quetzal::stratum_sim::World
    {
        explicit TestWorld(quetzal::stratum_sim::IdAllocator& id_allocator, size_t zone_count_max = test_zone_count_max);

        quetzal::stratum_sim::PersistentId region_id;
    };

    // Counts the walled sides of every open cell, and how many of those edges are walled from the other side too
    struct WallCounts
    {
        size_t sides = 0;
        size_t both_sides = 0;
    };

    WallCounts wall_counts(const quetzal::stratum_sim::DiscreteLayout& layout);

    // Reads a zone from a file into a world of its own, with a predicate that crosses its door walls as though every door were open
    struct OpenZone
    {
        explicit OpenZone(const std::string& filepath);

        quetzal::stratum_sim::DiscreteLayout& layout();

        quetzal::stratum_sim::CountingIdAllocator id_allocator;
        TestWorld world;
        flecs::entity zone;
        quetzal::stratum_sim::Layout::crossable_function_type crossable;
    };

    std::unique_ptr<OpenZone> load_open(const std::string& filepath);

    // Loads a zone from a file with its doors and without denizens
    flecs::entity load_zone_unpopulated(TestWorld& world, const std::string& filepath);

    // The access point type the test cases enter zones at, and the level offset its relocator targets: a staircase leading up
    flecs::entity test_entry_type(const quetzal::stratum_sim::Session& session);

    inline constexpr quetzal::stratum_sim::level_type test_entry_offset = 1;

    // Loads the zone file into a region of its own, made from the region spec "staircase", and enters it at the test entry type
    // and level offset. Creates the game's character first if the game has none
    void enter_zone_file(quetzal::stratum_sim::Game& game, const std::string& filepath);

    // A zone's layout as a DiscreteLayout
    quetzal::stratum_sim::DiscreteLayout& zone_layout(quetzal::stratum_sim::World& world, flecs::entity zone);

} // namespace stratum_sim_test

#endif // STRATUM_SIM_TEST_TEST_SUPPORT_HPP
