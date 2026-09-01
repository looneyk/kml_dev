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

    // Where the shared level files are, configured in at build time rather than found
    // relative to wherever Godot was started from
    godot::String default_level_path() const;

    // False, with the reason pushed to Godot's error list, when the file cannot be read
    bool load_level(const godot::String& filepath);

    godot::Vector2i extent() const;
    godot::Vector2i cell() const;

    // The whole zone as characters, one line per row, highest row first
    godot::String render() const;

    // What occupies each cell, one code per cell, row major from row zero. A tile view wants
    // this rather than render's characters. The codes are stratum_view::Kind, repeated here
    // only because Godot has to be told the names of the constants it can use
    enum Kind
    {
        KindRock,
        KindFloor,
        KindActor
    };

    godot::PackedByteArray cells() const;

    // The cell arrived at, unchanged when something blocked the way or it was not this
    // entity's turn
    godot::Vector2i step(int32_t di, int32_t dj);

    // Scheduler units elapsed. Advances only when something actually costs a turn
    int64_t turn() const;

private:

    quetzal::stratum_sim::Session m_session;
};

VARIANT_ENUM_CAST(StratumWorld::Kind);

#endif // STRATUM_GODOT_STRATUMWORLD_HPP
