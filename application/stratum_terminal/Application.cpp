//------------------------------------------------------------------------------
// stratum_terminal
// Application.cpp
//------------------------------------------------------------------------------

#include "Application.hpp"
#include "quetzal/dungen/generate.hpp"
#include "quetzal/dungen/write_zone.hpp"
#include "quetzal/stratum_sim/attachments.hpp"
#include "quetzal/stratum_sim/DiscreteLayout.hpp"
#include "quetzal/stratum_sim/commands.hpp"
#include "quetzal/stratum_sim/load_layout.hpp"
#include "quetzal/stratum_sim/Site.hpp"
#include <cstdlib>
#include <format>
#include <iostream>
#include <memory>
#include <string>

using namespace quetzal;

namespace
{

    static constexpr auto help_string = "arrows or hjkl move, space acts, < or > enters, . waits, ^L redraws, q quits";

    static constexpr char key_redraw = '\x0c'; // Ctrl-L, as curses and the roguelikes bind it

    // Temporary: the zone loaded is entered at its lowest-ordered staircase leading up
    static constexpr auto entry_access_type = "staircase";
    static constexpr stratum_sim::level_type entry_level_offset = 1;

    // Temporary: the zone loaded is the first level down if its file states no level
    static constexpr stratum_sim::level_type starting_level = -1;

    //--------------------------------------------------------------------------
    // Temporary, until the generators are integrated: the starting region makes its zones with dungen, at the level each
    // is made at. Its warnings are dropped, as they would write over the terminal's frame
    stratum_format::Document generate_dungen(const stratum_sim::ZoneSpec& zone_spec)
    {
        dungen::Parameters parameters;
        stratum_sim::level_type level = zone_spec.value<stratum_sim::level_type>(stratum_sim::level_key).value();
        parameters.level = level;
        parameters.seed = dungen::random_seed();

        std::string name = std::format("Dungeon Level {}", std::abs(level));
        return dungen::make_document(dungen::generate(0, name, parameters, [](const std::string&) {}), parameters);
    }

    //--------------------------------------------------------------------------
    // Temporary, until the generators are integrated: every zone of the starting region is made with dungen
    stratum_sim::Generation::GeneratorSelection select_dungen(const stratum_sim::World&, flecs::entity, const stratum_sim::ZoneSpec& zone_spec)
    {
        return stratum_sim::Generation::GeneratorSelection {"dungen", {}, {}, zone_spec};
    }

    //--------------------------------------------------------------------------
    std::string name_of(const stratum_sim::vector_type& direction)
    {
        auto [dx, dy, dz] = direction.rep();

        if (std::abs(dx) >= std::abs(dy))
        {
            return dx >= stratum_sim::value_type(0) ? "east" : "west";
        }

        return dy >= stratum_sim::value_type(0) ? "north" : "south";
    }

} // namespace

//------------------------------------------------------------------------------
stratum_terminal::Application::Application(const std::string& filepath_zone, View view, const std::string& script, bool incremental) :
    m_game(),
    m_session(m_game.session()),
    m_view(view),
    m_view_glyphs(),
    m_terminal(incremental),
    m_message(),
    m_script(script),
    m_script_slot(0),
    m_running(true)
{
    m_game.generation().m_generators["dungen"] = generate_dungen;
    m_game.generation().m_generator_selectors["dungen"] = select_dungen;
    m_game.starting_region().get_mut<stratum_sim::Region>().region_spec = stratum_sim::RegionSpec {{std::string(stratum_sim::generator_selector_key), "dungen"}};

    m_session.load(m_game.starting_region(), filepath_zone, stratum_sim::access_type(m_game.world().entities(), entry_access_type),
        entry_level_offset);

    if (!m_session.zone().has<stratum_sim::ZoneLevel>())
    {
        m_session.zone().set<stratum_sim::ZoneLevel>(stratum_sim::ZoneLevel {starting_level});
    }

    if (m_view == View::Null)
    {
        m_view = m_session.notation() == stratum_format::Notation::Wall ? View::Wall : View::Cell;
    }

    m_view_glyphs = m_view == View::Cell ? stratum_view::cell_glyphs() : stratum_view::lattice_glyphs();
}

//------------------------------------------------------------------------------
int stratum_terminal::Application::run()
{
    // Outlives an iteration, so the events of a command are reported with the frame drawn after it
    stratum_sim::EventQueue events;

    while (m_running)
    {
        id_type actor_id = m_session.advance(events);

        if (actor_id == nullid)
        {
            break;
        }

        report(events);
        events.clear();
        draw();

        KeyPress key_press = next_key();
        if (key_press.key == Key::Escape || key_press.c == 'q' || key_press.c == 'Q')
        {
            m_running = false;
            continue;
        }

        if (key_press.c == key_redraw)
        {
            m_terminal.redraw();
            continue;
        }

        if (key_press.c == '.')
        {
            m_message.clear();
            m_session.scheduler().spend(actor_id);
            continue;
        }

        submit_key(key_press);
    }

    return 0;
}

