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

    // The alternate screen buffer, as vim, less and htop use it: the game draws on a
    // screen of its own and the shell's own screen and scrollback come back untouched
    static constexpr auto escape_screen_alternate_string = "\x1b[?1049h";
    static constexpr auto escape_screen_primary_string = "\x1b[?1049l";
    static constexpr auto escape_position_pattern = "\x1b[{};{}H"; // Row and column both count from one

    // The map, then a blank, then the status, then the help
    static constexpr size_t row_count_below_map = 3;

    // How long a resize request is given to take effect before it is called a failure.
    // Generous because it only ever costs this much when the terminal will not comply,
    // and a request that is merely slow must not be mistaken for one that was refused
    static constexpr int wait_count_size = 100;
    static constexpr auto wait_interval_size = std::chrono::milliseconds(10);

    // What the terminal is given to act on the last escape before the process exits
    static constexpr auto wait_settle_size = std::chrono::milliseconds(250);

    // And to finish resizing before the first frame is painted into it. Longer, since the
    // whole frame is wrong if it is too short, and it is paid once at startup
    static constexpr auto wait_resize_size = std::chrono::milliseconds(400);

    //--------------------------------------------------------------------------
    // Windows Terminal turns this on for itself, conhost does not, so relying on the
    // default leaves the escapes showing as text under some hosts
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
stratum_terminal::Size stratum_terminal::terminal_size()
{
#if defined(_WIN32)

    CONSOLE_SCREEN_BUFFER_INFO info {};
    if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info))
    {
        return {};
    }

    // The window rather than the buffer, which is what is actually visible
    return {static_cast<size_t>(info.srWindow.Bottom - info.srWindow.Top + 1),
        static_cast<size_t>(info.srWindow.Right - info.srWindow.Left + 1)};

#else

    winsize ws {};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0 || ws.ws_row == 0)
    {
        return {};
    }

    return {static_cast<size_t>(ws.ws_row), static_cast<size_t>(ws.ws_col)};

#endif
}

//------------------------------------------------------------------------------
bool stratum_terminal::set_terminal_size(const Size& size)
{
    if (size.rows == 0 || size.columns == 0)
    {
        return false;
    }

    // XTWINOPS, an xterm extension rather than ECMA-48, honoured by Windows Terminal as
    // well. Preferred over the Windows console API, which does the same job but discards
    // the terminal's scrollback doing it
    std::cout << std::format("\x1b[8;{};{}t", size.rows, size.columns) << std::flush;

    // The request goes unanswered, so the result is read back rather than reported, and
    // a terminal that cannot or will not comply simply never arrives at it
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
        // Before any sizing, so that the shell's own screen keeps the size and the
        // scrollback it had
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

    // Only when it is still the size it was put at, so that a resize by hand stands
    // Emitted correctly and honoured on a terminal driven directly, but Windows Terminal
    // does not act on it: the pseudoconsole reports the new size while the window keeps
    // the one the game asked for. Harmless where it does not take, so it stands
    bool restore = m_size_set.rows != 0 && terminal_size() == m_size_set;

    if (restore)
    {
        set_terminal_size(m_size_original);
        std::this_thread::sleep_for(wait_settle_size);
    }

    std::cout << escape_cursor_show_string << escape_screen_primary_string << std::flush;

    // The console API answers for the pseudoconsole rather than the terminal, and reports
    // a request as done before the terminal has read it, so exiting at once would lose the
    // last escapes
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

    size_t height = console.height();
    size_t rows_wanted = height + row_count_below_map;
    size_t columns_wanted = std::max({console.width(), status.size(), help.size()});

    // Once, at the first frame, the level's size not being known before then. A later
    // resize by hand is left alone rather than fought over
    if (!m_sized)
    {
        m_sized = true;
        m_size_original = terminal_size();

        // Grown where it falls short and left alone otherwise. Windows Terminal will not
        // take the size back on exit, so a window narrowed to fit a small level would stay
        // narrow afterwards; this way it is never returned smaller than it was found
        Size wanted {std::max(rows_wanted, m_size_original.rows), std::max(columns_wanted, m_size_original.columns)};

        if (wanted != m_size_original && set_terminal_size(wanted))
        {
            m_size_set = terminal_size(); // What it actually became, clamping included

            // The pseudoconsole reports the new size before the terminal has resized to
            // it, and a frame painted into the old one scrolls off the top. So the
            // terminal is given time to catch up and the frame is painted again after
            std::this_thread::sleep_for(wait_resize_size);
            redraw();
        }
    }

    // A resize reflows and scrolls what is on screen, so nothing already there can be
    // patched and the cached frame no longer describes anything
    Size size = terminal_size();
    if (size != m_size_previous)
    {
        // A change after the first frame is the window being resized by hand. The size is
        // read again after settling, since the pseudoconsole answers before the terminal
        // has finished, and a frame painted too early scrolls off the top
        if (m_size_previous.rows != 0)
        {
            std::this_thread::sleep_for(wait_settle_size);
            size = terminal_size();
        }

        m_size_previous = size;
        redraw();
    }

    // Zero means the size is unknown, in which case drawing is the best that can be done
    if (size.rows != 0 && (size.rows < rows_wanted || size.columns < columns_wanted))
    {
        std::cout << escape_clear_string
            << std::format("This terminal is {} rows by {} columns.\n", size.rows, size.columns)
            << std::format("The level needs {} by {}.\n\n", rows_wanted, columns_wanted)
            << "Resize the window, then press a key." << std::flush;

        redraw();
        return;
    }

    // A size change, the first frame included, cannot be patched into what is already there
    if (m_console_previous.width() != console.width() || m_console_previous.height() != height)
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
            std::cout << std::format(escape_position_pattern, height - span.j, span.i + 1);

            for (size_t i = span.i; i < span.i + span.count; ++i)
            {
                std::cout << console.at(i, span.j);
            }
        }

        // The status varies in length, so what it no longer covers has to be erased
        std::cout << std::format(escape_position_pattern, height + 2, 1) << status << escape_erase_line_string;
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

