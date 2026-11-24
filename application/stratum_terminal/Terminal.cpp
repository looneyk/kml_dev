//------------------------------------------------------------------------------
// stratum_terminal
// Terminal.cpp
//------------------------------------------------------------------------------

#include "Terminal.hpp"
#include <algorithm>
#include <chrono>
#include <format>
#include <iostream>
#include <thread>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

using namespace quetzal;

namespace
{

    static constexpr auto escape_clear_string = "\x1b[2J\x1b[H";
    static constexpr auto escape_erase_line_string = "\x1b[K";
    static constexpr auto escape_cursor_hide_string = "\x1b[?25l";
    static constexpr auto escape_cursor_show_string = "\x1b[?25h";

    // The alternate screen buffer, leaving the shell's screen and scrollback untouched
    static constexpr auto escape_screen_alternate_string = "\x1b[?1049h";
    static constexpr auto escape_screen_primary_string = "\x1b[?1049l";
    static constexpr auto escape_position_pattern = "\x1b[{};{}H"; // Row and column both count from one

    // The map, then a blank, then the status, then the help
    static constexpr size_t row_count_below_map = 3;

    // Polls for a resize to take effect. Generous, as it runs out only when the terminal does not comply
    static constexpr int wait_count_size = 100;
    static constexpr auto wait_interval_size = std::chrono::milliseconds(10);

    // Time for the terminal to act on the last escape before the process exits
    static constexpr auto wait_settle_size = std::chrono::milliseconds(250);

    // Time for the terminal to finish resizing before the first frame is painted
    static constexpr auto wait_resize_size = std::chrono::milliseconds(400);

    //--------------------------------------------------------------------------
    // Windows Terminal enables this itself, but conhost does not
    void enable_virtual_terminal()
    {
#if defined(_WIN32)

        HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;

        if (GetConsoleMode(handle, &mode))
        {
            SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }

#endif
        return;
    }

} // namespace

//------------------------------------------------------------------------------
stratum_terminal::discrete_vector2_type stratum_terminal::terminal_size()
{
#if defined(_WIN32)

    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info))
    {
        return {};
    }

    // The visible window, not the buffer
    return {static_cast<coord_type>(info.srWindow.Right - info.srWindow.Left + 1),
        static_cast<coord_type>(info.srWindow.Bottom - info.srWindow.Top + 1)};

#else

    winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0 || ws.ws_row == 0)
    {
        return {};
    }

    return {static_cast<coord_type>(ws.ws_col), static_cast<coord_type>(ws.ws_row)};

#endif
}

//------------------------------------------------------------------------------
bool stratum_terminal::set_terminal_size(const discrete_vector2_type& size)
{
    if (size.x() <= 0 || size.y() <= 0)
    {
        return false;
    }

    // XTWINOPS, an xterm extension Windows Terminal also honors, which keeps the scrollback that the Windows console API
    // discards. Height then width
    std::cout << std::format("\x1b[8;{};{}t", size.y(), size.x()) << std::flush;

    // The request has no reply, so the size is polled
    for (int i = 0; i < wait_count_size; ++i)
    {
        if (terminal_size() == size)
        {
            return true;
        }

        std::this_thread::sleep_for(wait_interval_size);
    }

    return false;
}

//------------------------------------------------------------------------------
stratum_terminal::Terminal::Terminal(bool incremental) :
    m_console_previous(),
    m_size_previous(),
    m_size_original(),
    m_size_set(),
    m_incremental(incremental)
{
    if (m_incremental)
    {
        // Before any sizing, so the shell's screen keeps its size and scrollback
        enable_virtual_terminal();
        std::cout << escape_screen_alternate_string << escape_cursor_hide_string << std::flush;
    }
}

//------------------------------------------------------------------------------
stratum_terminal::Terminal::~Terminal()
{
    if (!m_incremental)
    {
        return;
    }

    // Only when it is still the size set, so a manual resize is kept. Windows Terminal does not act on it: the
    // pseudoconsole reports the new size while the window keeps the old one
    bool restore = !m_size_set.zero() && terminal_size() == m_size_set;

    if (restore)
    {
        set_terminal_size(m_size_original);
        std::this_thread::sleep_for(wait_settle_size);
    }

    std::cout << escape_cursor_show_string << escape_screen_primary_string << std::flush;

    // The console API reports for the pseudoconsole and marks a request done before the terminal has read it, so exiting at
    // once would lose the last escapes
    std::this_thread::sleep_for(wait_settle_size);
}

//------------------------------------------------------------------------------
void stratum_terminal::Terminal::draw(const stratum_view::Console& console, const std::string& status, const std::string& help)
{
    if (!m_incremental)
    {
        std::cout << '\n' << console.to_string() << '\n' << status << '\n' << help << '\n' << std::flush;
        return;
    }

    const discrete_vector2_type& extent = console.extent();

    coord_type rows_wanted = extent.y() + row_count_below_map;
    coord_type columns_wanted = std::max({extent.x(),
        static_cast<coord_type>(status.size()), static_cast<coord_type>(help.size())});

    // Once, at the first frame, when the zone's size is first known
    if (!m_sized)
    {
        m_sized = true;
        m_size_original = terminal_size();

        // Only grown, as Windows Terminal does not restore the size on exit
        discrete_vector2_type wanted{std::max(columns_wanted, m_size_original.x()), std::max(rows_wanted, m_size_original.y())};

        if (wanted != m_size_original && set_terminal_size(wanted))
        {
            m_size_set = terminal_size(); // Clamping included

            // The pseudoconsole reports the new size before the terminal has resized, and a frame painted too early
            // scrolls off the top
            std::this_thread::sleep_for(wait_resize_size);
            redraw();
        }
    }

    // A resize reflows and scrolls the screen, so the cached frame no longer matches it
    discrete_vector2_type size = terminal_size();
    if (size != m_size_previous)
    {
        // A manual resize; the size is read again once the terminal has settled
        if (!m_size_previous.zero())
        {
            std::this_thread::sleep_for(wait_settle_size);
            size = terminal_size();
        }

        m_size_previous = size;
        redraw();
    }

    // Zero means the size is unknown, so the frame is drawn anyway
    if (!size.zero() && (size.y() < rows_wanted || size.x() < columns_wanted))
    {
        std::cout << escape_clear_string
            << std::format("This terminal is {} rows by {} columns.\n", size.y(), size.x())
            << std::format("The frame needs {} rows by {} columns.\n\n", rows_wanted, columns_wanted)
            << "Resize the window, then press a key." << std::flush;

        redraw();
        return;
    }

    // A size change, the first frame included, cannot be patched into what is already there
    if (m_console_previous.extent() != extent)
    {
        std::cout << escape_clear_string << console.to_string()
            << '\n' << status << escape_erase_line_string
            << '\n' << help;
    }
    else
    {
        for (const stratum_view::Span& span : stratum_view::difference(m_console_previous, console))
        {
            // Console row zero is the lowest, terminal row one the highest
            std::cout << std::format(escape_position_pattern, extent.y() - span.position.y(), span.position.x() + 1);

            for (auto i = span.position.x(); i < span.position.x() + span.count; ++i)
            {
                std::cout << console.at({i, span.position.y()});
            }
        }

        // The status varies in length, so the rest of its line is erased
        std::cout << std::format(escape_position_pattern, extent.y() + 2, 1) << status << escape_erase_line_string;
    }

    std::cout << std::flush;
    m_console_previous = console;

    return;
}

//------------------------------------------------------------------------------
void stratum_terminal::Terminal::redraw()
{
    m_console_previous = stratum_view::Console();
    return;
}

