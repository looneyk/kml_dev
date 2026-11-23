//------------------------------------------------------------------------------
// stratum_sim_test
// test_support.cpp
//------------------------------------------------------------------------------

#include "test_support.hpp"
#include "quetzal/settings/Settings.hpp"
#include "quetzal/stratum_core/PersistentId.hpp"
#include "quetzal/stratum_core/spec_keys.hpp"
#include "quetzal/stratum_format/ZoneDocument.hpp"
#include "quetzal/stratum_format/read_zone.hpp"
#include "quetzal/stratum_sim/access_points.hpp"
#include "quetzal/stratum_sim/attachments.hpp"
#include "quetzal/stratum_sim/load_layout.hpp"
#include "quetzal/stratum_sim/setting_definitions.hpp"
#include "quetzal/stratum_sim/specs.hpp"
#include <algorithm>
#include <format>
#include <map>
#include <optional>

using namespace std;
using namespace quetzal;

namespace
{

    //--------------------------------------------------------------------------
    // Returns the ZoneDocument of the spec's zone file, at the requested position, and without the unpopulated flag, which a
    // zone generator does not set
    stratum_format::ZoneDocument generate_from_zone_file(const stratum_core::ZoneSpec& zone_spec)
    {
        stratum_format::ZoneDocument document = stratum_format::read_zone(zone_spec.store.value(stratum_sim_test::zone_file_key).value());
        document.position = zone_spec.position();
        document.is_unpopulated = false;
        return document;
    }

} // namespace

const stratum_sim::Config stratum_sim_test::g_test_config{"out/saves", "out/working", test_zone_count_max};

const stratum_format::GameManifestDocument stratum_sim_test::g_test_manifest{.notation = stratum_core::Notation::Block, .views = {"cell"},
    .view_menu = false, .default_view = "cell", .settings = {{"world.region_size.x", "7"}, {"world.start_zone", "0,0,-1"}}};

stratum_sim::Generation stratum_sim_test::g_test_generation = test_generation();

//------------------------------------------------------------------------------
stratum_sim_test::WorldFixture::WorldFixture()
{
    static bool definitions_loaded = false;
    if (!definitions_loaded)
    {
        stratum_sim::load_setting_definitions("../../assets", settings::store());
        definitions_loaded = true;
    }

    settings::store().reset(stratum_sim::user_setting_scope);
    settings::store().reset(stratum_sim::game_setting_scope);
    stratum_sim::gameplay_random().seed(test_random_seed);
}

//------------------------------------------------------------------------------
stratum_sim::Generation stratum_sim_test::test_generation()
{
    stratum_sim::Generation generation;

    generation.m_zone_generators[stratum_core::Notation::Block] = generate_from_zone_file;
    generation.m_zone_generators[stratum_core::Notation::Wall] = generate_from_zone_file;
    generation.m_zone_generator = generate_from_zone_file;
    generation.m_region_specs["staircase"] = test_region_spec("staircase");
    generation.m_region_specs["door"] = test_region_spec("door");
    generation.m_region_specs["dungeon"] = test_region_spec("staircase");
    generation.m_world_definition.region_spec_trees["dungeon"] = {{.region_spec = stratum_sim::RegionSpec{.name = "dungeon", .region_spec = "dungeon"}}};
    generation.m_world_definition.starting_region = "dungeon";

    return generation;
}

//------------------------------------------------------------------------------
stratum_core::RegionExtent stratum_sim_test::box_extent(const range_type& x, const range_type& y, const range_type& z)
{
    return stratum_core::RegionExtent{.arrangement = stratum_core::RegionExtent::Arrangement::Boxes, .boxes = {{x, y, z}}, .positions = {}};
}

//------------------------------------------------------------------------------
stratum_sim::RegionSpec stratum_sim_test::test_region_spec(const string& test_zone)
{
    static const map<string, string> zone_files{{"door", "data/door.zone"}, {"staircase", "data/walls.zone"}};

    stratum_sim::RegionSpec region_spec;
    region_spec.extent = box_extent(range_type(0, 0), range_type(0, 0), range_type(-10, 10));
    region_spec.store.insert(format("{}{}", stratum_core::zone_prefix, zone_file_key), zone_files.at(test_zone));

    // The zone at level 0, whose zone counter is 10
    region_spec.store.insert(string(stratum_core::entry_zone_key), "&region_name .. '_10'");
    return region_spec;
}

//------------------------------------------------------------------------------
stratum_sim::RegionSpec stratum_sim_test::named_region_spec(stratum_sim::RegionSpec region_spec, const string& name)
{
    region_spec.name = name;
    return region_spec;
}

//------------------------------------------------------------------------------
stratum_core::PersistentId stratum_sim_test::create_test_region(stratum_sim::World& world, const string& test_zone)
{
    return top_level_region(world, world.create_worldspace({{.region_spec = named_region_spec(test_region_spec(test_zone), test_zone)}}, test_worldspace_seed));
}

//------------------------------------------------------------------------------
optional<string> stratum_sim_test::zone_file_of(const stratum_sim::RegionSpec& region_spec)
{
    return region_spec.store.value(format("{}{}", stratum_core::zone_prefix, zone_file_key));
}

//------------------------------------------------------------------------------
stratum_core::PersistentId stratum_sim_test::top_level_region(const stratum_sim::World& world, stratum_core::PersistentId worldspace_id)
{
    return world.worldspace(worldspace_id).regions.root().region_id;
}

