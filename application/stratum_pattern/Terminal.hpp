#if !defined(STRATUM_PATTERN_TERMINAL_HPP)
#define STRATUM_PATTERN_TERMINAL_HPP
//------------------------------------------------------------------------------
// stratum_pattern
// Terminal.hpp
//------------------------------------------------------------------------------

#include "quetzal/stratum_view/Console.hpp"
#include <string>

namespace stratum_pattern
{

    //--------------------------------------------------------------------------
    struct Size
    {
        size_t rows = 0;
        size_t columns = 0;

        friend bool operator==(const Size&, const Size&) = default;
    };

    // The terminal's text area, zero in both when it cannot be determined, as when output
    // is a pipe rather than a console
    Size terminal_size();

    // Asks for a text area of this size, clamped to what the screen can hold. False when
    // the request could not be made at all; on anything but Windows it is a request with
    // no reply, so true does not promise the terminal obliged
    bool set_terminal_size(const Size& size);

    //--------------------------------------------------------------------------
    // Writes a Console to a character terminal, emitting only the cells that changed
    // since the last frame so that the screen is never blanked and never scrolls
    // The escapes are CUP, ED and EL of ECMA-48, plus DECTCEM to hide the cursor, which is
    // a DEC private mode rather than a standard one and is only cosmetic
    // On Windows these need a console with virtual terminal processing enabled, which
    // Windows Terminal provides
    class Terminal
    {
    public:

        // Not incremental writes every frame in full and in order, one after another,
        // for a run whose output is read back rather than watched
        Terminal(bool incremental = true);
        Terminal(const Terminal&) = delete;
        ~Terminal();

        Terminal& operator=(const Terminal&) = delete;

        void draw(const quetzal::stratum_view::Console& console, const std::string& status, const std::string& help);

        // Discards what is believed to be on screen, so that the next draw paints in full
        void redraw();

    private:

        quetzal::stratum_view::Console m_console_previous;
        Size m_size_previous;
        Size m_size_original; // What the terminal was before it was sized to the level
        Size m_size_set;      // And what it became, so a later resize by hand can be told apart
        bool m_incremental;
        bool m_sized = false; // The level's size is not known until the first frame arrives
    };

} // namespace stratum_pattern

#endif // STRATUM_PATTERN_TERMINAL_HPP
