#if !defined(QUETZAL_LIGHTMATRIX_ARRANGEMENT_HPP)
#define QUETZAL_LIGHTMATRIX_ARRANGEMENT_HPP
//------------------------------------------------------------------------------
// Light Matrix Simulation
// Arrangement.hpp
//------------------------------------------------------------------------------

#include <format>
#include <ostream>
#include <string>
#include <cassert>

namespace quetzal::lightmatrix
{

    enum class Arrangement
    {
        Rectangular,
        Radial,
    };

    std::string to_string(const Arrangement& arrangement);

    Arrangement to_arrangement(const std::string& str);

    std::ostream& operator<<(std::ostream& os, const Arrangement& arrangement);

} // namespace quetzal::lightmatrix

//------------------------------------------------------------------------------
inline std::string quetzal::lightmatrix::to_string(const Arrangement& arrangement)
{
    switch (arrangement)
    {
        case Arrangement::Rectangular:
            return "rectangular";
        case Arrangement::Radial:
            return "radial";
        default:
            assert(false && "Invalid Arrangement value");
            return "";
    }
}

//------------------------------------------------------------------------------
inline quetzal::lightmatrix::Arrangement quetzal::lightmatrix::to_arrangement(const std::string& str)
{
    return str == "radial" ? Arrangement::Radial : Arrangement::Rectangular;
}

//------------------------------------------------------------------------------
inline std::ostream& quetzal::lightmatrix::operator<<(std::ostream& os, const Arrangement& arrangement)
{
    os << to_string(arrangement);
    return os;
}

//------------------------------------------------------------------------------
template<>
struct std::formatter<quetzal::lightmatrix::Arrangement> : std::formatter<std::string>
{
    auto format(const quetzal::lightmatrix::Arrangement& arrangement, std::format_context& ctx) const
    {
        return std::formatter<std::string>::format(to_string(arrangement), ctx);
    }
};

#endif // QUETZAL_LIGHTMATRIX_ARRANGEMENT_HPP
