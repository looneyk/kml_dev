#if !defined(QUETZAL_GEOMETRY_PATH_HPP)
#define QUETZAL_GEOMETRY_PATH_HPP
//------------------------------------------------------------------------------
// geometry
// Path.hpp
//------------------------------------------------------------------------------

#include "OrientedPoint.hpp"
#include "quetzal/common/string_util.hpp"
#include "quetzal/xml/Document.hpp"
#include <filesystem>
#include <fstream>
#include <vector>
#include <cassert>

namespace quetzal::geometry
{

    template<typename Traits>
    using Path = std::vector<geometry::OrientedPoint<Traits>>;

    template<typename Traits>
    Path<Traits> load_path(const std::filesystem::path& filepath);

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Path<Traits> quetzal::geometry::load_path(const std::filesystem::path& filepath)
{
    static const std::string s_path = "path";
    static const std::string s_point = "point";
    static const std::string s_position = "position";
    static const std::string s_direction = "direction";
    static const std::string s_right = "right";

    Path<Traits> path;

    std::ifstream is(filepath, std::ios_base::binary);
    if (!is)
    {
        return path;
    }

    xml::Document document;
    is >> document;

    const xml::Element& e = document.element();
    if (e.name() != s_path)
    {
        return path;
    }

    for (auto p : e.elements())
    {
        const xml::Element* pe = p.get();
        assert(pe != nullptr);

        const std::string element_name = pe->name();
        if (element_name == s_point)
        {
            auto position = to_type<typename Path<Traits>::value_type::point_type>(pe->element_value(s_position));
            auto direction = to_type<typename Path<Traits>::value_type::vector_type>(pe->element_value(s_direction));
            auto right = to_type<typename Path<Traits>::value_type::vector_type>(pe->element_value(s_right));
            path.emplace_back(position, {direction, right});
        }
    }

    return path;
}

#endif // QUETZAL_GEOMETRY_PATH_HPP
