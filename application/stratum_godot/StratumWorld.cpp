//------------------------------------------------------------------------------
// stratum_godot
// StratumWorld.cpp
//------------------------------------------------------------------------------

#include "StratumWorld.hpp"
#include "quetzal/stratum_sim/attachments.hpp"
#include "quetzal/stratum_sim/DiscreteLayout.hpp"
#include "quetzal/stratum_sim/commands.hpp"
#include "quetzal/stratum_sim/Site.hpp"
#include "quetzal/stratum_view/ascii.hpp"
#include "quetzal/stratum_view/cells.hpp"
#include <godot_cpp/variant/utility_functions.hpp>
#include <exception>
#include <string>

namespace sim = quetzal::stratum_sim;
namespace view = quetzal::stratum_view;

namespace
{

    // Temporary: a zone loaded is entered at its lowest-ordered staircase leading up
    static constexpr auto entry_access_type = "up";

} // namespace

//------------------------------------------------------------------------------
StratumWorld::StratumWorld() :
    m_stratum_game(),
    m_session(m_stratum_game.session())
{
}

//------------------------------------------------------------------------------
void StratumWorld::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("default_zone_path"), &StratumWorld::default_zone_path);
    godot::ClassDB::bind_method(godot::D_METHOD("load_zone", "filepath"), &StratumWorld::load_zone);
    godot::ClassDB::bind_method(godot::D_METHOD("extent"), &StratumWorld::extent);
    godot::ClassDB::bind_method(godot::D_METHOD("cell"), &StratumWorld::cell);
    godot::ClassDB::bind_method(godot::D_METHOD("render"), &StratumWorld::render);
    godot::ClassDB::bind_method(godot::D_METHOD("cells"), &StratumWorld::cells);
    godot::ClassDB::bind_method(godot::D_METHOD("actors"), &StratumWorld::actors);

    BIND_ENUM_CONSTANT(PlaceTypeNone);
    BIND_ENUM_CONSTANT(PlaceTypeSolid);
    BIND_ENUM_CONSTANT(PlaceTypeFloor);
    BIND_ENUM_CONSTANT(PlaceTypeWall);
    BIND_ENUM_CONSTANT(PlaceTypeDoor);
    godot::ClassDB::bind_method(godot::D_METHOD("step", "di", "dj"), &StratumWorld::step);
    godot::ClassDB::bind_method(godot::D_METHOD("turn"), &StratumWorld::turn);

    return;
}

//------------------------------------------------------------------------------
godot::String StratumWorld::default_zone_path() const
{
    return godot::String::utf8(KML_DEV_ASSETS_ROOT "/level/dungeon.level");
}

//------------------------------------------------------------------------------
bool StratumWorld::load_zone(const godot::String& filepath)
{
    // Exceptions must not cross into Godot. A failed load leaves the player in the zone it was in
    try
    {
        m_session.load(std::string(filepath.utf8().get_data()), sim::access_type(m_stratum_game.world().entities(), entry_access_type));
    }
    catch (const std::exception& e)
    {
        godot::UtilityFunctions::push_error("unable to load zone: ", godot::String::utf8(e.what()));
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
godot::Vector2i StratumWorld::extent() const
{
    if (!m_session.loaded())
    {
        return {};
    }

    const sim::GridMapping& grid = sim::layout_as<sim::DiscreteLayout>(m_session.layout()).grid();

    return godot::Vector2i(static_cast<int32_t>(grid.extent.x()), static_cast<int32_t>(grid.extent.y()));
}

//------------------------------------------------------------------------------
godot::Vector2i StratumWorld::cell() const
{
    if (!m_session.loaded())
    {
        return {};
    }

    const sim::placement_type& placement = sim::placement_of(m_session.player().entity());
    sim::discrete_point_type cell_position = sim::layout_as<sim::DiscreteLayout>(m_session.layout()).cell_position_at(placement.position());

    return godot::Vector2i(cell_position.i(), cell_position.j());
}

//------------------------------------------------------------------------------
godot::String StratumWorld::render() const
{
    if (!m_session.loaded())
    {
        return {};
    }

    return godot::String::utf8(view::render(m_session.world(), m_session.zone(), m_session.player().entity()).to_string().c_str());
}

//------------------------------------------------------------------------------
godot::PackedByteArray StratumWorld::cells() const
{
    godot::PackedByteArray codes;

    if (!m_session.loaded())
    {
        return codes;
    }

    view::Cells cells = view::cells(m_session.world(), m_session.zone(), m_session.player().entity());

    const view::vector_type& extent = cells.extent();
    codes.resize(static_cast<int64_t>(extent.x()) * extent.y());

    for (view::coord_type j = 0; j < extent.y(); ++j)
    {
        for (view::coord_type i = 0; i < extent.x(); ++i)
        {
            codes[static_cast<int64_t>(j) * extent.x() + i] = static_cast<uint8_t>(cells.at({i, j}).type);
        }
    }

    return codes;
}

//------------------------------------------------------------------------------
godot::PackedByteArray StratumWorld::actors() const
{
    godot::PackedByteArray flags;

    if (!m_session.loaded())
    {
        return flags;
    }

    view::Cells cells = view::cells(m_session.world(), m_session.zone(), m_session.player().entity());

    const view::vector_type& extent = cells.extent();
    flags.resize(static_cast<int64_t>(extent.x()) * extent.y());

    for (view::coord_type j = 0; j < extent.y(); ++j)
    {
        for (view::coord_type i = 0; i < extent.x(); ++i)
        {
            flags[static_cast<int64_t>(j) * extent.x() + i] = cells.at({i, j}).actor ? 1 : 0;
        }
    }

    return flags;
}

//------------------------------------------------------------------------------
godot::Vector2i StratumWorld::step(int32_t di, int32_t dj)
{
    if (!m_session.loaded())
    {
        return {};
    }

    m_session.player().set_intent(sim::Intent {
        sim::vector_type(static_cast<sim::value_type>(di), static_cast<sim::value_type>(dj), sim::value_type(0)),
        sim::value_type(1), sim::vector_type()});

    sim::EventQueue events;
    m_session.advance(events);

    return cell();
}

//------------------------------------------------------------------------------
int64_t StratumWorld::turn() const
{
    return static_cast<int64_t>(m_session.scheduler().elapsed());
}
