#if !defined(QUETZAL_STRATUM_FORMAT_TRANSITENDPOINT_HPP)
#define QUETZAL_STRATUM_FORMAT_TRANSITENDPOINT_HPP
//------------------------------------------------------------------------------
// stratum_format
// TransitEndpoint.hpp
//------------------------------------------------------------------------------

#include "Directionality.hpp"
#include "types.hpp"
#include "quetzal/common/id.hpp"
#include <iosfwd>
#include <optional>
#include <string>

namespace quetzal::stratum_format
{

    //--------------------------------------------------------------------------
    // One end of an inter-zone transit pair
    struct TransitEndpoint
    {
        id_type id = nullid;
        std::optional<std::string> name;
        std::string type;
        discrete_point_type cell_position;
        discrete_vector_type orientation;
        discrete_vector_type egress_direction;
        std::optional<Directionality> directionality;

        friend bool operator==(const TransitEndpoint&, const TransitEndpoint&) = default;
    };

    std::ostream& operator<<(std::ostream& os, const TransitEndpoint& endpoint);

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_TRANSITENDPOINT_HPP
