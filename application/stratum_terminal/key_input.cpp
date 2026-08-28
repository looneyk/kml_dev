//------------------------------------------------------------------------------
// stratum_terminal
// key_input.cpp
//------------------------------------------------------------------------------

#include "key_input.hpp"

#if defined(_WIN32)

#include <conio.h>

//------------------------------------------------------------------------------
stratum_terminal::KeyPress stratum_terminal::read_key()
{
    int c = _getch();

    // An arrow arrives as a lead byte followed by its own code
    if (c == 0x00 || c == 0xE0)
    {
        switch (_getch())
        {
            case 72: return {Key::Up};
            case 80: return {Key::Down};
            case 75: return {Key::Left};
            case 77: return {Key::Right};
        }

        return {Key::None};
    }

    if (c == 0x1B)
    {
        return {Key::Escape};
    }

    return {Key::Character, static_cast<char>(c)};
}

#else

#include <termios.h>
#include <unistd.h>

namespace
{

    //--------------------------------------------------------------------------
    // Unbuffered and unechoed for as long as it is in scope
    class RawMode
    {
    public:

        RawMode()
        {
            tcgetattr(STDIN_FILENO, &m_termios);

            termios raw = m_termios;
            raw.c_lflag &= ~static_cast<tcflag_t>(ICANON | ECHO);
            tcsetattr(STDIN_FILENO, TCSANOW, &raw);
        }

        RawMode(const RawMode&) = delete;
        RawMode& operator=(const RawMode&) = delete;

        ~RawMode()
        {
            tcsetattr(STDIN_FILENO, TCSANOW, &m_termios);
        }

    private:

        termios m_termios;
    };

    //--------------------------------------------------------------------------
    bool read_char(char& c)
    {
        return ::read(STDIN_FILENO, &c, 1) == 1;
    }

} // namespace

//------------------------------------------------------------------------------
stratum_terminal::KeyPress stratum_terminal::read_key()
{
    RawMode raw_mode;

    char c = '\0';
    if (!read_char(c))
    {
        return {Key::None};
    }

    // An arrow arrives as the three byte sequence ESC [ code
    if (c == '\x1b')
    {
        char bracket = '\0';
        char code = '\0';

        if (!read_char(bracket) || bracket != '[' || !read_char(code))
        {
            return {Key::Escape};
        }

        switch (code)
        {
            case 'A': return {Key::Up};
            case 'B': return {Key::Down};
            case 'D': return {Key::Left};
            case 'C': return {Key::Right};
        }

        return {Key::None};
    }

    return {Key::Character, c};
}

#endif