//------------------------------------------------------------------------------
stratum_terminal::KeyPress stratum_terminal::Application::next_key()
{
    if (m_script.empty())
    {
        return read_key();
    }

    if (m_script_slot == m_script.size())
    {
        return {Key::Escape};
    }

    return {Key::Character, m_script[m_script_slot++]};
}

//------------------------------------------------------------------------------
void stratum_terminal::Application::draw()
{
    stratum_view::Console console = m_view == View::Cell
        ? stratum_view::render(m_session.world(), m_session.zone(), m_session.character().entity(), m_view_glyphs)
        : stratum_view::render_lattice(m_session.world(), m_session.zone(), m_session.character().entity(), m_view_glyphs);

    m_terminal.draw(console, status(), help_string);
    return;
}

//------------------------------------------------------------------------------
void stratum_terminal::Application::submit_key(const KeyPress& key_press)
{
    using value_type = stratum_sim::value_type;

    // This view does not show the facing direction, so a move also turns toward the way it goes
    auto step = [this](value_type dx, value_type dy)
    {
        stratum_sim::vector_type direction(dx, dy, value_type(0));
        m_session.character().set_intent(stratum_sim::Intent {direction, value_type(1), direction});
    };

    switch (key_press.key)
    {
        case Key::Up: step(value_type(0), value_type(1)); return;
        case Key::Down: step(value_type(0), value_type(-1)); return;
        case Key::Left: step(value_type(-1), value_type(0)); return;
        case Key::Right: step(value_type(1), value_type(0)); return;
        default: break;
    }

    switch (key_press.c)
    {
        case 'k': step(value_type(0), value_type(1)); return;
        case 'j': step(value_type(0), value_type(-1)); return;
        case 'h': step(value_type(-1), value_type(0)); return;
        case 'l': step(value_type(1), value_type(0)); return;

        case ' ': m_session.character().submit(stratum_sim::Interact {}); return;

        case '>':
        case '<': m_session.character().submit(stratum_sim::Enter {}); return;
    }

    return;
}

//------------------------------------------------------------------------------
void stratum_terminal::Application::report(const quetzal::stratum_sim::EventQueue& events)
{
    m_message.clear();

    for (const stratum_sim::Event& event : events.events())
    {
        if (event.is<stratum_sim::Moved>())
        {
            continue; // The map already shows it
        }
        else if (const stratum_sim::Blocked* pblocked = event.try_get<stratum_sim::Blocked>())
        {
            flecs::entity relocator = pblocked->blocking_wall_id != nullid
                ? stratum_sim::find_relocator(m_session.zone(), stratum_sim::WallAddress {pblocked->blocking_wall_id}) : flecs::entity();

            if (relocator)
            {
                std::string name = stratum_sim::access_type_name(relocator);
                m_message = std::format("There is a {} before you.", name.empty() ? "way" : name);
                continue;
            }

            m_message = pblocked->blocking_entity_id != nullid
                ? std::format("Blocked by {}.", pblocked->blocking_entity_id)
                : std::format("Blocked by wall {}.", pblocked->blocking_wall_id);
        }
        else if (event.is<stratum_sim::Turned>())
        {
            continue; // The status line already shows it
        }
        else if (const stratum_sim::Interacted* pinteracted = event.try_get<stratum_sim::Interacted>())
        {
            if (pinteracted->interaction == stratum_sim::Interaction::NoDestination)
            {
                std::string name = stratum_sim::access_type_name(m_session.world().entities().entity(pinteracted->acted_on_id));
                m_message = std::format("You try the {}, but it leads nowhere.", name.empty() ? "way" : name);
                continue;
            }

            m_message = pinteracted->interaction == stratum_sim::Interaction::Opened ? "The door opens."
                : pinteracted->interaction == stratum_sim::Interaction::Closed ? "The door shuts."
                : std::format("Nothing happens at wall {}.", pinteracted->wall_id);
        }
        else if (event.is<stratum_sim::Relocated>())
        {
            // A level is shown as its distance from the surface
            m_message = std::format("You arrive on level {}.", std::abs(m_session.zone().get<stratum_sim::ZoneLevel>().level));
        }
    }

    return;
}

//------------------------------------------------------------------------------
std::string stratum_terminal::Application::status() const
{
    const stratum_sim::placement_type& placement = stratum_sim::placement_of(m_session.character().entity());
    stratum_sim::discrete_point_type cell_position = stratum_sim::layout_as<stratum_sim::DiscreteLayout>(m_session.layout()).cell_position_at(placement.position());

    // The message shares this line so that the frame keeps its height as it comes and goes
    return std::format("cell ({}, {})  facing {}  turn {}  {}", cell_position.i(), cell_position.j(), name_of(placement.direction()), m_session.scheduler().elapsed(), m_message);
}
