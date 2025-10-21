#if !defined(OBJ_IO_HPP)
#define OBJ_IO_HPP
//------------------------------------------------------------------------------
// obj_io
// obj_io.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/id.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/Vector.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/wavefront_obj/Reader.hpp"
#include "quetzal/wavefront_obj/symbols.hpp"
#include <filesystem>
#include <string>

namespace obj_io
{
    template<typename M>
    void read_obj(M& mesh, const std::filesystem::path& filepath);

} // namespace obj_io

//------------------------------------------------------------------------------
template<typename M>
void obj_io::read_obj(M& mesh, const std::filesystem::path& filepath)
{
    auto on_open = [&](M& mesh, const std::filesystem::path& filepath) -> void
    {
        mesh;
        filepath;
        return;
    };

    auto on_position = [&](M& mesh, const typename M::point_type& position) -> void
    {
        mesh.positions.push_back(position);
        return;
    };

    auto on_normal = [&](M& mesh, const typename M::vector_type& normal) -> void
    {
        mesh.normals.push_back(normal);
        return;
    };

    auto on_texcoord = [&](M& mesh, const typename M::texcoord_type& texcoord) -> void
    {
        mesh.texcoords.push_back(texcoord);
        return;
    };

    auto on_object = [&](M& mesh, const std::string& name) -> void
    {
        mesh;
        name;
        return;
    };

    auto on_group = [&](M& mesh, const std::string& name) -> void
    {
        mesh;
        name;
        return;
    };

    auto on_face_open = [&](M& mesh) -> void
    {
        mesh;
        return;
    };

    auto on_face_vertex = [&](M& mesh, const typename M::point_type& position, const typename M::vector_type& normal, const typename M::texcoord_type& texcoord) -> void
    {
        mesh;
        position;
        normal;
        texcoord;
        return;
    };

    auto on_face_close = [&](M& mesh) -> void
    {
        mesh;
        return;
    };

    auto on_materials = [&](M& mesh, const std::string& name) -> void
    {
        mesh;
        name;
        return;
    };

    auto on_material = [&](M& mesh, const std::string& name) -> void
    {
        mesh;
        name;
        return;
    };

    quetzal::wavefront_obj::Reader<M> reader(on_open, on_position, on_normal, on_texcoord, on_object, on_group, on_face_open, on_face_vertex, on_face_close, on_materials, on_material);
    reader.read(mesh, filepath);
    return;
}

#endif // OBJ_IO_HPP
