//------------------------------------------------------------------------------
// stratum_terminal
// Application.cpp
//------------------------------------------------------------------------------

#include "Application.hpp"
#include "quetzal/stratum_gen/generators.hpp"
#include "quetzal/stratum_sim/attachments.hpp"
#include "quetzal/stratum_sim/DiscreteLayout.hpp"
#include "quetzal/stratum_sim/commands.hpp"
#include "quetzal/stratum_sim/Site.hpp"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <optional>
#include <string>
#include <vector>
#include <cassert>

using namespace quetzal;

namespace
{

    static constexpr auto help_string = "arrows or hjkl move, space acts, < or > enters, . waits, ^L redraws, Esc for the menu";

    static constexpr auto menu_help_string = "n starts a new game, c or Esc continues, q quits, ^L redraws";

    static constexpr auto end_help_string = "press any key to return to the menu";

    static constexpr char key_redraw = '\x0c'; // Ctrl-L, as curses and the roguelikes bind it

    static constexpr char key_escape = '\x1b'; // Escape in a script, which has characters only

    //--------------------------------------------------------------------------
    // Temporary, until the config file is read: during development the game's directories are in the temp directory
    stratum_sim::Config temporary_config()
    {
        const std::filesystem::path root = std::filesystem::temp_directory_path() / "KML/Stratum";
        return stratum_sim::Config {root / "saves", root / "working", 8};
    }

    //--------------------------------------------------------------------------
    // Temporary, until a new game chooses its region: a dungeon whose first zone is the first level down, entered at its
    // staircase leading up
    stratum_sim::RegionSpec temporary_region_spec()
    {
        return stratum_sim::RegionSpec {{std::string(stratum_sim::generator_selector_key), "dungen"},
            {std::string(stratum_sim::entry_access_key), "staircase,1"}, {std::format("{}{}", stratum_sim::zone_prefix, stratum_sim::level_key), "-1"}};
    }

    //--------------------------------------------------------------------------
    // A new game is named by the local time it began, in ISO 8601 basic format, until a player names a game. A second
    // game begun in the same second has a suffix
    std::string new_game_name(const stratum_sim::Config& config)
    {
        const std::string time = std::format("{:%Y%m%dT%H%M%S}", std::chrono::zoned_time(std::chrono::current_zone(),
            std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now())));
        std::string name = time;

        for (int suffix = 2; std::filesystem::exists(config.saves_directory / name) || std::filesystem::exists(config.working_directory / name); ++suffix)
        {
            name = std::format("{}-{}", time, suffix);
        }

