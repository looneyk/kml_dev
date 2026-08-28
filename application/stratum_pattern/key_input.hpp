#if !defined(STRATUM_PATTERN_KEY_INPUT_HPP)
#define STRATUM_PATTERN_KEY_INPUT_HPP
//------------------------------------------------------------------------------
// stratum_pattern
// key_input.hpp
//------------------------------------------------------------------------------

namespace stratum_pattern
{

    //--------------------------------------------------------------------------
    enum class Key
    {
        None,
        Character,
        Up,
        Down,
        Left,
        Right,
        Escape
    };

    //--------------------------------------------------------------------------
    struct KeyPress
    {
        Key key = Key::None;
        char c = '\0'; // Only when key is Character
    };

    // Reads one key without waiting for a newline, blocking until there is one
    KeyPress read_key();

} // namespace stratum_pattern

#endif // STRATUM_PATTERN_KEY_INPUT_HPP
