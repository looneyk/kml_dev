
#include <tiny_obj_loader.h>
#include <stb_image.h>

#include "Scene.hpp"

#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace quetzal::scene_graph
{

    namespace
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
    vulkan::GeometryHandle load_model(vulkan::IRenderer& renderer, const std::string& id, const std::filesystem::path& path)
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.string().c_str()))
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

        return renderer.upload_geometry<V>(id, vertices, indices);
    }

    vulkan::TextureHandle load_texture(vulkan::IRenderer& renderer, const std::string& id, const std::filesystem::path& path)
    {
        int width, height, channels;
        stbi_uc* pixels = stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!pixels)
        {
            throw std::runtime_error("Failed to load texture: " + path.string());
        }

        vulkan::TextureHandle texture_handle = renderer.upload_texture(id, pixels, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        stbi_image_free(pixels);
        return texture_handle;
    }

    } // namespace

    template<typename V>
    void load(Scene& scene,
              vulkan::IRenderer& renderer,
              const std::filesystem::path& resource_path,
              const std::string& id,
              const std::filesystem::path& model_path,
              const std::filesystem::path& texture_path,
              const game::matrix_type& transform,
              const std::string& vert_spv,
              const std::string& frag_spv)
    {
        vulkan::GeometryHandle geometry_handle = load_model<V>(renderer, id, resource_path / model_path);
        vulkan::TextureHandle texture_handle = load_texture(renderer, id + "_tex", resource_path / texture_path);
        vulkan::EffectHandle effect_handle = renderer.create_effect<V>((resource_path / vert_spv).string(), (resource_path / frag_spv).string());
        vulkan::MaterialHandle material_handle = renderer.create_material(effect_handle, texture_handle);

        auto transform_node = std::make_shared<TransformNode>(id, transform);
        Node& transform_ref = scene.insert(scene.root(), std::move(transform_node));

        auto material_node = std::make_shared<MaterialNode>(id + "_mat", material_handle);
        Node& material_ref = scene.insert(transform_ref, std::move(material_node));

        scene.insert(material_ref, std::make_shared<GeometryNode>(id + "_geo", geometry_handle));
    }

    template void load<vulkan::VertexPositionColorTexcoord>(Scene&, vulkan::IRenderer&, const std::filesystem::path&, const std::string&, const std::filesystem::path&, const std::filesystem::path&, const game::matrix_type&, const std::string&, const std::string&);
    template void load<vulkan::VertexPositionNormalTexcoord>(Scene&, vulkan::IRenderer&, const std::filesystem::path&, const std::string&, const std::filesystem::path&, const std::filesystem::path&, const game::matrix_type&, const std::string&, const std::string&);

} // namespace quetzal::scene_graph
