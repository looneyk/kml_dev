#if !defined(STRATUM_GODOT_STRATUMWORLD_HPP)
#define STRATUM_GODOT_STRATUMWORLD_HPP
//------------------------------------------------------------------------------
// stratum_godot
// StratumWorld.hpp
//------------------------------------------------------------------------------

#include "quetzal/stratum_sim/Session.hpp"
#include "quetzal/stratum_sim/StratumGame.hpp"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2i.hpp>

//------------------------------------------------------------------------------
// One simulation zone and the player, exposed to Godot through Godot types only
class StratumWorld : public godot::RefCounted
{
    GDCLASS(StratumWorld, godot::RefCounted);

protected:

    static void _bind_methods();

public:

    StratumWorld();
    ~StratumWorld() override = default;

    // The shared zone file directory, configured at build time
    godot::String default_zone_path() const;

    // False, with the reason pushed to Godot's error list, when the file cannot be read
    bool load_zone(const godot::String& filepath);

    godot::Vector2i extent() const;
    godot::Vector2i cell() const;

    // The whole zone as characters, one line per row, highest row first
    godot::String render() const;

    // The fixed structure at each cell, one code per cell, row major from row zero. The codes repeat
    // stratum_view::PlaceType for Godot; None and Wall are never emitted, but keep the values aligned
    enum PlaceType
    {
        PlaceTypeNone,
        PlaceTypeSolid,
        PlaceTypeFloor,
        PlaceTypeWall,
        PlaceTypeDoor
    };

    godot::PackedByteArray cells() const;

    // One byte per cell in the order of cells(), nonzero if an actor is on the cell
    godot::PackedByteArray actors() const;

    // The cell arrived at, unchanged when the move was blocked or it was not the player's turn
    godot::Vector2i step(int32_t di, int32_t dj);

    // Scheduler units elapsed. Advances only when something actually costs a turn
    int64_t turn() const;

private:

    quetzal::stratum_sim::StratumGame m_stratum_game;
    quetzal::stratum_sim::Session& m_session;
};

VARIANT_ENUM_CAST(StratumWorld::PlaceType);

#endif // STRATUM_GODOT_STRATUMWORLD_HPP
