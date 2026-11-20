//------------------------------------------------------------------------------
// stratum_format
// TransitEndpoint.cpp
//------------------------------------------------------------------------------

#include "TransitEndpoint.hpp"
#include <ostream>

//------------------------------------------------------------------------------
std::ostream& quetzal::stratum_format::operator<<(std::ostream& os, const TransitEndpoint& endpoint)
{
    os << endpoint.type << " " << endpoint.id;

    if (endpoint.name.has_value())
    {
        os << "\t" << *endpoint.name;
    }

    if (endpoint.directionality.has_value())
    {
        os << "\t" << name(*endpoint.directionality);
    }

    os << "\tcell (" << endpoint.cell_position << ")\torientation (" << endpoint.orientation << ")\tegress direction ("
        << endpoint.egress_direction << ")" << std::endl;
    return os;
}
