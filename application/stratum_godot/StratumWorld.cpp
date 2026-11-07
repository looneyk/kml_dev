//------------------------------------------------------------------------------
// stratum_godot
// StratumWorld.cpp
//------------------------------------------------------------------------------

#include "StratumWorld.hpp"
#include "register_types.hpp"
#include "quetzal/settings/Settings.hpp"
#include "quetzal/stratum_gen/generators.hpp"
#include "quetzal/stratum_sim/attachments.hpp"
#include "quetzal/stratum_sim/DiscreteLayout.hpp"
#include "quetzal/stratum_sim/commands.hpp"
#include "quetzal/stratum_sim/Name.hpp"
#include "quetzal/stratum_sim/RegionSpec.hpp"
#include "quetzal/stratum_sim/Site.hpp"
#include "quetzal/stratum_sim/specs.hpp"
#include "quetzal/stratum_view/ascii.hpp"
#include "quetzal/stratum_view/cells.hpp"
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <cassert>

namespace sim = quetzal::stratum_sim;
namespace view = quetzal::stratum_view;

namespace
{

    // Temporary, until the world definition file has the starting region spec
    constexpr std::string_view temporary_initial_region_spec_name = "surface";

    //--------------------------------------------------------------------------
    // Temporary, until zone patterns set every zone's extent: the worldspace's zone size is the game's
    void add_temporary_zone_size(sim::RegionSpec& region_spec)
    {
        region_spec.keys.insert(std::format("{}{}", sim::worldspace_prefix, sim::zone_size_x_key),
            std::format("{}", quetzal::settings::store().value<int64_t>("world.zone_size.x")));
        region_spec.keys.insert(std::format("{}{}", sim::worldspace_prefix, sim::zone_size_y_key),
            std::format("{}", quetzal::settings::store().value<int64_t>("world.zone_size.y")));
        return;
    }

    //--------------------------------------------------------------------------
    // Temporary, until a new game creates its character: a character with only a name
    sim::CharacterSpec temporary_character_spec()
    {
        sim::CharacterSpec character_spec;
        character_spec.name = "Adventurer";
        return character_spec;
    }

    //--------------------------------------------------------------------------
    // Temporary, until zone patterns set every zone's extent: a zone without one has the game's zone size
    quetzal::stratum_gen::ZoneRequest make_zone_request(const sim::ZoneSpec& zone_spec)
    {
        std::optional<quetzal::stratum_gen::seed_type> seed = zone_spec.keys.value<quetzal::stratum_gen::seed_type>(sim::seed_key);
        assert(seed.has_value());
        const sim::ZonePosition position = zone_spec.position();

        quetzal::KeyValueStore parameters = sim::select_prefixed(zone_spec.keys, sim::parameters_prefix);
        if (!parameters.contains("extent"))
        {
            parameters.insert("extent", std::format("{},{}", quetzal::settings::store().value<int64_t>("world.zone_size.x"),
                quetzal::settings::store().value<int64_t>("world.zone_size.y")));
        }

        return quetzal::stratum_gen::ZoneRequest {zone_spec.keys.value_or(sim::name_key, std::string()),
            quetzal::stratum_gen::zone_position_type {position.x, position.y, position.z}, *seed, parameters};
    }

    //--------------------------------------------------------------------------
    // Empty for an explicit save
    const char* get_save_type_label(sim::SaveType save_type)
    {
        switch (save_type)
        {
        case sim::SaveType::Explicit:
            return "";
        case sim::SaveType::Auto:
            return "Autosave";
        case sim::SaveType::Quick:
            return "Quicksave";
        }

        assert(false);
        return "";
    }

} // namespace

//------------------------------------------------------------------------------
std::filesystem::path project_directory()
{
    godot::CharString utf8 = godot::ProjectSettings::get_singleton()->globalize_path("res://").utf8();
    return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(utf8.get_data())));
}

