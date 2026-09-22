#if !defined(STRATUM_TERMINAL_TERMINAL_HPP)
#define STRATUM_TERMINAL_TERMINAL_HPP
//------------------------------------------------------------------------------
// stratum_terminal
// Terminal.hpp
//------------------------------------------------------------------------------

#include "quetzal/stratum_view/ascii.hpp"
#include <string>

namespace stratum_terminal
{

    using quetzal::stratum_view::coord_type;
    using quetzal::stratum_view::vector_type;

    // The terminal's text area in columns by rows, zero in both when it cannot be determined,
    // as when output is a pipe rather than a console
    vector_type terminal_size();

    // Requests a text area of this size, clamped to the screen. False when the request could not be made; outside
    // Windows, true does not confirm the terminal complied
    bool set_terminal_size(const vector_type& size);

    //--------------------------------------------------------------------------
    // Writes a Console to a character terminal, emitting only the cells changed since the last frame
    // Uses the ECMA-48 escapes CUP, ED and EL, and the DEC private mode DECTCEM to hide the cursor. On Windows these need
    // virtual terminal processing enabled
    class Terminal
    {
    public:

        // Non-incremental writes every frame in full, one after another
        Terminal(bool incremental = true);
        Terminal(const Terminal&) = delete;
        ~Terminal();

        Terminal& operator=(const Terminal&) = delete;

        void draw(const quetzal::stratum_view::Console& console, const std::string& status, const std::string& help);

        // Discards what is believed to be on screen, so that the next draw paints in full
        void redraw();

    private:

        quetzal::stratum_view::Console m_console_previous;
        vector_type m_size_previous;
        vector_type m_size_original; // Before sizing to the zone
        vector_type m_size_set;      // After sizing to the zone, to detect a manual resize
        bool m_incremental;
        bool m_sized = false; // The zone's size is not known until the first frame arrives
    };

} // namespace stratum_terminal

#endif // STRATUM_TERMINAL_TERMINAL_HPP
