#if !defined(QUETZAL_STRATUM_VIEW_CONSOLE_HPP)
#define QUETZAL_STRATUM_VIEW_CONSOLE_HPP
//------------------------------------------------------------------------------
// stratum_view
// Console.hpp
//------------------------------------------------------------------------------

#include <string>
#include <vector>

namespace quetzal::stratum_view
{

    //--------------------------------------------------------------------------
    // A grid of characters to be drawn, not a device that draws them. The term is
    // libtcod's and curses', where a console is an offscreen buffer that is later blitted
    // Row 0 is the lowest, matching the simulation, so writing it out reverses them
    class Console
    {
    public:

        Console() = default;
        Console(size_t width, size_t height, char fill = ' ');
        Console(const Console&) = default;
        Console(Console&&) noexcept = default;
        ~Console() = default;

        Console& operator=(const Console&) = default;
        Console& operator=(Console&&) noexcept = default;

        size_t width() const;
        size_t height() const;

        char at(size_t i, size_t j) const;
        void set(size_t i, size_t j, char c);

        void fill(char c);

        // Highest row first, so that it reads as a map
        std::string to_string() const;

    private:

        size_t m_width = 0;
        size_t m_height = 0;
        std::vector<char> m_characters;
    };

    //--------------------------------------------------------------------------
    // A run of cells within one row
    struct Span
    {
        size_t i = 0;
        size_t j = 0;
        size_t count = 0;

        friend bool operator==(const Span&, const Span&) = default;
    };

    using spans_type = std::vector<Span>;

    // The runs in which b differs from a, in row order, lowest row first. Empty when the
    // two are the same, and the whole of b when they differ in size, which a caller
    // repainting a device should take as its cue to clear first
    spans_type difference(const Console& a, const Console& b);

} // namespace quetzal::stratum_view

#endif // QUETZAL_STRATUM_VIEW_CONSOLE_HPP