        return name;
    }

    //--------------------------------------------------------------------------
    // Temporary, until the generators are integrated: every zone of the starting region is made with dungen, named
    // "Dungeon Level N" for its level
    stratum_sim::Generation::GeneratorSelection select_dungen(const stratum_sim::World&, flecs::entity, const stratum_sim::ZoneSpec& zone_spec)
    {
        stratum_sim::ZoneSpec named_zone_spec = zone_spec;

        if (std::optional<stratum_sim::level_type> level = zone_spec.value<stratum_sim::level_type>(stratum_sim::level_key))
        {
            named_zone_spec.insert(std::string(stratum_gen::name_key), std::format("Dungeon Level {}", std::abs(*level)));
        }

        return stratum_sim::Generation::GeneratorSelection {std::string(stratum_gen::dungen_generator_name), {}, {}, named_zone_spec};
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
stratum_terminal::Application::Application(View view, const std::string& script, bool incremental) :
    m_game(temporary_config()),
    m_session(m_game.session()),
    m_view_requested(view),
    m_view(view),
    m_view_glyphs(),
    m_terminal(incremental),
    m_message(),
    m_script(script),
    m_script_slot(0),
    m_menu_open(true),
    m_running(true)
{
    // dungen's warnings are dropped, as they would write over the terminal's frame
    stratum_gen::register_generators(m_game.generation(), [](const std::string&) {});
    m_game.generation().m_generator_selectors["dungen"] = select_dungen;
}

//------------------------------------------------------------------------------
int stratum_terminal::Application::run()
{
    // Outlives an iteration, so the events of a command are reported with the frame drawn after it
    stratum_sim::EventQueue events;

    while (m_running)
    {
        if (m_menu_open)
        {
            run_menu();
            continue;
        }

        id_type actor_id = m_session.advance(events);

        report(events);
        events.clear();

        if (m_session.over())
        {
            end_game();
            continue;
        }

        if (actor_id == nullid)
        {
            break;
        }

        draw(help_string);

        if (script_exhausted())
        {
            m_game.quit();
            break;
        }

        KeyPress key_press = next_key();
        if (key_press.key == Key::Escape)
        {
            m_game.menu_open();
            m_menu_open = true;
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
    assert(!script_exhausted());

    if (m_script.empty())
    {
        return read_key();
    }

    char c = m_script[m_script_slot++];
    return c == key_escape ? KeyPress {Key::Escape} : KeyPress {Key::Character, c};
}

//------------------------------------------------------------------------------
bool stratum_terminal::Application::script_exhausted() const
{
    return !m_script.empty() && m_script_slot == m_script.size();
}

//------------------------------------------------------------------------------
void stratum_terminal::Application::run_menu()
{
    draw_menu();

    if (script_exhausted())
    {
        m_game.quit();
        m_running = false;
        return;
    }

    KeyPress key_press = next_key();

    if (key_press.c == key_redraw)
    {
        m_terminal.redraw();
    }
    else if (key_press.c == 'n' || key_press.c == 'N')
    {
        create_game();
    }
    else if ((key_press.key == Key::Escape || key_press.c == 'c' || key_press.c == 'C') && m_session.loaded())
    {
        m_game.menu_close();
        m_menu_open = false;
    }
    else if (key_press.c == 'q' || key_press.c == 'Q')
    {
        m_game.quit();
        m_running = false;
    }

    return;
}

//------------------------------------------------------------------------------
void stratum_terminal::Application::draw_menu()
{
    std::vector<std::string> lines {"STRATUM", "", "n  New game"};

    if (m_session.loaded())
    {
        lines.push_back("c  Continue");
    }

    lines.push_back("q  Quit");

    stratum_view::coord_type width = 0;
    for (const std::string& line : lines)
    {
        width = std::max(width, static_cast<stratum_view::coord_type>(line.size()));
    }

    // Row 0 is the lowest, so the first line is written to the highest row
    const stratum_view::coord_type height = static_cast<stratum_view::coord_type>(lines.size());
    stratum_view::Console console(stratum_view::vector_type(width, height), ' ');

    for (stratum_view::coord_type j = 0; j < height; ++j)
    {
        const std::string& line = lines[static_cast<size_t>(height - 1 - j)];

        for (stratum_view::coord_type i = 0; i < static_cast<stratum_view::coord_type>(line.size()); ++i)
        {
            console.set(stratum_view::point_type(i, j), line[static_cast<size_t>(i)]);
        }
    }

    m_terminal.draw(console, m_message, menu_help_string);
    return;
}

//------------------------------------------------------------------------------
void stratum_terminal::Application::create_game()
{
    try
    {
        m_game.create(new_game_name(m_game.config()), temporary_region_spec(), stratum_sim::CharacterSpec {});
    }
    catch (const std::exception& e)
    {
        m_message = std::format("No game was created: {}", e.what());
        return;
    }

    m_view = m_view_requested != View::Null ? m_view_requested
        : m_session.notation() == stratum_format::Notation::Wall ? View::Wall : View::Cell;
    m_view_glyphs = m_view == View::Cell ? stratum_view::cell_glyphs() : stratum_view::lattice_glyphs();
    m_message.clear();
    m_menu_open = false;

    return;
}

//------------------------------------------------------------------------------
void stratum_terminal::Application::draw(const std::string& help)
{
    stratum_view::Console console = m_view == View::Cell
        ? stratum_view::render(m_session.world(), m_session.zone(), m_session.character().entity(), m_view_glyphs)
        : stratum_view::render_lattice(m_session.world(), m_session.zone(), m_session.character().entity(), m_view_glyphs);

    m_terminal.draw(console, status(), help);
    return;
}

//------------------------------------------------------------------------------
void stratum_terminal::Application::end_game()
{
    draw(end_help_string);

    if (script_exhausted())
    {
        m_game.quit();
        m_running = false;
        return;
    }

    next_key();
    m_game.quit();
    m_message.clear();
    m_menu_open = true;

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
        else if (event.is<stratum_sim::Died>())
        {
            m_message = "You have died.";
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
