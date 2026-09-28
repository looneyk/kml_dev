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
#include <cstdlib>
#include <exception>
#include <format>
#include <string>

namespace sim = quetzal::stratum_sim;
namespace view = quetzal::stratum_view;

namespace
{

    // Temporary: a zone loaded is entered at its lowest-ordered staircase leading up
    static constexpr auto entry_access_type = "staircase";
    static constexpr sim::level_type entry_level_offset = 1;

    // Temporary: a zone loaded is the first level down if its file states no level
    static constexpr sim::level_type starting_level = -1;

} // namespace

//------------------------------------------------------------------------------
StratumWorld::StratumWorld() :
    m_stratum_game(),
    m_session(m_stratum_game.session()),
    m_message()
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
    BIND_ENUM_CONSTANT(PlaceTypeStairsUp);
    BIND_ENUM_CONSTANT(PlaceTypeStairsDown);
    BIND_ENUM_CONSTANT(PlaceTypeDoorOpen);
    BIND_ENUM_CONSTANT(OccupantNone);
    BIND_ENUM_CONSTANT(OccupantPlayer);
    BIND_ENUM_CONSTANT(OccupantOther);
    godot::ClassDB::bind_method(godot::D_METHOD("step", "di", "dj"), &StratumWorld::step);
    godot::ClassDB::bind_method(godot::D_METHOD("interact"), &StratumWorld::interact);
    godot::ClassDB::bind_method(godot::D_METHOD("enter"), &StratumWorld::enter);
    godot::ClassDB::bind_method(godot::D_METHOD("turn"), &StratumWorld::turn);
    godot::ClassDB::bind_method(godot::D_METHOD("message"), &StratumWorld::message);

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
        m_session.load(m_stratum_game.starting_region(), std::string(filepath.utf8().get_data()), sim::access_type(m_stratum_game.world().entities(), entry_access_type),
            entry_level_offset);
    }
    catch (const std::exception& e)
    {
        godot::UtilityFunctions::push_error("unable to load zone: ", godot::String::utf8(e.what()));
        return false;
    }

    if (!m_session.zone().has<sim::ZoneLevel>())
    {
        m_session.zone().set<sim::ZoneLevel>(sim::ZoneLevel {starting_level});
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
            const view::Place& place = cells.at({i, j});
            bool open_door = place.type == view::PlaceType::Door && view::any(place.state & view::State::Open);
            codes[static_cast<int64_t>(j) * extent.x() + i] = open_door ? static_cast<uint8_t>(PlaceTypeDoorOpen) : static_cast<uint8_t>(place.type);
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

    godot::Vector2i player_cell = cell();

    for (view::coord_type j = 0; j < extent.y(); ++j)
    {
        for (view::coord_type i = 0; i < extent.x(); ++i)
        {
            Occupant occupant = !cells.at({i, j}).actor ? OccupantNone : godot::Vector2i(i, j) == player_cell ? OccupantPlayer : OccupantOther;
            flags[static_cast<int64_t>(j) * extent.x() + i] = static_cast<uint8_t>(occupant);
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

    // This view does not show the facing direction, so a move also turns toward the way it goes, and interact acts ahead
    sim::vector_type direction(static_cast<sim::value_type>(di), static_cast<sim::value_type>(dj), sim::value_type(0));
    m_session.player().set_intent(sim::Intent {direction, sim::value_type(1), direction});
    advance();

    return cell();
}

//------------------------------------------------------------------------------
void StratumWorld::interact()
{
    if (!m_session.loaded())
    {
        return;
    }

    m_session.player().submit(sim::Interact {});
    advance();

    return;
}

//------------------------------------------------------------------------------
godot::Vector2i StratumWorld::enter()
{
    if (!m_session.loaded())
    {
        return {};
    }

    m_session.player().submit(sim::Enter {});
    advance();

    return cell();
}

//------------------------------------------------------------------------------
int64_t StratumWorld::turn() const
{
    return static_cast<int64_t>(m_session.scheduler().elapsed());
}

//------------------------------------------------------------------------------
godot::String StratumWorld::message() const
{
    return godot::String::utf8(m_message.c_str());
}

//------------------------------------------------------------------------------
void StratumWorld::advance()
{
    sim::EventQueue events;
    m_session.advance(events);

    m_message.clear();

    for (const sim::Event& event : events.events())
    {
        if (const sim::Blocked* pblocked = event.try_get<sim::Blocked>())
        {
            flecs::entity relocator = pblocked->blocking_wall_id != quetzal::nullid
                ? sim::find_relocator(m_session.zone(), sim::WallAddress {pblocked->blocking_wall_id}) : flecs::entity();
            std::string name = relocator ? sim::access_type_name(relocator) : std::string();

            m_message = relocator ? std::format("There is a {} before you.", name.empty() ? "way" : name) : std::string();
        }
        else if (const sim::Interacted* pinteracted = event.try_get<sim::Interacted>())
        {
            if (pinteracted->interaction == sim::Interaction::NoDestination)
            {
                std::string name = sim::access_type_name(m_session.world().entities().entity(pinteracted->acted_on_id));
                m_message = std::format("You try the {}, but it leads nowhere.", name.empty() ? "way" : name);
                continue;
            }

            m_message = pinteracted->interaction == sim::Interaction::Opened ? "The door opens."
                : pinteracted->interaction == sim::Interaction::Closed ? "The door shuts."
                : "Nothing happens.";
        }
        else if (event.is<sim::Relocated>())
        {
            // A level is shown as its distance from the surface
            m_message = std::format("You arrive on level {}.", std::abs(m_session.zone().get<sim::ZoneLevel>().level));
        }
    }

    return;
}
