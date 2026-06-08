
#include <tiny_obj_loader.h>
#include <stb_image.h>

#include "Scene.hpp"

#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace quetzal::scene_graph
{

    template<typename V>
    V make_vertex(const tinyobj::attrib_t& attrib, const tinyobj::index_t& index);

    template<>
    vulkan::VertexPositionColorTexcoord make_vertex(const tinyobj::attrib_t& attrib, const tinyobj::index_t& index)
    {
        return {
            {attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1], attrib.vertices[3 * index.vertex_index + 2]},
            {1.0f, 1.0f, 1.0f},
            {attrib.texcoords[2 * index.texcoord_index + 0], 1.0f - attrib.texcoords[2 * index.texcoord_index + 1]}
        };
    }

    template<>
    vulkan::VertexPositionNormalTexcoord make_vertex(const tinyobj::attrib_t& attrib, const tinyobj::index_t& index)
    {
        return {
            {attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1], attrib.vertices[3 * index.vertex_index + 2]},
            {attrib.normals[3 * index.vertex_index + 0], attrib.normals[3 * index.vertex_index + 1], attrib.normals[3 * index.vertex_index + 2]},
            {attrib.texcoords[2 * index.texcoord_index + 0], 1.0f - attrib.texcoords[2 * index.texcoord_index + 1]}
        };
    }

    template<typename V>
    vulkan::GeometryHandle Scene::load_model(const std::string& id, const std::filesystem::path& model_path)
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, (m_resource_path / model_path).string().c_str()))
        {
            throw std::runtime_error(warn + err);
        }

        std::vector<V> vertices;
        std::vector<uint32_t> indices;
        std::unordered_map<V, uint32_t> unique_vertices;

        for (const auto& shape : shapes)
        {
            for (const auto& index : shape.mesh.indices)
            {
                V vertex = make_vertex<V>(attrib, index);
                if (unique_vertices.count(vertex) == 0)
                {
                    unique_vertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }
                indices.push_back(unique_vertices[vertex]);
            }
        }

        return m_renderer.upload_geometry<V>(id, vertices, indices);
    }

    vulkan::TextureHandle Scene::load_texture(const std::string& id, const std::filesystem::path& texture_path)
    {
        int width, height, channels;
        stbi_uc* pixels = stbi_load((m_resource_path / texture_path).string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!pixels)
        {
            throw std::runtime_error("Failed to load texture: " + texture_path.string());
        }

        vulkan::TextureHandle texture_handle = m_renderer.upload_texture(id, pixels, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        stbi_image_free(pixels);
        return texture_handle;
    }

    template<typename V>
    void Scene::insert(const std::string& id,
                       const std::filesystem::path& model_path,
                       const std::filesystem::path& texture_path,
                       const game::matrix_type& transform,
                       const std::string& vert_spv,
                       const std::string& frag_spv)
    {
        vulkan::GeometryHandle geometry_handle = load_model<V>(id, model_path);
        vulkan::TextureHandle texture_handle = load_texture(id + "_tex", texture_path);
        vulkan::MaterialHandle material_handle = m_renderer.create_material<V>(texture_handle, vert_spv, frag_spv);

        auto transform_node = std::make_unique<TransformNode>(id, transform);
        Node& transform_ref = insert(m_root, std::move(transform_node));

        auto material_node = std::make_unique<MaterialNode>(id + "_mat", material_handle);
        Node& material_ref = insert(transform_ref, std::move(material_node));

        insert(material_ref, std::make_unique<GeometryNode>(id + "_geo", geometry_handle));
    }

    template void Scene::insert<vulkan::VertexPositionColorTexcoord>(const std::string&, const std::filesystem::path&, const std::filesystem::path&, const game::matrix_type&, const std::string&, const std::string&);
    template void Scene::insert<vulkan::VertexPositionNormalTexcoord>(const std::string&, const std::filesystem::path&, const std::filesystem::path&, const game::matrix_type&, const std::string&, const std::string&);

} // namespace quetzal::scene_graph
