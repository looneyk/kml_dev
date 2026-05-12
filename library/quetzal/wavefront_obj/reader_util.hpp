#if !defined(QUETZAL_WAVEFRONT_OBJ_READER_UTIL_HPP)
#define QUETZAL_WAVEFRONT_OBJ_READER_UTIL_HPP
//------------------------------------------------------------------------------
// wavefront_obj
// reader_util.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/string_util.hpp"
#include <array>
#include <limits>
#include <string>
#include <vector>

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
inline std::vector<std::string> quetzal::wavefront_obj::tokenize(const std::string& s, std::string delimiters, empties e)
{
    std::vector<std::string> tokens;
    size_t pos;
    size_t pos_next = std::numeric_limits<size_t>::max();
    do
    {
        if (e == empties::rejected)
        {
            pos_next = s.find_first_not_of(delimiters, pos_next + 1);
            if (pos_next == std::string::npos)
            {
                break;
            }
            pos_next -= 1;
        }

        pos = pos_next + 1;
        pos_next = s.find_first_of(delimiters, pos);
        tokens.push_back(s.substr(pos, pos_next - pos));
    }
    while (pos_next != std::string::npos);

    return tokens;
}

//--------------------------------------------------------------------------
inline bool quetzal::wavefront_obj::parse_vertex_reference(const std::string& s, vertex_reference_type& vr)
{
    auto tokens = tokenize(s, "/", empties::allowed);
    if (tokens.empty() || tokens.size() > 3)
    {
        return false;
    }

    vr[0] = std::stol(tokens[0]);

    if (tokens.size() > 1)
    {
        vr[1] = std::stol(tokens[1]);
    }

    if (tokens.size() > 2)
    {
        vr[2] = std::stol(tokens[2]);
    }

    return true;
}

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
