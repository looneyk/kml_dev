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
    // What constructs a zone. An interface here and an implementation wherever a generator
    // was chosen: this library links neither of them, and which one runs decides which
    // parameters apply, so the concrete generator declares its own
    // A zone is constructed whole when it is encountered, so this is the general way one
    // arrives; a level file is a snapshot of part of the same construction
    class Generator
    {
    public:

        Generator() = default;
        Generator(const Generator&) = default;
        virtual ~Generator() = default;

        Generator& operator=(const Generator&) = default;

        // The geometry, reporting the walls each door is attached to rather than placing one, as
        // the loader does -- a generator has no world to place anything in either
        virtual std::unique_ptr<Layout> generate(const door_type& door) const = 0;

        // Which geometry model was built, a constructed zone having no file to supply it
        virtual stratum_format::Notation notation() const = 0;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_GENERATOR_HPP