//------------------------------------------------------------------------------
StratumWorld::StratumWorld() :
    m_game(sim::load_config(project_directory()), game_manifest()),
    m_session(m_game.session()),
    m_message(),
    m_view_slot(static_cast<size_t>(std::ranges::find(m_game.manifest().views, m_game.manifest().default_view) - m_game.manifest().views.begin())),
    m_notation(m_game.manifest().notation)
{
    const auto generators = quetzal::stratum_gen::generators();
    for (const auto& notation : {quetzal::stratum_format::Notation::Block, quetzal::stratum_format::Notation::Wall})
    {
        const quetzal::stratum_gen::generator_function_type& generate = generators.at(std::string(quetzal::stratum_gen::generator_name(notation)));
        m_game.generation().m_zone_generators[notation] = [generate](const sim::ZoneSpec& zone_spec)
        {
            return generate(make_zone_request(zone_spec));
        };
    }

    m_game.generation().m_region_specs = sim::load_region_specs(m_game.config().assets_directory);
    m_game.generation().m_initial_region_spec_name = temporary_initial_region_spec_name;
    add_temporary_zone_size(m_game.generation().m_region_specs.at(m_game.generation().m_initial_region_spec_name));
}

//------------------------------------------------------------------------------
void StratumWorld::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("create_game"), &StratumWorld::create_game);
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
    godot::ClassDB::bind_method(godot::D_METHOD("has_game"), &StratumWorld::has_game);
    godot::ClassDB::bind_method(godot::D_METHOD("game_over"), &StratumWorld::game_over);
    godot::ClassDB::bind_method(godot::D_METHOD("menu_open"), &StratumWorld::menu_open);
    godot::ClassDB::bind_method(godot::D_METHOD("menu_close"), &StratumWorld::menu_close);
    godot::ClassDB::bind_method(godot::D_METHOD("quit_game"), &StratumWorld::quit_game);
    godot::ClassDB::bind_method(godot::D_METHOD("save_game"), &StratumWorld::save_game);
    godot::ClassDB::bind_method(godot::D_METHOD("saved_games"), &StratumWorld::saved_games);
    godot::ClassDB::bind_method(godot::D_METHOD("open_game", "save_name"), &StratumWorld::open_game);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_game", "save_name"), &StratumWorld::remove_game);
    godot::ClassDB::bind_method(godot::D_METHOD("view"), &StratumWorld::view);
    godot::ClassDB::bind_method(godot::D_METHOD("cycle_view"), &StratumWorld::cycle_view);
    godot::ClassDB::bind_method(godot::D_METHOD("notation"), &StratumWorld::notation);
    godot::ClassDB::bind_method(godot::D_METHOD("cycle_notation"), &StratumWorld::cycle_notation);

    return;
}

//------------------------------------------------------------------------------
bool StratumWorld::create_game()
{
    // Exceptions must not cross into Godot. A failed creation leaves no game in progress
    try
    {
        m_game.create(temporary_character_spec(), m_notation);
    }
    catch (const std::exception& e)
    {
        godot::UtilityFunctions::push_error("unable to create a game: ", godot::String::utf8(e.what()));
        return false;
    }

    m_message.clear();
    return true;
}

//------------------------------------------------------------------------------
bool StratumWorld::has_game() const
{
    return m_session.loaded();
}

//------------------------------------------------------------------------------
bool StratumWorld::game_over() const
{
    return m_session.over();
}

//------------------------------------------------------------------------------
void StratumWorld::menu_open()
{
    m_game.menu_open();
    return;
}

//------------------------------------------------------------------------------
void StratumWorld::menu_close()
{
    m_game.menu_close();
    return;
}

//------------------------------------------------------------------------------
void StratumWorld::quit_game()
{
    m_game.quit();
    return;
}

//------------------------------------------------------------------------------
godot::String StratumWorld::save_game()
{
    // Exceptions must not cross into Godot
    try
    {
        return godot::String::utf8(m_game.save(sim::SaveType::Explicit).c_str());
    }
    catch (const std::exception& e)
    {
        godot::UtilityFunctions::push_error("unable to save the game: ", godot::String::utf8(e.what()));
        return {};
    }
}

//------------------------------------------------------------------------------
godot::Array StratumWorld::saved_games() const
{
    godot::Array saves;

    // Exceptions must not cross into Godot
    try
    {
        for (const sim::SavedGameDetails& details : m_game.saved_games())
        {
            std::chrono::sys_seconds time = std::chrono::floor<std::chrono::seconds>(std::chrono::clock_cast<std::chrono::system_clock>(details.time()));

            godot::Dictionary save;
            save["save_name"] = godot::String::utf8(details.save_name().c_str());
            save["character_name"] = godot::String::utf8(details.character_name().c_str());
            save["time"] = godot::String::utf8(std::format("{:%Y-%m-%d %H:%M}", std::chrono::zoned_time(std::chrono::current_zone(),
                std::chrono::floor<std::chrono::minutes>(time))).c_str());
            save["save_type"] = godot::String(get_save_type_label(details.save_type()));
            saves.push_back(save);
        }
    }
    catch (const std::exception& e)
    {
        godot::UtilityFunctions::push_error("unable to list the saved games: ", godot::String::utf8(e.what()));
        return {};
    }

    return saves;
}

