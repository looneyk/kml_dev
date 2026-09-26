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

    // The first reason the world cannot build a zone from the document: an access point type it does not register, or a
    // component object with no component registered under its name or that does not fit it
    std::optional<std::string> build_error(const flecs::world& world, const stratum_format::Document& document);

    // Fills an empty zone from a document build_error accepts: its ZoneLayout, with a cell per grid position, its walls,
    // the room of each cell and the two flats; its ZoneNotation, the notation the rows are written in; and every entity,
    // each component object decoded. Returns the entities a reference in a component object refers to
    std::vector<flecs::entity> load_layout(flecs::entity zone, const stratum_format::Document& document);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_LOAD_LAYOUT_HPP
