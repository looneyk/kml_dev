#if !defined(STRATUM_TERMINAL_APPLICATION_HPP)
#define STRATUM_TERMINAL_APPLICATION_HPP
//------------------------------------------------------------------------------
// stratum_terminal
// Application.hpp
//------------------------------------------------------------------------------

#include "key_input.hpp"
#include "Terminal.hpp"
#include "quetzal/stratum_sim/CommandQueue.hpp"
#include "quetzal/stratum_sim/EventQueue.hpp"
#include "quetzal/stratum_sim/Scheduler.hpp"
#include "quetzal/stratum_sim/World.hpp"
#include "quetzal/stratum_view/ascii.hpp"
#include <string>

namespace stratum_terminal
{

    //--------------------------------------------------------------------------
    // A terminal host for the simulation: keys in, characters out, no engine between
    class Application
    {
    public:

        // A non empty script is played in place of the keyboard, one character per turn,
        // and the run ends when it is exhausted. A scripted run writes whole frames unless
        // asked for incremental, which is how the terminal writer's own output is examined
        Application(const std::string& filepath_level, const std::string& script = {}, bool incremental = true);
        Application(const Application&) = delete;
        ~Application() = default;

        Application& operator=(const Application&) = delete;

        int run();

    private:

        KeyPress next_key();

        void draw();

        // The command a key asks for, empty when it asks for nothing
        quetzal::stratum_sim::Command command_of(const KeyPress& key_press) const;

        void report(const quetzal::stratum_sim::EventQueue& events);
        std::string status() const;

        quetzal::stratum_sim::World m_world;
        flecs::entity m_zone;
        flecs::entity m_player;
        quetzal::stratum_sim::Scheduler m_scheduler;
        quetzal::stratum_view::Glyphs m_view_glyphs;
        Terminal m_terminal;
        std::string m_message;
        std::string m_script;
        size_t m_script_slot = 0;
        bool m_running = true;
    };

} // namespace stratum_terminal

#endif // STRATUM_TERMINAL_APPLICATION_HPP