//------------------------------------------------------------------------------
bool StratumWorld::open_game(const godot::String& save_name)
{
    // Exceptions must not cross into Godot. A failed opening leaves no game in progress
    try
    {
        m_game.open(save_name.utf8().get_data());
    }
    catch (const std::exception& e)
    {
        godot::UtilityFunctions::push_error("unable to open the game: ", godot::String::utf8(e.what()));
        return false;
    }

    m_message.clear();
    m_notation = m_game.notation();
    return true;
}

//------------------------------------------------------------------------------
bool StratumWorld::remove_game(const godot::String& save_name)
{
    // Exceptions must not cross into Godot
    try
    {
        m_game.remove(save_name.utf8().get_data());
    }
    catch (const std::exception& e)
    {
        godot::UtilityFunctions::push_error("unable to delete the save: ", godot::String::utf8(e.what()));
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
godot::String StratumWorld::view() const
{
    return godot::String::utf8(m_game.manifest().views[m_view_slot].c_str());
}

//------------------------------------------------------------------------------
void StratumWorld::cycle_view()
{
    m_view_slot = (m_view_slot + 1) % m_game.manifest().views.size();
    return;
}

//------------------------------------------------------------------------------
godot::String StratumWorld::notation() const
{
    const std::string_view name = quetzal::stratum_format::name(m_notation);
    return godot::String::utf8(name.data(), static_cast<int64_t>(name.size()));
}

//------------------------------------------------------------------------------
void StratumWorld::cycle_notation()
{
    m_notation = m_notation == quetzal::stratum_format::Notation::Block ? quetzal::stratum_format::Notation::Wall
        : quetzal::stratum_format::Notation::Block;
    return;
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

    const sim::placement_type& placement = sim::placement_of(m_session.character().entity());
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

    return godot::String::utf8(view::render(m_session.world(), m_session.zone(), m_session.character().entity()).to_string().c_str());
}

//------------------------------------------------------------------------------
godot::PackedByteArray StratumWorld::cells() const
{
    godot::PackedByteArray codes;

    if (!m_session.loaded())
    {
        return codes;
    }

    view::Cells cells = view::cells(m_session.world(), m_session.zone(), m_session.character().entity());

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

    view::Cells cells = view::cells(m_session.world(), m_session.zone(), m_session.character().entity());

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
    m_session.character().set_intent(sim::Intent {direction, sim::value_type(1), direction});
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

    m_session.character().submit(sim::Interact {});
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

    m_session.character().submit(sim::Enter {});
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
    m_message.clear();

    // Exceptions must not cross into Godot. A game that fails during play cannot continue, so it is quit
    try
    {
        m_session.advance(events);
    }
    catch (const std::exception& e)
    {
        godot::UtilityFunctions::push_error("the game ended in an error: ", godot::String::utf8(e.what()));
        m_game.quit();
        return;
    }

    for (const sim::Event& event : events.events())
    {
        if (const sim::Blocked* pblocked = event.try_get<sim::Blocked>())
        {
            flecs::entity access_point = pblocked->blocking_wall_id != quetzal::nullid
                ? sim::find_access_point(m_session.zone(), sim::WallAddress {pblocked->blocking_wall_id}) : flecs::entity();
            std::string name = access_point ? sim::access_type_name(access_point) : std::string();

            m_message = access_point ? std::format("There is a {} before you.", name.empty() ? "way" : name) : std::string();
        }
        else if (const sim::Interacted* pinteracted = event.try_get<sim::Interacted>())
        {
            m_message = pinteracted->interaction == sim::Interaction::Opened ? "The door opens."
                : pinteracted->interaction == sim::Interaction::Closed ? "The door shuts."
                : "Nothing happens.";
        }
        else if (event.is<sim::Relocated>())
        {
            // A level is shown as its distance from the surface
            m_message = std::format("You arrive on level {}.", std::abs(m_session.zone().get<sim::ZonePosition>().z));
        }
        else if (event.is<sim::Died>())
        {
            m_message = "You have died.";
        }
    }

    return;
}
