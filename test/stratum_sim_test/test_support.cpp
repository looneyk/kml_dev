//------------------------------------------------------------------------------
// stratum_sim_test
// test_support.cpp
//------------------------------------------------------------------------------

#include "test_support.hpp"
#include "quetzal/settings/Settings.hpp"
#include "quetzal/stratum_format/Document.hpp"
#include "quetzal/stratum_format/read_zone.hpp"
#include "quetzal/stratum_sim/attachments.hpp"
#include "quetzal/stratum_sim/load_layout.hpp"
#include "quetzal/stratum_sim/PersistentId.hpp"
#include "quetzal/stratum_sim/setting_definitions.hpp"

using namespace std;
using namespace quetzal;

namespace
{

    //--------------------------------------------------------------------------
    // Returns a generator that reads the zone file, at the requested level if there is one, and without the unpopulated
    // flag, which a generator does not set
    stratum_sim::Generation::generator_type file_generator(const string& filepath)
    {
        return [filepath](const stratum_sim::ZoneSpec& zone_spec)
        {
            stratum_format::Document document = stratum_format::read_zone(filepath);
            document.level = zone_spec.value<stratum_sim::level_type>(stratum_sim::level_key);
            document.is_unpopulated = false;
            return document;
        };
    }

    //--------------------------------------------------------------------------
    // Returns a generator selector that chooses the one generator for every zone
    stratum_sim::Generation::generator_selector_type sole_generator_selector(const string& generator_name)
    {
        return [generator_name](const stratum_sim::World&, flecs::entity, const stratum_sim::ZoneSpec& zone_spec)
        {
            return stratum_sim::Generation::GeneratorSelection {generator_name, {}, {}, zone_spec};
        };
    }

} // namespace

const stratum_sim::Config stratum_sim_test::g_test_config {"out/saves", "out/working", test_zone_count_max};

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

    settings::store().reset(settings::SettingScope::User);
    settings::store().reset(settings::SettingScope::Game);
    stratum_sim::gameplay_random().seed(test_random_seed);
}

//------------------------------------------------------------------------------
stratum_sim::Generation stratum_sim_test::test_generation()
{
    stratum_sim::Generation generation;

    generation.m_generators["door"] = file_generator("data/door.zone");
    generation.m_generators["staircase"] = file_generator("data/walls.zone");
    generation.m_generator_selectors["door"] = sole_generator_selector("door");
    generation.m_generator_selectors["staircase"] = sole_generator_selector("staircase");
    generation.m_generator_selectors["unregistered_generator"] = sole_generator_selector("unregistered");
    generation.m_generator_selectors["next_region"] = [](const stratum_sim::World&, flecs::entity, const stratum_sim::ZoneSpec& zone_spec)
    {
        return stratum_sim::Generation::GeneratorSelection {{}, {}, "staircase", zone_spec};
    };
    generation.m_region_specs["staircase"] = stratum_sim::RegionSpec {{string(stratum_sim::generator_selector_key), "staircase"}};
    generation.m_region_specs["door"] = stratum_sim::RegionSpec {{string(stratum_sim::generator_selector_key), "door"}};

    return generation;
}

//------------------------------------------------------------------------------
stratum_sim::RegionSpec stratum_sim_test::test_region_spec(const string& generator_selector_name)
{
    return stratum_sim::RegionSpec {{string(stratum_sim::generator_selector_key), generator_selector_name}};
}

//------------------------------------------------------------------------------
stratum_sim::ZoneSpec stratum_sim_test::test_zone_spec(stratum_sim::level_type level)
{
    return stratum_sim::ZoneSpec {{string(stratum_sim::level_key), to_string(level)}};
}

//------------------------------------------------------------------------------
stratum_sim_test::TestWorld::TestWorld(stratum_sim::IdAllocator& id_allocator, size_t zone_count_max) :
    World(id_allocator, g_test_generation, zone_count_max),
    region(create_region(test_region_spec("staircase")))
{
    stratum_sim::register_game_types(*this);
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
            stratum_sim::discrete_point_type cell_position {i, j};

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
    id_allocator(stratum_sim::PersistentId {}),
    world(id_allocator),
    zone(world.create_zone(world.region, world.next_id()))
{
    stratum_sim::load_layout(zone, stratum_format::read_zone(filepath));
    crossable = [open_zone = zone](id_type wall_id) { return static_cast<bool>(stratum_sim::way_through(open_zone, wall_id)); };
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
    flecs::entity zone = world.create_zone(world.region, world.next_id());
    stratum_sim::load_layout(zone, stratum_format::read_zone(filepath));
    return zone;
}

//------------------------------------------------------------------------------
flecs::entity stratum_sim_test::test_entry_type(const stratum_sim::Session& session)
{
    return stratum_sim::access_type(session.world().entities(), "staircase");
}

//------------------------------------------------------------------------------
void stratum_sim_test::enter_zone_file(stratum_sim::Game& game, const string& filepath)
{
    if (!game.character().entity())
    {
        game.character().create(stratum_sim::CharacterSpec {});
    }

    flecs::entity region = game.world().create_region(test_region_spec("staircase"));
    stratum_sim::Session& session = game.session();
    session.enter(game.world().load_zone(region, filepath), test_entry_type(session), test_entry_offset);

    return;
}

//------------------------------------------------------------------------------
stratum_sim::DiscreteLayout& stratum_sim_test::zone_layout(stratum_sim::World& world, flecs::entity zone)
{
    return stratum_sim::layout_as<stratum_sim::DiscreteLayout>(world.layout(zone));
}