//------------------------------------------------------------------------------
stratum_core::ZoneSpec stratum_sim_test::test_zone_spec(stratum_sim::level_type level)
{
    stratum_core::ZoneSpec zone_spec;
    zone_spec.set_position(stratum_core::zone_position_type{0, 0, level});
    return zone_spec;
}

//------------------------------------------------------------------------------
stratum_core::zone_position_type stratum_sim_test::zone_position_at_level(stratum_sim::level_type level)
{
    return {0, 0, level};
}

//------------------------------------------------------------------------------
stratum_sim::level_type stratum_sim_test::level_of(flecs::entity zone)
{
    return zone.get<stratum_core::zone_position_type>().z();
}

//------------------------------------------------------------------------------
stratum_sim_test::TestWorld::TestWorld(stratum_sim::IdAllocator& id_allocator, size_t zone_count_max) :
    World(id_allocator, g_test_generation, zone_count_max, g_test_config.pattern_limits),
    worldspace_id(create_worldspace({{.region_spec = named_region_spec(test_region_spec("staircase"), "staircase")}}, test_worldspace_seed)),
    region_id(top_level_region(*this, worldspace_id))
{
    stratum_sim::register_game_types(*this);
}

//------------------------------------------------------------------------------
size_t stratum_sim_test::filled_zone_count(const stratum_sim::World& world)
{
    return static_cast<size_t>(ranges::count_if(world.zones(), [](flecs::entity zone) { return !stratum_sim::empty_zone(zone); }));
}

//------------------------------------------------------------------------------
stratum_sim_test::WallCounts stratum_sim_test::wall_counts(const stratum_sim::DiscreteLayout& layout)
{
    WallCounts counts;
    const stratum_sim::GridMapping& grid = layout.grid();

    for (stratum_sim::coord_type j = 0; j < grid.extent.y(); ++j)
    {
        for (stratum_sim::coord_type i = 0; i < grid.extent.x(); ++i)
        {
            stratum_sim::discrete_point_type cell_position{i, j};

            for (const stratum_sim::discrete_vector_type& direction : stratum_sim::cardinal_directions)
            {
                if (layout.wall_toward(cell_position, direction) == nullid)
                {
                    continue;
                }

                ++counts.sides;

                stratum_sim::discrete_point_type beyond = cell_position + direction;
                counts.both_sides += grid.contains(beyond) && layout.wall_toward(beyond, -direction) != nullid ? 1 : 0;
            }
        }
    }

    return counts;
}

//------------------------------------------------------------------------------
stratum_sim_test::OpenZone::OpenZone(const string& filepath) :
    id_allocator(stratum_core::PersistentId{}),
    world(id_allocator),
    zone(world.create_zone(world.region_id, world.next_id()))
{
    stratum_sim::load_layout(zone, stratum_format::read_zone(filepath));
    crossable = [open_zone = zone](id_type wall_id) { return static_cast<bool>(stratum_sim::find_access_attachment(open_zone, wall_id)); };
}

//------------------------------------------------------------------------------
stratum_sim::DiscreteLayout& stratum_sim_test::OpenZone::layout()
{
    return stratum_sim::layout_as<stratum_sim::DiscreteLayout>(world.layout(zone));
}

//------------------------------------------------------------------------------
unique_ptr<stratum_sim_test::OpenZone> stratum_sim_test::load_open(const string& filepath)
{
    return make_unique<OpenZone>(filepath);
}

//------------------------------------------------------------------------------
flecs::entity stratum_sim_test::load_zone_unpopulated(TestWorld& world, const string& filepath)
{
    flecs::entity zone = world.create_zone(world.region_id, world.next_id());
    stratum_sim::load_layout(zone, stratum_format::read_zone(filepath));
    return zone;
}

//------------------------------------------------------------------------------
flecs::entity stratum_sim_test::load_zone_unpopulated_at(stratum_sim::World& world, stratum_core::PersistentId worldspace_id,
    const stratum_core::zone_position_type& position, const string& filepath)
{
    flecs::entity zone = world.entity(world.zone_ids(worldspace_id).at(position));
    stratum_format::ZoneDocument document = stratum_format::read_zone(filepath);
    document.position = position;
    stratum_sim::load_layout(zone, document);
    return zone;
}

//------------------------------------------------------------------------------
flecs::entity stratum_sim_test::test_entry_type(const stratum_sim::Session& session)
{
    return stratum_sim::entity_type(session.world().entities(), "staircase");
}

//------------------------------------------------------------------------------
void stratum_sim_test::enter_zone_file(stratum_sim::Game& game, const string& filepath)
{
    if (!game.character().entity())
    {
        game.character().create(stratum_sim::CharacterSpec{});
    }

    stratum_core::PersistentId worldspace_id = game.world().create_worldspace({{.region_spec = named_region_spec(test_region_spec("staircase"), "staircase")}}, test_worldspace_seed);
    stratum_sim::Session& session = game.session();
    session.enter(game.world().load_template(worldspace_id, zone_position_at_level(-1), filepath, stratum_core::ZoneSpec{}), test_entry_type(session), test_entry_offset);

    return;
}

//------------------------------------------------------------------------------
stratum_sim::DiscreteLayout& stratum_sim_test::zone_layout(stratum_sim::World& world, flecs::entity zone)
{
    return stratum_sim::layout_as<stratum_sim::DiscreteLayout>(world.layout(zone));
}
