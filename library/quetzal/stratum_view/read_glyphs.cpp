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
    using quetzal::stratum_view::FeatureType;
    using quetzal::stratum_view::DirectionMask;
    using quetzal::stratum_sim::AccessState;
    using quetzal::stratum_sim::DetectionState;

    //--------------------------------------------------------------------------
    // Pre-release, so files of an older version are not readable
    constexpr auto format_version = "0.1";

    //--------------------------------------------------------------------------
    // Names as written in a file
    struct Named
    {
        const char* name;
        FeatureType type;
    };

    constexpr Named types[]
    {
        {"none", FeatureType::None},
        {"solid", FeatureType::Solid},
        {"floor", FeatureType::Floor},
        {"wall", FeatureType::Wall},
        {"door", FeatureType::Door},
        {"stairs_up", FeatureType::StairsUp},
        {"stairs_down", FeatureType::StairsDown}
    };

    struct NamedAccess
    {
        const char* name;
        AccessState access;
    };

    constexpr NamedAccess access_states[]
    {
        {"open", AccessState::Open},
        {"closed", AccessState::Closed},
        {"locked", AccessState::Locked}
    };

    struct NamedDetection
    {
        const char* name;
        DetectionState detection;
    };

    constexpr NamedDetection detection_states[]
    {
        {"visible", DetectionState::Visible},
        {"concealed", DetectionState::Concealed},
        {"revealed", DetectionState::Revealed}
    };

    //--------------------------------------------------------------------------
    [[noreturn]] void fail(const std::string& name, size_t row, const std::string& what)
    {
        throw std::runtime_error(std::format("{}({}): {}", name.empty() ? "glyphs" : name, row, what));
    }

    //--------------------------------------------------------------------------
    // The character a line draws, in single quotes so that a space can be a glyph
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
    DirectionMask read_mask(const std::string& token, const std::string& name, size_t row)
    {
        DirectionMask mask = DirectionMask::None;

        for (char c : token)
        {
            switch (c)
            {
                case 'E': mask |= DirectionMask::East; break;
                case 'N': mask |= DirectionMask::North; break;
                case 'W': mask |= DirectionMask::West; break;
                case 'S': mask |= DirectionMask::South; break;
                default: fail(name, row, std::format("{} is not a direction", c));
            }
        }

        return mask;
    }

    //--------------------------------------------------------------------------
    // Sets the glyph's access or detection from the token, and returns false if the token is neither
    bool read_condition(Glyph& glyph, const std::string& token, const std::string& name, size_t row)
    {
        for (const NamedAccess& access : access_states)
        {
            if (token == access.name)
            {
                if (glyph.access.has_value())
                {
                    fail(name, row, "more than one access state");
                }

                glyph.access = access.access;
                return true;
            }
        }

        for (const NamedDetection& detection : detection_states)
        {
            if (token == detection.name)
            {
                if (glyph.detection.has_value())
                {
                    fail(name, row, "more than one detection state");
                }

                glyph.detection = detection.detection;
                return true;
            }
        }

        return false;
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

    // Read whole so line endings are filtered in one place
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

        for (const Named& type : types)
        {
            if (token == type.name)
            {
                glyph.type = type.type;
                named = true;
            }
        }

        if (!named)
        {
            fail(name, row, std::format("{} is not a feature type", token));
        }

        // The quoted character is read to the end of the line, as a quoted space would split into two tokens
        size_t quote = line.find('\'');

        if (quote == std::string::npos)
        {
            fail(name, row, "no character to draw");
        }

        size_t end = line.find_last_not_of(" \t");
        glyph.c = read_character(line.substr(quote, end + 1 - quote), name, row);

        // Everything between the type and the character, in whatever order it was written
        std::istringstream rest(line.substr(0, quote));
        rest >> token; // Skips the type

        while (rest >> token)
        {
            if (token.starts_with("mask="))
            {
                glyph.mask = read_mask(token.substr(5), name, row);
                continue;
            }

            if (token == "actor")
            {
                glyph.actor = true;
                continue;
            }

            if (!read_condition(glyph, token, name, row))
            {
                fail(name, row, std::format("{} is not an access state or a detection state", token));
            }
        }

        glyphs.push_back(glyph);
    }

    if (!header)
    {
        throw std::runtime_error(std::format("{} names no format", name.empty() ? "glyphs" : name));
    }

    return Glyphs(std::move(glyphs));
}
