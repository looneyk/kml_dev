#if !defined(QUETZAL_STRATUM_SIM_SCRIPT_LIMITS_HPP)
#define QUETZAL_STRATUM_SIM_SCRIPT_LIMITS_HPP
//------------------------------------------------------------------------------
// stratum_sim
// ScriptLimits.hpp
//------------------------------------------------------------------------------

#include <cstddef>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // The limits on the work of a script sandbox's code, so that the code cannot stop the game or exhaust its memory
    struct ScriptLimits
    {
        std::size_t memory_size_max = 0;       // The maximum byte count that the sandbox's code allocates
        std::size_t instruction_count_max = 0; // The maximum number of Lua instructions that one evaluation runs
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_SCRIPT_LIMITS_HPP
