#if !defined(QUETZAL_WAVEFRONT_OBJ_READER_UTIL_HPP)
#define QUETZAL_WAVEFRONT_OBJ_READER_UTIL_HPP
//------------------------------------------------------------------------------
// wavefront_obj
// reader_util.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/string_util.hpp"
#include <array>
#include <charconv>
#include <limits>
#include <span>
#include <string>
#include <string_view>
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

    // Tokenizes into views over s, reusing the caller's buffer so that repeated
    // calls over the lines of a file do not allocate
    void tokenize(std::string_view s, std::vector<std::string_view>& tokens, std::string_view delimiters = " ", empties e = empties::rejected);

    bool parse_vertex_reference(std::string_view s, vertex_reference_type& vr);

    template<typename T, size_t N>
    bool parse_components(const std::vector<std::string>& tokens, std::array<T, N>& values, size_t nMin, T tDefault = T(0));

    template<typename T, size_t N>
    bool parse_components(std::span<const std::string_view> tokens, std::array<T, N>& values, size_t nMin, T tDefault = T(0));

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
inline void quetzal::wavefront_obj::tokenize(std::string_view s, std::vector<std::string_view>& tokens, std::string_view delimiters, empties e)
{
    tokens.clear();

    size_t pos;
    size_t pos_next = std::numeric_limits<size_t>::max();
    do
    {
        if (e == empties::rejected)
        {
            pos_next = s.find_first_not_of(delimiters, pos_next + 1);
            if (pos_next == std::string_view::npos)
            {
                break;
            }

            pos_next -= 1;
        }

        pos = pos_next + 1;
        pos_next = s.find_first_of(delimiters, pos);
        tokens.push_back(s.substr(pos, pos_next - pos));
    }
    while (pos_next != std::string_view::npos);

    return;
}

//--------------------------------------------------------------------------
inline bool quetzal::wavefront_obj::parse_vertex_reference(std::string_view s, vertex_reference_type& vr)
{
    vr = {};

    size_t n = 0;
    size_t pos = 0;

    for (;;)
    {
        if (n >= vr.size())
        {
            return false;
        }

        size_t pos_next = s.find('/', pos);
        std::string_view component = s.substr(pos, pos_next == std::string_view::npos ? std::string_view::npos : pos_next - pos);

        // An omitted component is left zero, v//vn supplies no texture coordinate
        if (!component.empty())
        {
            auto result = std::from_chars(component.data(), component.data() + component.size(), vr[n]);
            if (result.ec != std::errc{} || result.ptr != component.data() + component.size())
            {
                return false;
            }
        }

        ++n;

        if (pos_next == std::string_view::npos)
        {
            break;
        }

        pos = pos_next + 1;
    }

    return vr[0] != 0;
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

//--------------------------------------------------------------------------
template<typename T, size_t N>
bool quetzal::wavefront_obj::parse_components(std::span<const std::string_view> tokens, std::array<T, N>& values, size_t nRequired, T tDefault)
{
    static_assert(N >= 3 && N <= 4);

    // First token is the keyword
    if (tokens.size() < (nRequired + 1) || tokens.size() > (N + 1))
    {
        return false;
    }

    auto component = [&tokens, tDefault](size_t i) -> T
    {
        if (i >= tokens.size())
        {
            return tDefault;
        }

        T t{};
        std::from_chars(tokens[i].data(), tokens[i].data() + tokens[i].size(), t);
        return t;
    };

    values[0] = component(1);
    values[1] = component(2);
    values[2] = component(3);

    if constexpr (N > 3)
    {
        values[3] = component(4);
    }

    return true;
}

#endif // QUETZAL_WAVEFRONT_OBJ_READER_UTIL_HPP
