#if !defined(QUETZAL_MODEL_OBJ_IO_HPP)
#define QUETZAL_MODEL_OBJ_IO_HPP
//------------------------------------------------------------------------------
// model
// obj_io.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/id.hpp"
#include "quetzal/wavefront_obj/Material.hpp"
#include "quetzal/wavefront_obj/MaterialLibrary.hpp"
#include "quetzal/wavefront_obj/Reader.hpp"
#include "quetzal/wavefront_obj/Writer.hpp"
#include "quetzal/wavefront_obj/symbols.hpp"
#include <array>
#include <map>
#include <filesystem>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace quetzal::model
{

    template<typename M>
    void read_obj(M& mesh, const std::filesystem::path& filepath);

    template<typename M>
    void write_obj(const M& mesh, const std::filesystem::path& filepath);

    template<typename U>
    void write_submesh_obj(const U& submesh, const std::filesystem::path& filepath);

    template<typename W, typename M>
    void write_vertices(W& writer, const M& mesh);

    template<typename W, typename M>
    void write_faces(W& writer, const M& mesh);

    template<typename W, typename U>
    void write_submesh_vertices(W& writer, const U& submesh);

    template<typename W, typename U>
    void write_submesh_faces(W& writer, const U& submesh);

    template<typename W, typename F>
    void write_face(W& writer, const F& face);

    using Materials = std::unordered_map<std::string, wavefront_obj::Material>;

    template<typename M>
    Materials read_materials(const M& mesh, const std::filesystem::path& filepath); // path to materials directory

    // central location for these? ...
    // these are not directly related to either wavefront_obj or Mesh ...
    const std::string MaterialStandardPropertyName = "material_standard";
    const std::string MaterialsPropertyName = "materials";
    const std::string MaterialPropertyName = "material";

} // namespace quetzal::model

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::read_obj(M& mesh, const std::filesystem::path& filepath)
{
    id_type submesh_id = nullid;
    id_type surface_id = nullid;
    id_type face_id = nullid;
    size_t vertex_count = 0;
    std::string material;

    using segment_type = std::array<typename M::point_type, 2>;
    std::map<segment_type, id_type> segments;

    auto on_open = [&](M& mesh, const std::filesystem::path& filepath) -> void
    {
        filepath;

        assert(submesh_id == nullid);
        assert(surface_id == nullid);
        assert(face_id == nullid);
        
        mesh.properties()[MaterialStandardPropertyName] = "wavefront_obj";
        return;
    };

    auto on_position = [&](M& mesh, const typename M::point_type& position) -> void
    {
        mesh;
        position;
        return;
    };

    auto on_normal = [&](M& mesh, const typename M::vector_type& normal) -> void
    {
        mesh;
        normal;
        return;
    };

//    auto on_texcoord = [&](M& mesh, const typename M::vertex_attributes_type::texcoord_type& texcoord) -> void
    auto on_texcoord = [&](M& mesh, const wavefront_obj::t_type_t<M>& texcoord) -> void
    {
        mesh;
        texcoord;
        return;
    };

    auto on_object = [&](M& mesh, const std::string& name) -> void
    {
        submesh_id = mesh.contains_submesh(name) ? mesh.submesh(name).id() : mesh.create_submesh(name);
        return;
    };

    auto on_group = [&](M& mesh, const std::string& name) -> void
    {
        if (submesh_id == nullid)
        {
            submesh_id = mesh.create_submesh(wavefront_obj::ObjectNameDefault);
        }

        surface_id = mesh.get_surface_id(submesh_id, name);

        if (mesh.surface(surface_id).empty() && !material.empty())
        {
            mesh.surface(surface_id).properties()[MaterialPropertyName] = material;
        }
        else if (mesh.surface(surface_id).properties().contains(MaterialPropertyName))
        {
            if (mesh.surface(surface_id).properties().at(MaterialPropertyName) != material)
            {
                // The case where an existing group is being specified in the context of a different material is not handled here ...
                // This needs to be deferred in case the discrepancy is corrected in a subsequent material specification ...
                assert(false);
            }
        }

        return;
    };

    auto on_face_open = [&](M& mesh) -> void
    {
        if (surface_id == nullid)
        {
            if (submesh_id == nullid)
            {
                submesh_id = mesh.create_submesh(wavefront_obj::ObjectNameDefault);
            }

            surface_id = mesh.create_surface(submesh_id, wavefront_obj::GroupNameDefault);
        }
        else
        {
            assert(submesh_id != nullid);
        }

        face_id = mesh.create_face(surface_id, mesh.halfedge_store_count());
        vertex_count = 0;
        return;
    };

    auto on_face_vertex = [&](M& mesh, const typename M::point_type& position, const typename M::vector_type& normal, const typename M::vertex_attributes_type::texcoord_type& texcoord) -> void
    {
        id_type nh = mesh.halfedge_store_count();
        typename M::vertex_attributes_type av = {position, normal, texcoord};
        mesh.create_halfedge_vertex(0, nh + 1, nh - 1, face_id, av);
        ++vertex_count;
        return;
    };

    auto on_face_close = [&](M& mesh) -> void
    {
        // Correct prev and next for first and last halfedges respectively
        id_type nh = mesh.halfedge_store_count();
        mesh.halfedge(nh - vertex_count).set_prev_id(nh - 1);
        mesh.halfedge(nh - 1).set_next_id(nh - vertex_count);

        // Add partner id to each halfedge in this new face
        auto& face = mesh.face(face_id);
        for (auto& halfedge : face.halfedges())
        {
            id_type halfedge_id = halfedge.id();
            const auto& position = halfedge.vertex_attributes().position();
            const auto& position_next = halfedge.next().vertex_attributes().position();
            segment_type segment = {position_next, position};
            auto i = segments.find(segment);
            if (i == segments.end())
            {
                segments[{position, position_next}] = halfedge_id;
            }
            else
            {
                halfedge.set_partner_id(i->second);
                mesh.halfedge(i->second).set_partner_id(halfedge_id);
                segments.erase(i);
            }
        }

        // Recreate face normal
        face.attributes().set_normal(geometry::normal(face));
        return;
    };

    auto on_materials = [&](M& mesh, const std::string& name) -> void
    {
        std::string prefix;
        if (mesh.properties().contains(MaterialsPropertyName))
        {
            prefix = mesh.properties().at(MaterialsPropertyName) + " ";
        }

        mesh.properties()[MaterialsPropertyName] = prefix + name;
        return;
    };

    auto on_material = [&](M& mesh, const std::string& name) -> void
    {
        material = name;

        if (surface_id == nullid)
        {
            on_group(mesh, wavefront_obj::GroupNameDefault);
        }

        if (mesh.surface(surface_id).empty() && !material.empty())
        {
            mesh.surface(surface_id).properties()[MaterialPropertyName] = material;
        }
        else if (mesh.surface(surface_id).properties().contains(MaterialPropertyName))
        {
            if (mesh.surface(surface_id).properties().at(MaterialPropertyName) != material)
            {
                // The case where an existing group is being specified in the context of a different material is not handled here ...
                // Could handle this here creating surface with name group_material, store group name in properties for subsequent use, ...
                assert(false);
            }
        }

        return;
    };

    wavefront_obj::Reader<M> reader(on_open, on_position, on_normal, on_texcoord, on_object, on_group, on_face_open, on_face_vertex, on_face_close, on_materials, on_material);
    reader.read(mesh, filepath);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::model::write_obj(const M& mesh, const std::filesystem::path& filepath)
{
    using vertex_type = M::vertex_type;
    using attributes_type = vertex_type::attributes_type;
    using point_type = attributes_type::point_type;
    using vector_type = attributes_type::vector_type;
    using texcoord_type = attributes_type::texcoord_type;

    wavefront_obj::Writer<point_type, vector_type, texcoord_type> writer(filepath);

    if (mesh.properties().contains(MaterialsPropertyName))
    {
        writer.set_materials(mesh.properties().at(MaterialsPropertyName));
    }

    write_vertices(writer, mesh);
    write_faces(writer, mesh);
    return;
}

//------------------------------------------------------------------------------
template<typename U>
void quetzal::model::write_submesh_obj(const U& submesh, const std::filesystem::path& filepath)
{
    using vertex_type = U::vertex_type;
    using attributes_type = vertex_type::attributes_type;
    using point_type = attributes_type::point_type;
    using vector_type = attributes_type::vector_type;
    using texcoord_type = attributes_type::texcoord_type;

    wavefront_obj::Writer<point_type, vector_type, texcoord_type> writer(filepath);

    if (submesh.properties().contains(MaterialsPropertyName))
    {
        writer.set_materials(submesh.properties().at(MaterialsPropertyName));
    }

    write_submesh_vertices(writer, submesh);
    write_submesh_faces(writer, submesh);
    return;
}

//------------------------------------------------------------------------------
template<typename W, typename M>
void quetzal::model::write_vertices(W& writer, const M& mesh)
{
    for (const typename M::vertex_type& vertex : mesh.vertices())
    {
        const auto& attributes = vertex.attributes();
        writer.set_vertex(attributes.position(), attributes.normal(), attributes.texcoord());
    }

    return;
}

//------------------------------------------------------------------------------
template<typename W, typename M>
void quetzal::model::write_faces(W& writer, const M& mesh)
{
    bool b = false;

    // First write mesh faces with no submesh
    for (const typename M::face_type& face : mesh.faces())
    {
        if (face.submesh_id() == nullid)
        {
            write_face(writer, face);
            b = true;
        }
    }

    assert(!b); // Currently, all faces should be associated with a submesh ...

    // Write remaining faces by object and group
    for (const typename M::submesh_type& submesh : mesh.submeshes())
    {
        write_submesh_faces(writer, submesh);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename W, typename U>
void quetzal::model::write_submesh_vertices(W& writer, const U& submesh)
{
    using vertex_type = U::vertex_type;

    for (const auto& face : submesh.faces())
    {
        for (const auto& halfedge : face.halfedges())
        {
            const auto& attributes = halfedge.vertex_attributes();
            writer.set_vertex(attributes.position(), attributes.normal(), attributes.texcoord());
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename W, typename U>
void quetzal::model::write_submesh_faces(W& writer, const U& submesh)
{
    assert(!submesh.empty());

    writer.set_object(submesh.name());

    bool b = false;

    // First write submesh faces with no surface
    for (const auto& face : submesh.faces())
    {
        if (face.surface_id() == nullid)
        {
            write_face(writer, face);
            b = true;
        }
    }

    assert(!b); // Currently, all faces should be associated with a surface ...

    for (const auto& surface : submesh.surfaces())
    {
        assert(!surface.empty());
        writer.set_group(surface.name());
        if (surface.properties().contains(MaterialPropertyName))
        {
            writer.set_material(surface.properties().at(MaterialPropertyName));
        }

        for (const auto& face : surface.faces())
        {
            write_face(writer, face);
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename W, typename F>
void quetzal::model::write_face(W& writer, const F& face)
{
    writer.set_face();
    for (const auto& halfedge : face.halfedges())
    {
        const auto& attributes = halfedge.vertex_attributes();
        writer.set_face_vertex(attributes.position(), attributes.normal(), attributes.texcoord());
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::Materials quetzal::model::read_materials(const M& mesh, const std::filesystem::path& filepath)
{
    Materials materials;

    std::vector<std::string> filenames;
    if (mesh.properties().contains(MaterialsPropertyName))
    {
        std::istringstream iss(mesh.properties().at(MaterialsPropertyName));
        std::string filename;
        while (iss >> filename)  
        { 
            filenames.push_back(filename);
        } 
    }

    std::unordered_set<std::string> material_usages;
    for (const auto& surface : mesh.surfaces())
    {
        if (surface.properties().contains(MaterialPropertyName))
        {
            material_usages.insert(surface.properties().at(MaterialPropertyName));
        }
    }

    for (const auto& filename : filenames)
    {
//        filename = filepath / filename;
filepath;
        wavefront_obj::MaterialLibrary material_library(filename);
        while (!material_library.eof())
        {
            wavefront_obj::Material material = material_library.read_material();
            std::string name = material.m_name;
            if (materials.contains(name))
            {
                continue; // skip second definition, should log warning ...
            }

            if (material_usages.contains(name))
            {
                materials.emplace(name, material);
                material_usages.erase(name);
            }
        }
    }

    return materials;
}

#endif // QUETZAL_MODEL_OBJ_IO_HPP
