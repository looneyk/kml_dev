#if !defined(QUETZAL_STRATUM_SIM_LOAD_LAYOUT_HPP)
#define QUETZAL_STRATUM_SIM_LOAD_LAYOUT_HPP
//------------------------------------------------------------------------------
// stratum_sim
// load_layout.hpp
//------------------------------------------------------------------------------

#include <flecs.h>
#include <optional>
#include <string>
#include <vector>

namespace quetzal::stratum_format
{
    struct Document;
}

namespace quetzal::stratum_sim
{

    // Returns the first reason the world cannot build a zone from the document, or none. The reason is an access point type
    // that the world has not registered, or a component object that has no component registered under its name or does
    // not fit that component
    std::optional<std::string> build_error(const flecs::world& world, const stratum_format::Document& document);

    // Fills an empty zone from a document that build_error accepts. It sets the ZoneLayout, with the cells, walls, rooms,
    // and both flats, and the ZoneNotation, and creates every entity with its component objects decoded. Returns the
    // entities that references in component objects refer to
    std::vector<flecs::entity> load_layout(flecs::entity zone, const stratum_format::Document& document);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_LOAD_LAYOUT_HPP
