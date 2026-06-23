#if !defined(QUETZAL_LIGHTMATRIX_SMOOTHING_HPP)
#define QUETZAL_LIGHTMATRIX_SMOOTHING_HPP
//------------------------------------------------------------------------------
// Light Matrix Simulation
// Smoothing.hpp
//------------------------------------------------------------------------------

#include <format>
#include <ostream>
#include <string>
#include <cassert>

namespace quetzal::lightmatrix
{

    //----------------------------------------------------------------------
    enum class Smoothing
    {
        Nearest,
        Linear,
        Cubic
    };

    std::string to_string(const Smoothing& smoothing);

    Smoothing to_smoothing(const std::string& str);

    std::ostream& operator<<(std::ostream& os, const Smoothing& smoothing);

} // namespace quetzal::lightmatrix

//------------------------------------------------------------------------------
inline std::string quetzal::lightmatrix::to_string(const Smoothing& smoothing)
{
    switch (smoothing)
    {
        case Smoothing::Nearest:
            return "nearest";
        case Smoothing::Linear:
            return "linear";
        case Smoothing::Cubic:
            return "cubic";
        default:
            assert(false && "Invalid Smoothing value");
            return "";
    }
}

//------------------------------------------------------------------------------
inline quetzal::lightmatrix::Smoothing quetzal::lightmatrix::to_smoothing(const std::string& str)
{
    if (str == "linear")
    {
        return Smoothing::Linear;
    }
    else if (str == "cubic")
    {
        return Smoothing::Cubic;
    }

    return Smoothing::Nearest;
}

//------------------------------------------------------------------------------
inline std::ostream& quetzal::lightmatrix::operator<<(std::ostream& os, const Smoothing& smoothing)
{
    os << to_string(smoothing);
    return os;
}

//------------------------------------------------------------------------------
template<>
struct std::formatter<quetzal::lightmatrix::Smoothing> : std::formatter<std::string>
{
    auto format(const quetzal::lightmatrix::Smoothing& smoothing, std::format_context& ctx) const
    {
        return std::formatter<std::string>::format(to_string(smoothing), ctx);
    }
};

#endif // QUETZAL_LIGHTMATRIX_SMOOTHING_HPP
