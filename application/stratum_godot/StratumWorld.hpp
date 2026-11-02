#if !defined(STRATUM_GODOT_STRATUMWORLD_HPP)
#define STRATUM_GODOT_STRATUMWORLD_HPP
//------------------------------------------------------------------------------
// stratum_godot
// StratumWorld.hpp
//------------------------------------------------------------------------------

#include "quetzal/stratum_sim/Game.hpp"
#include "quetzal/stratum_sim/Session.hpp"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include <cstddef>
#include <filesystem>
#include <string>

// Returns the Godot project's directory, res://, which is the program directory: the host's executable is Godot's
std::filesystem::path project_directory();

//------------------------------------------------------------------------------
// The game, exposed to Godot through Godot types only
class StratumWorld : public godot::RefCounted
{
    GDCLASS(StratumWorld, godot::RefCounted);

protected:

    static void _bind_methods();

public:

    StratumWorld();
    ~StratumWorld() override = default;

    // Creates a new game and starts play in its first zone. False, with the reason pushed to Godot's error list, when the
    // game cannot be created
    bool create_game();

    // True while a game is in progress, whether or not the menu is open
    bool has_game() const;

    // True once the character is dead; the scene shows the death and then quits the game
    bool game_over() const;

    // Pause play while the menu is open, and resume it when the menu closes
    void menu_open();
    void menu_close();

    // Ends the game in progress, before the host quits
    void quit_game();

    // Saves the game in progress and returns the save's name. Empty, with the reason pushed to Godot's error list, when the
    // game cannot be saved
    godot::String save_game();

    // The saves, newest first, each a Dictionary with its save_name, which open_game takes, its character_name, and its
    // time, local and to the minute. Empty, with the reason pushed to Godot's error list, when they cannot be listed
    godot::Array saved_games() const;

    // Opens the save and starts play in it. False, with the reason pushed to Godot's error list, when it cannot be opened,
    // which leaves no game in progress
    bool open_game(const godot::String& save_name);

    // Deletes the save. False, with the reason pushed to Godot's error list, when it is not deleted
    bool remove_game(const godot::String& save_name);

    // The view the host draws: the manifest's default view at first
    godot::String view() const;

    // Changes the view to the next of the manifest's views
    void cycle_view();

    // The notation a new game is created in: the manifest's at first, and the last game's once one is opened
    godot::String notation() const;

    // Changes the notation a new game is created in to the other one
    void cycle_notation();

    godot::Vector2i extent() const;
    godot::Vector2i cell() const;

    // The whole zone as characters, one line per row, highest row first
    godot::String render() const;

    // The fixed structure at each cell, one code per cell, row major from row zero. The codes repeat
    // stratum_view::PlaceType for Godot, followed by PlaceTypeDoorOpen for a door whose access is open; None and Wall are
    // never emitted, but keep the values aligned
    enum PlaceType
    {
        PlaceTypeNone,
        PlaceTypeSolid,
        PlaceTypeFloor,
        PlaceTypeWall,
        PlaceTypeDoor,
        PlaceTypeStairsUp,
        PlaceTypeStairsDown,
        PlaceTypeDoorOpen
    };

    // Who is on each cell
    enum Occupant
    {
        OccupantNone,
        OccupantPlayer,
        OccupantOther
    };

    godot::PackedByteArray cells() const;

    // One Occupant per cell in the order of cells()
    godot::PackedByteArray actors() const;

    // The cell arrived at, unchanged when the move was blocked or it was not the player's turn
    godot::Vector2i step(int32_t di, int32_t dj);

    // Acts on what the player faces, such as opening or closing a door
    void interact();

    // Enters the access point on the floor of the player's cell. The cell arrived at, unchanged when there is none or it was
    // not the player's turn
    godot::Vector2i enter();

    // Scheduler units elapsed. Advances only when something actually costs a turn
    int64_t turn() const;

    // What the last step, interact, or enter did that the map does not show, or an empty string
    godot::String message() const;

private:

    // Advances the session and sets the message from its events
    void advance();

    quetzal::stratum_sim::Game m_game;
    quetzal::stratum_sim::Session& m_session;
    std::string m_message;
    size_t m_view_slot; // The slot of the view in the manifest's views
    quetzal::stratum_format::Notation m_notation;
};

VARIANT_ENUM_CAST(StratumWorld::PlaceType);
VARIANT_ENUM_CAST(StratumWorld::Occupant);

#endif // STRATUM_GODOT_STRATUMWORLD_HPP
