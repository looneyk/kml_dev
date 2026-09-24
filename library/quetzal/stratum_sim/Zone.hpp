#if !defined(QUETZAL_STRATUM_SIM_ZONE_HPP)
#define QUETZAL_STRATUM_SIM_ZONE_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Zone.hpp
//------------------------------------------------------------------------------

#include "Layout.hpp"
#include "quetzal/stratum_format/Notation.hpp"
#include <memory>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    struct Zone // Tag, an independently loadable area
    {
    };

    //--------------------------------------------------------------------------
    // On a zone: its geometry and topology
    struct ZoneLayout
    {
        std::unique_ptr<Layout> layout;
    };

    //--------------------------------------------------------------------------
    // On a zone: the notation its layout is written in, from which a host chooses a view
    struct ZoneNotation
    {
        stratum_format::Notation notation = stratum_format::Notation::Block;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_ZONE_HPP
