#if !defined(STRATUM_TERMINAL_KEY_INPUT_HPP)
#define STRATUM_TERMINAL_KEY_INPUT_HPP
//------------------------------------------------------------------------------
// stratum_terminal
// key_input.hpp
//------------------------------------------------------------------------------

namespace stratum_terminal
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

} // namespace stratum_terminal

#endif // STRATUM_TERMINAL_KEY_INPUT_HPP
