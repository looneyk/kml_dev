//------------------------------------------------------------------------------
// stratum_view
// read_glyphs.cpp
//------------------------------------------------------------------------------

#include "read_glyphs.hpp"
#include "quetzal/common/file_util.hpp"
#include <format>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace
{

    using quetzal::stratum_view::Glyph;
    using quetzal::stratum_view::Kind;
    using quetzal::stratum_view::Mask;
    using quetzal::stratum_view::State;

    //--------------------------------------------------------------------------
    // The one format version this reads. A leading zero says pre-release: no file older than
    // it survives a change, and none is kept readable
    constexpr auto format_version = "0.1";

    //--------------------------------------------------------------------------
    // The names a file writes, which are the names the code uses for the same things
    struct Named
    {
        const char* name;
        Kind kind;
    };

    constexpr Named kinds[]
    {
        {"rock", Kind::Rock},
        {"floor", Kind::Floor},
        {"actor", Kind::Actor},
        {"wall", Kind::Wall},
        {"door", Kind::Door}
    };

    struct NamedState
    {
        const char* name;
        State state;
    };

    constexpr NamedState states[]
    {
        {"open", State::Open},
        {"passable", State::Passable},
        {"concealed", State::Concealed},
        {"solid", State::Solid}
    };

    //--------------------------------------------------------------------------
    [[noreturn]] void fail(const std::string& name, size_t row, const std::string& what)
    {
        throw std::runtime_error(std::format("{}({}): {}", name.empty() ? "glyphs" : name, row, what));
    }

    //--------------------------------------------------------------------------
    // The character a line draws, in single quotes so that a space is a glyph like any other
    char read_character(const std::string& token, const std::string& name, size_t row)
    {
        if (token.size() < 3 || token.front() != '\'' || token.back() != '\'')
        {
            fail(name, row, std::format("expected a character in single quotes, got {}", token));
        }

        std::string body = token.substr(1, token.size() - 2);

        if (body.size() == 2 && body[0] == '\\' && (body[1] == '\\' || body[1] == '\''))
        {
            return body[1];
        }

        if (body.size() != 1)
        {
            fail(name, row, std::format("expected one character, got {}", token));
        }

        return body[0];
    }

    //--------------------------------------------------------------------------
    Mask read_mask(const std::string& token, const std::string& name, size_t row)
    {
        Mask mask = Mask::None;

        for (char c : token)
        {
            switch (c)
            {
                case 'E': mask |= Mask::East; break;
                case 'N': mask |= Mask::North; break;
                case 'W': mask |= Mask::West; break;
                case 'S': mask |= Mask::South; break;
                default: fail(name, row, std::format("{} is not a direction", c));
            }
        }

        return mask;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_view::Glyphs quetzal::stratum_view::read_glyphs(const std::filesystem::path& filepath)
{
    std::ifstream is(filepath, std::ios_base::binary);

    if (!is)
    {
        throw std::runtime_error(std::format("{} cannot be opened", filepath.string()));
    }

    // Read whole so that line endings are filtered in the one place that does it, whatever
    // wrote the file
    std::string text;
    std::string line;

    while (read_line(is, line))
    {
        text += line;
        text += '\n';
    }

    std::istringstream lines(text);
    return read_glyphs(lines, filepath.filename().string());
}

//------------------------------------------------------------------------------
quetzal::stratum_view::Glyphs quetzal::stratum_view::read_glyphs(std::istream& is_glyphs, const std::string& name)
{
    std::vector<Glyph> glyphs;

    std::string line;
    size_t row = 0;
    bool header = false;

    while (read_line(is_glyphs, line))
    {
        ++row;

        std::istringstream is(line);
        std::string token;

        if (!(is >> token) || token.front() == '#')
        {
            continue;
        }

        if (!header)
        {
            std::string version;
            is >> version;

            if (token != "glyphs" || version != format_version)
            {
                fail(name, row, std::format("expected glyphs {}, got {} {}", format_version, token, version));
            }

            header = true;
            continue;
        }

        Glyph glyph;
        bool named = false;

        for (const Named& kind : kinds)
        {
            if (token == kind.name)
            {
                glyph.kind = kind.kind;
                named = true;
            }
        }

        if (!named)
        {
            fail(name, row, std::format("{} is not a kind", token));
        }

        // The character runs to the end of the line rather than being a token of its own,
        // a quoted space being two tokens to anything reading words
        size_t quote = line.find('\'');

        if (quote == std::string::npos)
        {
            fail(name, row, "no character to draw");
        }

        size_t end = line.find_last_not_of(" \t");
        glyph.c = read_character(line.substr(quote, end + 1 - quote), name, row);

        // Everything between the kind and the character, in whatever order it was written
        std::istringstream rest(line.substr(0, quote));
        rest >> token; // the kind, read again to step over it

        while (rest >> token)
        {
            if (token.starts_with("mask="))
            {
                glyph.mask = read_mask(token.substr(5), name, row);
                continue;
            }

            named = false;

            for (const NamedState& state : states)
            {
                if (token == state.name)
                {
                    glyph.state |= state.state;
                    named = true;
                }
            }

            if (!named)
            {
                fail(name, row, std::format("{} is not a state", token));
            }
        }

        // A state a kind can never be in would be a line that never matches, which is a
        // mistake in the file rather than a set with nothing to say
        if (!applies(glyph.kind, glyph.state))
        {
            fail(name, row, "a state that kind cannot be in");
        }

        glyphs.push_back(glyph);
    }

    if (!header)
    {
        throw std::runtime_error(std::format("{} names no format", name.empty() ? "glyphs" : name));
    }

    return Glyphs(std::move(glyphs));
}
