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
    // Constructs a zone's geometry. Each implementation declares its own parameters
    class Generator
    {
    public:

        Generator() = default;
        Generator(const Generator&) = default;
        virtual ~Generator() = default;

        Generator& operator=(const Generator&) = default;

        // Fills an empty zone: its ZoneLayout, its ZoneNotation and its doors
        virtual void generate(flecs::entity zone) const = 0;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_GENERATOR_HPP
