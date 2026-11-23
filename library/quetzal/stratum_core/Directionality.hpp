#if !defined(QUETZAL_STRATUM_CORE_DIRECTIONALITY_HPP)
#define QUETZAL_STRATUM_CORE_DIRECTIONALITY_HPP
//------------------------------------------------------------------------------
// stratum_core
// Directionality.hpp
//------------------------------------------------------------------------------

#include <optional>
#include <string_view>

namespace quetzal::stratum_core
{

    //--------------------------------------------------------------------------
    // The direction in which a transit endpoint leads
    enum class Directionality
    {
        Up,
        Down
    };

    std::string_view name(Directionality directionality);
    std::optional<Directionality> to_directionality(std::string_view name);

} // namespace quetzal::stratum_core

#endif // QUETZAL_STRATUM_CORE_DIRECTIONALITY_HPP
