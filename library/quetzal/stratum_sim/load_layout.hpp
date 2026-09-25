#if !defined(QUETZAL_STRATUM_SIM_LOAD_LAYOUT_HPP)
#define QUETZAL_STRATUM_SIM_LOAD_LAYOUT_HPP
//------------------------------------------------------------------------------
// stratum_sim
// load_layout.hpp
//------------------------------------------------------------------------------

#include "quetzal/stratum_format/Document.hpp"
#include <flecs.h>
#include <optional>
#include <string>

namespace quetzal::stratum_sim
{

    // The first access point type the document refers to that the world does not register
    std::optional<std::string> unregistered_access_type(const flecs::world& world, const stratum_format::Document& document);

    // Fills an empty zone from a document: its ZoneLayout, with a cell per grid position, its walls, the room of each cell
    // and the two flats; its ZoneNotation, the notation the rows are written in; its doors; and its access points. Every
    // access point type the document refers to must be registered
    void load_layout(flecs::entity zone, const stratum_format::Document& document);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_LOAD_LAYOUT_HPP
