#if !defined(STRATUM_TERMINAL_APPLICATION_HPP)
#define STRATUM_TERMINAL_APPLICATION_HPP
//------------------------------------------------------------------------------
// stratum_terminal
// Application.hpp
//------------------------------------------------------------------------------

#include "key_input.hpp"
#include "Terminal.hpp"
#include "quetzal/stratum_sim/EventQueue.hpp"
#include "quetzal/stratum_sim/Game.hpp"
#include "quetzal/stratum_sim/Session.hpp"
#include <string>

namespace stratum_terminal
{

    //--------------------------------------------------------------------------
    // Which reduction the host draws: a glyph per cell, or the lattice, which also draws the walls and doors on edges
    // between cells. Null lets the zone's notation decide
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

        // A non empty script replaces the keyboard, one character per turn, and the run ends when it is exhausted
        explicit Application(View view = View::Null, const std::string& script = {}, bool incremental = true);
        Application(const Application&) = delete;
        ~Application() = default;

        Application& operator=(const Application&) = delete;

        int run();

    private:

        KeyPress next_key();

        void draw();

        // Submits a key for the player: movement and facing direction as its intent, and a one-shot action as a command
        void submit_key(const KeyPress& key_press);

        void report(const quetzal::stratum_sim::EventQueue& events);
        std::string status() const;

        quetzal::stratum_sim::Game m_game;
        quetzal::stratum_sim::Session& m_session;

        View m_view = View::Null; // Never Null once the zone is loaded

        quetzal::stratum_view::Glyphs m_view_glyphs;
        Terminal m_terminal;
        std::string m_message;
        std::string m_script;
        size_t m_script_slot = 0;
        bool m_running = true;
    };

} // namespace stratum_terminal

#endif // STRATUM_TERMINAL_APPLICATION_HPP
