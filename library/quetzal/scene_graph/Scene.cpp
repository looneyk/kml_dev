
#include "Scene.hpp"
#include "ShaderManager.hpp"
#include "quetzal/vulkan/Vertex.hpp"
#include <glm/glm.hpp>
#include <stdexcept>
#include <vector>

namespace quetzal::scene_graph
{

    namespace
    {

    template<typename V>
    V make_vertex(const MeshData& data, size_t i);

    template<>
    vulkan::VertexPositionColorTexcoord make_vertex(const MeshData& data, size_t i)
    {
        glm::vec3 color = data.colors.empty()
            ? glm::vec3{1.0f, 1.0f, 1.0f}
            : glm::vec3{data.colors[i][0], data.colors[i][1], data.colors[i][2]};
        return {
            {data.positions[i][0], data.positions[i][1], data.positions[i][2]},
            color,
            {data.texcoords[i][0], data.texcoords[i][1]}
        };
    }

    template<>
    vulkan::VertexPositionNormalTexcoord make_vertex(const MeshData& data, size_t i)
    {
        return {
            {data.positions[i][0], data.positions[i][1], data.positions[i][2]},
            {data.normals[i][0], data.normals[i][1], data.normals[i][2]},
            {data.texcoords[i][0], data.texcoords[i][1]}
        };
    }

    } // namespace

    template<typename V>
    void load(Scene& scene,
              vulkan::IRenderer& renderer,
              AssetManager& assets,
              const std::string& id,
              const std::filesystem::path& model_path,
              const std::filesystem::path& texture_path,
              const game::matrix_type& transform,
              ShaderManager& shaders,
              const std::string& effect_name)
    {
        MeshData mesh_data = assets.load_mesh(model_path);

        std::vector<V> vertices;
        vertices.reserve(mesh_data.positions.size());
        for (size_t i = 0; i < mesh_data.positions.size(); ++i)
        {
            vertices.push_back(make_vertex<V>(mesh_data, i));
        }

        vulkan::GeometryHandle geometry_handle = renderer.upload_geometry<V>(id, vertices, mesh_data.indices);
        vulkan::TextureHandle texture_handle = assets.load_texture(id + "_tex", texture_path);
        vulkan::EffectHandle effect_handle = renderer.create_effect<V>(
            shaders.vert_path(effect_name).string(),
            shaders.frag_path(effect_name).string());
        vulkan::MaterialHandle material_handle = renderer.create_material(effect_handle, texture_handle);

        auto transform_node = std::make_shared<TransformNode>(id, transform);
        Node& transform_ref = scene.insert(scene.root(), std::move(transform_node));

        auto material_node = std::make_shared<MaterialNode>(id + "_mat", material_handle);
        Node& material_ref = scene.insert(transform_ref, std::move(material_node));

        scene.insert(material_ref, std::make_shared<GeometryNode>(id + "_geo", geometry_handle));
    }

    template void load<vulkan::VertexPositionColorTexcoord>(Scene&, vulkan::IRenderer&, AssetManager&, const std::string&, const std::filesystem::path&, const std::filesystem::path&, const game::matrix_type&, ShaderManager&, const std::string&);
    template void load<vulkan::VertexPositionNormalTexcoord>(Scene&, vulkan::IRenderer&, AssetManager&, const std::string&, const std::filesystem::path&, const std::filesystem::path&, const game::matrix_type&, ShaderManager&, const std::string&);

} // namespace quetzal::scene_graph
