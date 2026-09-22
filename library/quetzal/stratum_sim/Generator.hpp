#if !defined(QUETZAL_STRATUM_SIM_GENERATOR_HPP)
#define QUETZAL_STRATUM_SIM_GENERATOR_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Generator.hpp
//------------------------------------------------------------------------------

#include "Layout.hpp"
#include "load_layout.hpp"
#include <memory>

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

        // Reports the walls of each door to door, having no world to create it in
        virtual std::unique_ptr<Layout> generate(const door_type& door) const = 0;

        virtual stratum_format::Notation notation() const = 0;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_GENERATOR_HPP
