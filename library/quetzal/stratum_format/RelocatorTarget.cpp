//------------------------------------------------------------------------------
// stratum_format
// RelocatorTarget.cpp
//------------------------------------------------------------------------------

#include "RelocatorTarget.hpp"
#include <algorithm>
#include <charconv>
#include <format>
#include <system_error>
#include <utility>
#include <cctype>

namespace
{

    using namespace quetzal::stratum_format;

    //--------------------------------------------------------------------------
    // A letter followed by letters, digits, and underscores
    bool is_name(std::string_view text)
    {
        return !text.empty() && std::isalpha(static_cast<unsigned char>(text.front()))
            && std::ranges::all_of(text, [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; });
    }

    //--------------------------------------------------------------------------
    // Returns the level written as digits with an optional leading '-', or nothing if the text is not that or is out of range
    std::optional<level_type> parse_level(std::string_view text)
    {
        std::string_view digits = text.starts_with('-') ? text.substr(1) : text;
        if (digits.empty() || !std::ranges::all_of(digits, [](char c) { return c >= '0' && c <= '9'; }))
        {
            return std::nullopt;
        }

        level_type level = 0;
        auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), level);
        if (error != std::errc {} || end != text.data() + text.size())
        {
            return std::nullopt;
        }

        return level;
    }

    //--------------------------------------------------------------------------
    std::optional<std::variant<RelativeZone, AbsoluteZone, std::string>> parse_zone(std::string_view text)
    {
        if (text.starts_with('='))
        {
            if (std::optional<level_type> level = parse_level(text.substr(1)); level.has_value())
            {
                return AbsoluteZone {*level};
            }
        }
        else if (text.starts_with('-') || (text.starts_with('+') && !text.substr(1).starts_with('-')))
        {
            if (std::optional<level_type> offset = parse_level(text.starts_with('+') ? text.substr(1) : text); offset.has_value())
            {
                return RelativeZone {*offset};
            }
        }
        else if (is_name(text))
        {
            return std::string(text);
        }

        return std::nullopt;
    }

} // namespace

//------------------------------------------------------------------------------
std::optional<quetzal::stratum_format::RelocatorTarget> quetzal::stratum_format::parse_relocator_target(std::string_view text)
{
    RelocatorTarget target;

    if (size_t at = text.find('@'); at != std::string_view::npos)
    {
        std::string_view position = text.substr(at + 1);
        if (position.empty() || position.find_first_of("/@") != std::string_view::npos)
        {
            return std::nullopt;
        }

        target.position = std::string(position);
        text = text.substr(0, at);
    }

    if (size_t slash = text.find('/'); slash != std::string_view::npos)
    {
        std::string_view region = text.substr(0, slash);
        if (region != "." && region != ".." && !is_name(region))
        {
            return std::nullopt;
        }

        target.region = std::string(region);
        text = text.substr(slash + 1);
    }

    auto zone = parse_zone(text);
    if (!zone.has_value())
    {
        return std::nullopt;
    }

    target.zone = std::move(*zone);
    return target;
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_format::format_relocator_target(const RelocatorTarget& target)
{
    std::string text = target.region.has_value() ? *target.region + "/" : std::string();

    if (const auto* relative = std::get_if<RelativeZone>(&target.zone))
    {
        text += std::format("{:+}", relative->offset);
    }
    else if (const auto* absolute = std::get_if<AbsoluteZone>(&target.zone))
    {
        text += std::format("={}", absolute->level);
    }
    else
    {
        text += std::get<std::string>(target.zone);
    }

    if (target.position.has_value())
    {
        text += "@" + *target.position;
    }

    return text;
}
