#if !defined(QUETZAL_WAVEFRONT_OBJ_READER_UTIL_HPP)
#define QUETZAL_WAVEFRONT_OBJ_READER_UTIL_HPP
//------------------------------------------------------------------------------
// wavefront_obj
// reader_util.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/string_util.hpp"
#include <array>
#include <string>
#include <vector>
#include <cassert>

namespace quetzal::wavefront_obj
{

    using vertex_reference_type = std::array<long, 3>;

    //--------------------------------------------------------------------------
    enum class empties
    {
        allowed,
        rejected
    };

    std::vector<std::string> tokenize(const std::string& s, std::string delimiters = " ", empties e = empties::rejected);

    bool parse_vertex_reference(const std::string& s, vertex_reference_type& vr);

    template<typename T, size_t N>
    bool parse_components(const std::vector<std::string>& tokens, std::array<T, N>& values, size_t nMin, T tDefault = T(0));

} // namespace quetzal::wavefront_obj

//--------------------------------------------------------------------------
template<typename T, size_t N>
bool quetzal::wavefront_obj::parse_components(const std::vector<std::string>& tokens, std::array<T, N>& values, size_t nRequired, T tDefault)
{
    static_assert(N >= 3 && N <= 4);

    // First token is the keyword
    if (tokens.size() < (nRequired + 1) || tokens.size() > (N + 1))
    {
        return false;
    }

    values[0] = quetzal::to_type<T>(tokens[1]);
    values[1] = tokens.size() > 2 ? quetzal::to_type<T>(tokens[2]) : tDefault;
    values[2] = tokens.size() > 3 ? quetzal::to_type<T>(tokens[3]) : tDefault;

    if constexpr (N > 3)
    {
        values[3] = tokens.size() > 4 ? quetzal::to_type<T>(tokens[4]) : tDefault;
    }

    return true;
}

#endif // QUETZAL_WAVEFRONT_OBJ_READER_UTIL_HPP
