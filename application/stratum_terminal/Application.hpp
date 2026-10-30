#if !defined(STRATUM_TERMINAL_APPLICATION_HPP)
#define STRATUM_TERMINAL_APPLICATION_HPP
//------------------------------------------------------------------------------
// stratum_terminal
// Application.hpp
//------------------------------------------------------------------------------

#include "key_input.hpp"
#include "Terminal.hpp"
#include "quetzal/stratum_format/GameManifestDocument.hpp"
#include "quetzal/stratum_sim/Config.hpp"
#include "quetzal/stratum_sim/EventQueue.hpp"
#include "quetzal/stratum_sim/Game.hpp"
#include "quetzal/stratum_sim/SavedGameDetails.hpp"
#include "quetzal/stratum_sim/Session.hpp"
#include "quetzal/stratum_sim/PersistentSettings.hpp"
#include <string>
#include <vector>

namespace stratum_terminal
{

    //--------------------------------------------------------------------------
    // Which reduction the host draws: a glyph per cell, or the lattice, which also draws the walls and doors on edges
    // between cells
    enum class View
    {
        Cell,
        Lattice
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

        // A non empty script replaces the keyboard, one character per turn, and the run ends when it is exhausted.
        // Throws std::runtime_error if the manifest lists a view the terminal does not have
        Application(const quetzal::stratum_sim::Config& config, const quetzal::stratum_format::GameManifestDocument& manifest,
            const std::string& script, bool incremental);
        Application(const Application&) = delete;

        ~Application();

        Application& operator=(const Application&) = delete;

        int run();

    private:

        KeyPress next_key();

        // True once a script has played its last key, which ends the run
        bool script_exhausted() const;

        // Draws the menu's page, reads a key, and acts on it. The main page has a new game, continuing or saving the game in
        // progress, listing the saves, changing the view or the notation, and quitting; the saves page selects a save, opens it or the save of a digit, and
        // deletes the selected save once confirmed
        void run_menu();
        void run_main_page(const KeyPress& key_press);
        void run_saves_page(const KeyPress& key_press);

        // Draws with the manifest's view at the slot from now on
        void choose_view(size_t view_slot);

        void draw_menu();

        // Draws the lines centered in place of the map, the first line highest
        void draw_menu_lines(const std::vector<std::string>& lines, const std::string& help);

        // Creates a new game and closes the menu, or keeps the menu open with the reason the game could not be created
        void create_game();

        // Lists the newest saves on the saves page, keeping the selection within them, or shows the main page with the reason
        // they could not be listed
        void list_saved_games();

        // Deletes the selected save and lists the saves again, or reports the reason it was not deleted
        void remove_selected_save();

        // Opens the save and closes the menu, or returns to the main page with the reason it could not be opened
        void open_game(const std::string& save_name);

        // Closes the menu on the game just created or opened
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
        quetzal::stratum_sim::PersistentSettings m_user_settings;

        std::vector<View> m_views; // The manifest's, in its order
        size_t m_view_selected = 0;
        View m_view = View::Cell;

        quetzal::stratum_view::Glyphs m_view_glyphs;
        quetzal::stratum_format::Notation m_notation; // The notation a new game is created in, the last game's once one is opened
        Terminal m_terminal;
        std::string m_message;
        std::string m_script;
        size_t m_script_slot = 0;
        bool m_menu_open = true;
        MenuPage m_menu_page = MenuPage::Main;
        std::vector<quetzal::stratum_sim::SavedGameDetails> m_saved_games; // The newest saves, as the saves page lists them
        size_t m_saved_game_selected = 0;
        bool m_save_removal_confirming = false; // The next key answers whether to delete the selected save
        bool m_running = true;
    };

} // namespace stratum_terminal

#endif // STRATUM_TERMINAL_APPLICATION_HPP
