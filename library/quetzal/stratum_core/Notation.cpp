//------------------------------------------------------------------------------
// stratum_core
// Notation.cpp
//------------------------------------------------------------------------------

#include "Notation.hpp"
#include <algorithm>
#include <array>
#include <utility>
#include <cassert>

namespace
{

    using quetzal::stratum_core::Notation;

    constexpr std::array<std::pair<Notation, std::string_view>, 2> notation_names
    {{
        {Notation::Block, "block"},
        {Notation::Wall, "wall"}
    }};

} // namespace

//------------------------------------------------------------------------------
std::string_view quetzal::stratum_core::name(Notation notation)
{
    const auto named = std::ranges::find(notation_names, notation, &std::pair<Notation, std::string_view>::first);
    assert(named != notation_names.end());
    return named->second;
}

//------------------------------------------------------------------------------
std::optional<quetzal::stratum_core::Notation> quetzal::stratum_core::to_notation(std::string_view name)
{
    const auto named = std::ranges::find(notation_names, name, &std::pair<Notation, std::string_view>::second);
    return named != notation_names.end() ? std::optional<Notation>(named->first) : std::nullopt;
}
