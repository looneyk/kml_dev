//------------------------------------------------------------------------------
// stratum_core
// Directionality.cpp
//------------------------------------------------------------------------------

#include "Directionality.hpp"
#include <algorithm>
#include <array>
#include <utility>
#include <cassert>

namespace
{

    using quetzal::stratum_core::Directionality;

    constexpr std::array<std::pair<Directionality, std::string_view>, 2> directionality_names
    {{
        {Directionality::Up, "Up"},
        {Directionality::Down, "Down"}
    }};

} // namespace

//------------------------------------------------------------------------------
std::string_view quetzal::stratum_core::name(Directionality directionality)
{
    const auto named = std::ranges::find(directionality_names, directionality, &std::pair<Directionality, std::string_view>::first);
    assert(named != directionality_names.end());
    return named->second;
}

//------------------------------------------------------------------------------
std::optional<quetzal::stratum_core::Directionality> quetzal::stratum_core::to_directionality(std::string_view name)
{
    const auto named = std::ranges::find(directionality_names, name, &std::pair<Directionality, std::string_view>::second);
    return named != directionality_names.end() ? std::optional<Directionality>(named->first) : std::nullopt;
}
