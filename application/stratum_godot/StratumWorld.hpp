#if !defined(STRATUM_GODOT_STRATUMWORLD_HPP)
#define STRATUM_GODOT_STRATUMWORLD_HPP
//------------------------------------------------------------------------------
// stratum_godot
// StratumWorld.hpp
//------------------------------------------------------------------------------

#include "quetzal/stratum_sim/Session.hpp"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2i.hpp>

//------------------------------------------------------------------------------
// One simulation zone and the entity moving through it, exposed to Godot. Answers only in
// Godot types, so nothing above this sees a quetzal type or has to be rebuilt when one changes
class StratumWorld : public godot::RefCounted
{
    GDCLASS(StratumWorld, godot::RefCounted);

protected:

    static void _bind_methods();

public:

    StratumWorld();
    ~StratumWorld() override = default;

    // Where the shared zone files are, configured in at build time rather than found
    // relative to wherever Godot was started from
    godot::String default_zone_path() const;

    // False, with the reason pushed to Godot's error list, when the file cannot be read
    bool load_zone(const godot::String& filepath);

    godot::Vector2i extent() const;
    godot::Vector2i cell() const;

    // The whole zone as characters, one line per row, highest row first
    godot::String render() const;

    // The fixed structure at each cell, one code per cell, row major from row zero. A tile
    // view needs this rather than render's characters. The codes are stratum_view::PlaceType,
    // repeated here only because Godot has to be told the names of the constants it can use
    // None and Wall are never emitted by this reduction -- every grid position is solid, floor
    // or door, and it has nowhere to put an edge -- and are here so that the values line
    // up with stratum_view::PlaceType rather than being renumbered
    enum PlaceType
    {
        PlaceTypeNone,
        PlaceTypeSolid,
        PlaceTypeFloor,
        PlaceTypeWall,
        PlaceTypeDoor
    };

    godot::PackedByteArray cells() const;

    // Where something stands, one byte per cell on the same shape and order as cells(). An
    // actor is layered over the structure rather than replacing it, so a view draws the floor
    // from cells() and the mover from this
    godot::PackedByteArray actors() const;

    // The cell arrived at, unchanged when something blocked the way or it was not this
    // entity's turn
    godot::Vector2i step(int32_t di, int32_t dj);

    // Scheduler units elapsed. Advances only when something actually costs a turn
    int64_t turn() const;

private:

    quetzal::stratum_sim::Session m_session;
};

VARIANT_ENUM_CAST(StratumWorld::PlaceType);

#endif // STRATUM_GODOT_STRATUMWORLD_HPP
