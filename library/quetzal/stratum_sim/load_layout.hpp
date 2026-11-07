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

    // Check, before anything is built, that the world can build a zone from the document. Returns the first reason it
    // cannot, or none: an access point type that the world has not registered, or a component object that has no
    // component registered under its name or does not fit that component
    std::optional<std::string> check_buildable(const flecs::world& world, const stratum_format::Document& document);

    // Fills a zone that has no layout from a document that check_buildable accepts, and leaves it without one if the
    // document is an empty zone's. Returns the entities that references in component objects refer to
    std::vector<flecs::entity> load_layout(flecs::entity zone, const stratum_format::Document& document);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_LOAD_LAYOUT_HPP
