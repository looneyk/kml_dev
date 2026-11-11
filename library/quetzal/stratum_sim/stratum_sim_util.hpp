#if !defined(QUETZAL_STRATUM_SIM_STRATUM_SIM_UTIL_HPP)
#define QUETZAL_STRATUM_SIM_STRATUM_SIM_UTIL_HPP
//------------------------------------------------------------------------------
// stratum_sim
// stratum_sim_util.hpp
//------------------------------------------------------------------------------

#include "quetzal/log/Log.hpp"
#include <format>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Returns the value in the index for the given key
    // Throws std::runtime_error if the key is not found, value_type is for the error message
    template<typename T>
    const T& value_at(const std::map<std::string, T>& index, const std::string& key, std::string_view value_type);

} // namespace quetzal::stratum_sim

//--------------------------------------------------------------------------
template<typename T>
const T& quetzal::stratum_sim::value_at(const std::map<std::string, T>& index, const std::string& key, std::string_view value_type)
{
    auto i = index.find(key);
    if (i == index.end())
    {
        const std::string message = std::format("{} value is not found for key {}", value_type, key);
        log::logger().error("stratum_sim", "{}", message);
        throw std::runtime_error(message);
    }

    return i->second;
}

#endif // QUETZAL_STRATUM_SIM_STRATUM_SIM_UTIL_HPP
