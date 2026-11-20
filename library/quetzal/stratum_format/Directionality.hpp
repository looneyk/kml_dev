#if !defined(QUETZAL_STRATUM_FORMAT_DIRECTIONALITY_HPP)
#define QUETZAL_STRATUM_FORMAT_DIRECTIONALITY_HPP
//------------------------------------------------------------------------------
// stratum_format
// Directionality.hpp
//------------------------------------------------------------------------------

#include <optional>
#include <string_view>

namespace quetzal::stratum_format
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

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_DIRECTIONALITY_HPP
