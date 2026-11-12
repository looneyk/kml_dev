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
#include "quetzal/stratum_format/RegionExtent.hpp"
#include "quetzal/stratum_format/types.hpp"
#include "quetzal/math/Interval.hpp"
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace stratum_sim_test
{

    inline constexpr size_t test_zone_count_max = 8;

    // A game's saves are in the test executable's own output directory
    extern const quetzal::stratum_sim::Config g_test_config;

    // The block notation, with the cell view only
    extern const quetzal::stratum_format::GameManifestDocument g_test_manifest;

    extern quetzal::stratum_sim::Generation g_test_generation;

    // The seed WorldFixture seeds the gameplay random engine with, so a random choice is the same on every run
    inline constexpr quetzal::stratum_sim::seed_type test_random_seed = 1;

    // The seed of a worldspace that a test case creates
    inline constexpr quetzal::stratum_sim::seed_type test_worldspace_seed = 2;

    // The state a test case that builds a world starts from: the setting definitions loaded, every setting at its default, and
    // the gameplay random engine seeded with test_random_seed. The definitions are loaded once in the process
    struct WorldFixture
    {
        WorldFixture();
    };

    // The ZoneSpec key of the zone file that the test executable's zone generator reads
    inline constexpr std::string_view zone_file_key = "parameters.zone_file";

    // Returns the test executable's registries. The game's zone generator reads the ZoneSpec's zone file: data/door.zone
    // for the region spec "door", which has one staircase, leading up, and data/walls.zone for "staircase", which has one
    // staircase leading up and one leading down. The region spec "dungeon" starts a game at level -1, at the staircase
    // leading up, in a zone made from data/walls.zone
    quetzal::stratum_sim::Generation test_generation();

    using range_type = quetzal::math::Interval<quetzal::stratum_sim::coord_type>;

    // Returns a ranges extent of one box
    quetzal::stratum_format::RegionExtent box_extent(const range_type& x, const range_type& y, const range_type& z);

    // Returns a region spec whose zones are made from the test zone's file, "door" or "staircase", with an extent of
    // 0 on x and y and -10 .. 10 on z
    quetzal::stratum_sim::RegionSpec test_region_spec(const std::string& test_zone);

    // The number of positions in the extent of test_region_spec, each of which has a zone
    inline constexpr size_t test_extent_zone_count = 21;

    // Creates a worldspace from test_region_spec, named for the test zone, and returns its top-level region's id
    quetzal::stratum_sim::PersistentId create_test_region(quetzal::stratum_sim::World& world, const std::string& test_zone);

    // Returns the zone file that the region spec's zones are made from
    std::optional<std::string> zone_file_of(const quetzal::stratum_sim::RegionSpec& region_spec);

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

        quetzal::stratum_sim::PersistentId worldspace_id;
        quetzal::stratum_sim::PersistentId region_id;
    };

    // Returns the number of the world's loaded zones that are not empty
    size_t filled_zone_count(const quetzal::stratum_sim::World& world);

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

    // Fills the empty zone at the position in the worldspace from a zone file, with its doors and without denizens, and returns it
    flecs::entity load_zone_unpopulated_at(quetzal::stratum_sim::World& world, quetzal::stratum_sim::PersistentId worldspace_id,
        const quetzal::stratum_sim::ZonePosition& position, const std::string& filepath);

    // The access point type the test cases enter zones at, and the level offset its relocator targets: a staircase leading up
    flecs::entity test_entry_type(const quetzal::stratum_sim::Session& session);

    inline constexpr quetzal::stratum_sim::level_type test_entry_offset = 1;

    // Loads the zone file at level -1 of a worldspace of its own, made from the region spec "staircase", and enters it at the test
    // entry type and level offset. Creates the game's character first if the game has none
    void enter_zone_file(quetzal::stratum_sim::Game& game, const std::string& filepath);

    // A zone's layout as a DiscreteLayout
    quetzal::stratum_sim::DiscreteLayout& zone_layout(quetzal::stratum_sim::World& world, flecs::entity zone);

} // namespace stratum_sim_test

#endif // STRATUM_SIM_TEST_TEST_SUPPORT_HPP
