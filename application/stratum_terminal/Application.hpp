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
#include "quetzal/stratum_sim/SavedGameDetails.hpp"
#include "quetzal/stratum_sim/Session.hpp"
#include <string>
#include <vector>

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
    // The page the menu shows: its choices, or the saves to open
    enum class MenuPage
    {
        Main,
        Saves
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

        // True once a script has played its last key, which ends the run
        bool script_exhausted() const;

        // Draws the menu's page, reads a key, and acts on it. The main page has a new game, continuing or saving the game in
        // progress, listing the saves to open one, and quitting; the saves page opens the save of a digit
        void run_menu();
        void run_main_page(const KeyPress& key_press);
        void run_saves_page(const KeyPress& key_press);

        void draw_menu();

        // Draws the lines centered in place of the map, the first line highest
        void draw_menu_lines(const std::vector<std::string>& lines, const std::string& help);

        // Creates a new game and closes the menu, or keeps the menu open with the reason the game could not be created
        void create_game();

        // Lists the newest saves on the saves page, or keeps the main page with the reason they could not be listed
        void list_saved_games();

        // Opens the save and closes the menu, or returns to the main page with the reason it could not be opened
        void open_game(const std::string& save_name);

        // Chooses the view for the game just created or opened, and closes the menu
        void start_play();

        // Saves the game in progress, and reports that it saved, or the reason it did not, on the menu
        void save_game();

        void draw(const std::string& help);

        // Shows the character's death and waits for a key, then quits the game and opens the menu
        void end_game();

        // Submits a key for the player: movement and facing direction as its intent, and a one-shot action as a command
        void submit_key(const KeyPress& key_press);

        void report(const quetzal::stratum_sim::EventQueue& events);
        std::string status() const;

        quetzal::stratum_sim::Game m_game;
        quetzal::stratum_sim::Session& m_session;

        View m_view_requested = View::Null;
        View m_view = View::Null; // Never Null once a game is created

        quetzal::stratum_view::Glyphs m_view_glyphs;
        Terminal m_terminal;
        std::string m_message;
        std::string m_script;
        size_t m_script_slot = 0;
        bool m_menu_open = true;
        MenuPage m_menu_page = MenuPage::Main;
        std::vector<quetzal::stratum_sim::SavedGameDetails> m_saved_games; // The newest saves, as the saves page lists them
        bool m_running = true;
    };

} // namespace stratum_terminal

#endif // STRATUM_TERMINAL_APPLICATION_HPP
