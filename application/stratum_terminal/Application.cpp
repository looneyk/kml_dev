//------------------------------------------------------------------------------
// stratum_terminal
// Application.cpp
//------------------------------------------------------------------------------

#include "Application.hpp"
#include "quetzal/stratum_sim/DiscreteLayout.hpp"
#include "quetzal/stratum_sim/commands.hpp"
#include "quetzal/stratum_sim/Site.hpp"
#include <format>
#include <iostream>

using namespace quetzal;

namespace
{

    static constexpr auto help_string = "arrows or hjkl move, space acts, . waits, ^L redraws, q quits";

    static constexpr char key_redraw = '\x0c'; // Ctrl-L, as curses and the roguelikes bind it

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
    m_session(),
    m_view(view),
    m_view_glyphs(),
    m_terminal(incremental),
    m_message(),
    m_script(script),
    m_script_slot(0),
    m_running(true)
{
    m_session.load(filepath_zone);

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

        submit_key(key_press, actor_id);
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
        ? stratum_view::render(m_session.world(), m_session.zone(), m_view_glyphs)
        : stratum_view::render_lattice(m_session.world(), m_session.zone(), m_view_glyphs);

    m_terminal.draw(console, status(), help_string);
    return;
}

//------------------------------------------------------------------------------
void stratum_terminal::Application::submit_key(const KeyPress& key_press, id_type id)
{
    using value_type = stratum_sim::value_type;

    // This view does not show the facing direction, so a move also turns toward the way it goes
    auto step = [this](value_type dx, value_type dy)
    {
        stratum_sim::vector_type direction(dx, dy, value_type(0));
        m_session.set_intent(stratum_sim::Intent {direction, value_type(1), direction});
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

        case ' ': m_session.submit(stratum_sim::Command(id, stratum_sim::Interact {})); return;
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
            m_message = pinteracted->interaction == stratum_sim::Interaction::Opened ? "The door opens."
                : pinteracted->interaction == stratum_sim::Interaction::Closed ? "The door shuts."
                : std::format("Nothing happens at wall {}.", pinteracted->wall_id);
        }
    }

    return;
}

//------------------------------------------------------------------------------
std::string stratum_terminal::Application::status() const
{
    const stratum_sim::placement_type& placement = stratum_sim::placement_of(m_session.player());
    stratum_sim::discrete_point_type cell_position = stratum_sim::layout_as<stratum_sim::DiscreteLayout>(m_session.layout()).cell_position_at(placement.position());

    // The message shares this line so that the frame keeps its height as it comes and goes
    return std::format("cell ({}, {})  facing {}  turn {}  {}", cell_position.i(), cell_position.j(), name_of(placement.direction()), m_session.scheduler().elapsed(), m_message);
}
