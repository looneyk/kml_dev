//------------------------------------------------------------------------------
// stratum_pattern
// Application.cpp
//------------------------------------------------------------------------------

#include "Application.hpp"
#include "quetzal/stratum_sim/commands.hpp"
#include "quetzal/stratum_sim/load_layout.hpp"
#include <format>
#include <iostream>
#include <stdexcept>

using namespace quetzal;

namespace
{

    static constexpr auto help_string = "arrows or hjkl move, < > turn, space acts, . waits, ^L redraws, q quits";

    static constexpr char key_redraw = '\x0c'; // Ctrl-L, as curses and the roguelikes bind it

    static constexpr int player_speed = 100;

    //--------------------------------------------------------------------------
    // The lowest open cell of a layout, where the player starts
    stratum_sim::Cell first_open_cell(const stratum_sim::Layout& layout)
    {
        const stratum_sim::Grid& grid = layout.grid();

        for (int j = 0; j < grid.extent.j; ++j)
        {
            for (int i = 0; i < grid.extent.i; ++i)
            {
                if (layout.region_at(stratum_sim::Cell {i, j}) != nullid)
                {
                    return {i, j};
                }
            }
        }

        throw std::runtime_error("level has no open cell");
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
stratum_pattern::Application::Application(const std::string& filepath_level, const std::string& script, bool incremental) :
    m_world(),
    m_zone(),
    m_player(),
    m_scheduler(),
    m_view_glyphs(),
    m_terminal(incremental),
    m_message(),
    m_script(script),
    m_script_slot(0),
    m_running(true)
{
    m_zone = m_world.create_zone("dungeon");
    stratum_sim::load_layout(filepath_level, m_world.layout(m_zone));

    const stratum_sim::Layout& layout = m_world.layout(m_zone);
    stratum_sim::point_type position = layout.cell_position(first_open_cell(layout));

    m_player = m_world.create_entity(m_zone, stratum_sim::placement_type(position, stratum_sim::vector_type(1.0f, 0.0f, 0.0f)));
    m_scheduler.add(m_player.id(), player_speed);
}

//------------------------------------------------------------------------------
int stratum_pattern::Application::run()
{
    while (m_running)
    {
        // Only the player is scheduled so far, but the loop is already driven by turn order
        id_type actor_id = m_scheduler.next();
        if (actor_id == nullid)
        {
            break;
        }

        draw();

        KeyPress key_press = next_key();
        if (key_press.key == Key::Escape || key_press.c == 'q' || key_press.c == 'Q')
        {
            m_running = false;
            continue;
        }

        // Costs no turn, so the scheduler is left where it is
        if (key_press.c == key_redraw)
        {
            m_terminal.redraw();
            continue;
        }

        // Waiting asks the simulation for nothing, so the host spends the turn itself
        if (key_press.c == '.')
        {
            m_message.clear();
            m_scheduler.spend(actor_id);
            continue;
        }

        stratum_sim::Command command = command_of(key_press);
        if (command.empty())
        {
            continue;
        }

        stratum_sim::EventQueue events;
        int cost = stratum_sim::apply(m_world, command, events);
        report(events);

        if (cost > 0)
        {
            m_scheduler.spend(actor_id, cost);
        }
    }

    return 0;
}

//------------------------------------------------------------------------------
stratum_pattern::KeyPress stratum_pattern::Application::next_key()
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
void stratum_pattern::Application::draw()
{
    m_terminal.draw(stratum_view::render(m_world, m_zone, m_view_glyphs), status(), help_string);
    return;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Command stratum_pattern::Application::command_of(const KeyPress& key_press) const
{
    id_type id = m_player.id();

    auto step = [id](stratum_sim::value_type dx, stratum_sim::value_type dy)
    {
        return stratum_sim::Command(id, stratum_sim::Step {stratum_sim::vector_type(dx, dy, 0.0f), 1.0f});
    };

    switch (key_press.key)
    {
        case Key::Up: return step(0.0f, 1.0f);
        case Key::Down: return step(0.0f, -1.0f);
        case Key::Left: return step(-1.0f, 0.0f);
        case Key::Right: return step(1.0f, 0.0f);
        default: break;
    }

    switch (key_press.c)
    {
        case 'k': return step(0.0f, 1.0f);
        case 'j': return step(0.0f, -1.0f);
        case 'h': return step(-1.0f, 0.0f);
        case 'l': return step(1.0f, 0.0f);

        case '<': return stratum_sim::Command(id, stratum_sim::TurnTo {stratum_sim::Turn::Left});
        case '>': return stratum_sim::Command(id, stratum_sim::TurnTo {stratum_sim::Turn::Right});

        case ' ': return stratum_sim::Command(id, stratum_sim::Interact {});
    }

    return {};
}

//------------------------------------------------------------------------------
void stratum_pattern::Application::report(const quetzal::stratum_sim::EventQueue& events)
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
            m_message = std::format("Blocked by wall {}.", pblocked->wall_id);
        }
        else if (event.is<stratum_sim::Turned>())
        {
            continue; // The status line already shows it
        }
        else if (const stratum_sim::Interacted* pinteracted = event.try_get<stratum_sim::Interacted>())
        {
            m_message = std::format("Nothing happens at wall {}.", pinteracted->wall_id);
        }
    }

    return;
}

//------------------------------------------------------------------------------
std::string stratum_pattern::Application::status() const
{
    const stratum_sim::Layout& layout = m_world.layout(m_zone);
    const stratum_sim::placement_type& placement = m_player.get<stratum_sim::placement_type>();
    stratum_sim::Cell cell = layout.cell_at(placement.position());

    // The message shares this line so that the frame keeps its height as it comes and goes
    return std::format("cell ({}, {})  facing {}  turn {}  {}", cell.i, cell.j, name_of(placement.direction()), m_scheduler.elapsed(), m_message);
}
