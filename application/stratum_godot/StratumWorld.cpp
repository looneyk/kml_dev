//------------------------------------------------------------------------------
// stratum_godot
// StratumWorld.cpp
//------------------------------------------------------------------------------

#include "StratumWorld.hpp"
#include "quetzal/stratum_sim/DiscreteLayout.hpp" // The tile view, which is grid shaped
#include "quetzal/stratum_sim/commands.hpp"
#include "quetzal/stratum_view/ascii.hpp"
#include "quetzal/stratum_view/cells.hpp"
#include <godot_cpp/variant/utility_functions.hpp>
#include <exception>
#include <string>

namespace sim = quetzal::stratum_sim;
namespace view = quetzal::stratum_view;

//------------------------------------------------------------------------------
StratumWorld::StratumWorld() :
    m_session()
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
    // Anything thrown here would cross into Godot's own frame, so it stops at this boundary.
    // A failed load leaves the previous zone intact, which is the session's guarantee
    try
    {
        m_session.load(std::string(filepath.utf8().get_data()));
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

    const sim::placement_type& placement = m_session.player().get<sim::placement_type>();
    sim::discrete_point_type grid_position = sim::layout_as<sim::DiscreteLayout>(m_session.layout()).grid_position_at(placement.position());

    return godot::Vector2i(grid_position.i(), grid_position.j());
}

//------------------------------------------------------------------------------
godot::String StratumWorld::render() const
{
    if (!m_session.loaded())
    {
        return {};
    }

    return godot::String::utf8(view::render(m_session.world(), m_session.zone()).to_string().c_str());
}

//------------------------------------------------------------------------------
godot::PackedByteArray StratumWorld::cells() const
{
    godot::PackedByteArray codes;

    if (!m_session.loaded())
    {
        return codes;
    }

    // The same reduction the terminal host renders characters from, taken one step earlier
    view::Cells cells = view::cells(m_session.world(), m_session.zone());

    codes.resize(static_cast<int64_t>(cells.width() * cells.height()));

    for (size_t j = 0; j < cells.height(); ++j)
    {
        for (size_t i = 0; i < cells.width(); ++i)
        {
            codes[static_cast<int64_t>(j * cells.width() + i)] = static_cast<uint8_t>(cells.at(i, j).type);
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

    view::Cells cells = view::cells(m_session.world(), m_session.zone());

    flags.resize(static_cast<int64_t>(cells.width() * cells.height()));

    for (size_t j = 0; j < cells.height(); ++j)
    {
        for (size_t i = 0; i < cells.width(); ++i)
        {
            flags[static_cast<int64_t>(j * cells.width() + i)] = cells.at(i, j).actor ? 1 : 0;
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

    m_session.set_intent(sim::Intent {
        sim::vector_type(static_cast<sim::value_type>(di), static_cast<sim::value_type>(dj), sim::value_type(0)),
        sim::value_type(1), sim::vector_type()});

    // Produced and discarded: nothing here consumes them yet, where the terminal host reports
    // them on its status line
    sim::EventQueue events;
    m_session.advance(events);

    return cell();
}

//------------------------------------------------------------------------------
int64_t StratumWorld::turn() const
{
    return static_cast<int64_t>(m_session.scheduler().elapsed());
}
