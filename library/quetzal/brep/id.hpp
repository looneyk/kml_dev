#if !defined(QUETZAL_BREP_ID_HPP)
#define QUETZAL_BREP_ID_HPP
//------------------------------------------------------------------------------
// brep
// id.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/id.hpp"
#include "quetzal/common/string_util.hpp"
#include <format>
#include <string>
#include <cstdint>

namespace quetzal::brep
{

    template<typename T>
    bool good(id_type id, const T& container);

    template<typename T>
    std::string id_string(id_type id, const T& container);

    inline std::string id_string(id_type id);
    inline std::string partner_id_string(id_type id);

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename T>
bool quetzal::brep::good(id_type id, const T& container)
{
    return id != nullid && id < container.size() && !container[id].deleted();
}

//------------------------------------------------------------------------------
template<typename T>
std::string quetzal::brep::id_string(id_type id, const T& container)
{
    if (id == nullid)
    {
        return "nullid";
    }

    if (id >= container.size())
    {
        return std::format("id {} out of range", id);
    }

    if (container[id].deleted())
    {
        return std::format("id {} deleted", id);
    }

    return quetzal::to_string(id);
}

//------------------------------------------------------------------------------
inline std::string quetzal::brep::id_string(id_type id)
{
    if (id == nullid)
    {
        return "nullid";
    }

    return quetzal::to_string(id);
}

//------------------------------------------------------------------------------
inline std::string quetzal::brep::partner_id_string(id_type id)
{
    if (id == nullid)
    {
        return "border";
    }

    return quetzal::to_string(id);
}

#endif // QUETZAL_BREP_ID_HPP
