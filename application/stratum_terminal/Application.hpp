#if !defined(STRATUM_TERMINAL_APPLICATION_HPP)
#define STRATUM_TERMINAL_APPLICATION_HPP
//------------------------------------------------------------------------------
// stratum_terminal
// Application.hpp
//------------------------------------------------------------------------------

#include "key_input.hpp"
#include "Terminal.hpp"
#include "quetzal/stratum_sim/EventQueue.hpp"
#include "quetzal/stratum_sim/Session.hpp"
#include <string>

namespace stratum_terminal
{

    //--------------------------------------------------------------------------
    // Which reduction the host draws. The cell view is a glyph per cell, which is what a
    // roguelike shows; the wall view is the lattice, which also draws what stands on the
    // edges between cells and so is the only one that shows a door or a wall two rooms share
    // Unasked, the zone decides: a block format zone loses nothing through the cell view,
    // and a wall format one loses every wall between two open cells
    enum class View
    {
        Null,
        Cell,
        Wall
    };

    //--------------------------------------------------------------------------
    // A terminal host for the simulation: keys in, characters out, no engine between
    class Application
    {
    public:

        // A non empty script is played in place of the keyboard, one character per turn,
        // and the run ends when it is exhausted. A scripted run writes whole frames unless
        // asked for incremental, which is how the terminal writer's own output is examined
        Application(const std::string& filepath_zone, View view = View::Null,
            const std::string& script = {}, bool incremental = true);
        Application(const Application&) = delete;
        ~Application() = default;

        Application& operator=(const Application&) = delete;

        int run();

    private:

        KeyPress next_key();

        void draw();

        // Routes a key to the session: movement and facing as intent, one-shot actions as
        // commands. Nothing is submitted for a key that asks for neither
        // For the actor the session is waiting on, not for the player by assumption
        void submit_key(const KeyPress& key_press, quetzal::id_type actor_id);

        void report(const quetzal::stratum_sim::EventQueue& events);
        std::string status() const;

        quetzal::stratum_sim::Session m_session;

        // Resolved once the zone is loaded, so it is never Null by the time anything draws
        View m_view = View::Null;

        // The set the chosen view is drawn with, taken once rather than asked for per frame
        quetzal::stratum_view::Glyphs m_view_glyphs;
        Terminal m_terminal;
        std::string m_message;
        std::string m_script;
        size_t m_script_slot = 0;
        bool m_running = true;
    };

} // namespace stratum_terminal

#endif // STRATUM_TERMINAL_APPLICATION_HPP
