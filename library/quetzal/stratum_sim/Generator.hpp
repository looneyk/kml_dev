#if !defined(QUETZAL_STRATUM_SIM_GENERATOR_HPP)
#define QUETZAL_STRATUM_SIM_GENERATOR_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Generator.hpp
//------------------------------------------------------------------------------

#include <flecs.h>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Builds a zone's contents in C++. Each implementation declares its own parameters
    class Generator
    {
    public:

        Generator() = default;
        Generator(const Generator&) = default;
        virtual ~Generator() = default;

        Generator& operator=(const Generator&) = default;

        // Fills an empty zone as load_layout does: its ZoneLayout, its ZoneNotation, its layout entities, and its entities
        virtual void generate(flecs::entity zone) const = 0;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_GENERATOR_HPP
