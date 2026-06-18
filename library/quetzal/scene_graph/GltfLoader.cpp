
#include "GltfLoader.hpp"
#include "GeometryNode.hpp"
#include "MaterialNode.hpp"
#include "TransformNode.hpp"
#include "quetzal/vulkan/TextureData.hpp"
#include "quetzal/vulkan/Vertex.hpp"
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <stb_image.h>
#include <stdexcept>
#include <string>
#include <vector>

using namespace quetzal;

namespace quetzal::scene_graph
{

namespace
{

game::matrix_type to_matrix(const fastgltf::Node& node)
{
    return std::visit(fastgltf::visitor{
        [](const fastgltf::TRS& trs) -> game::matrix_type
        {
            game::matrix_type T = glm::translate(game::matrix_type(1.0f),
                glm::vec3(trs.translation.x(), -trs.translation.z(), trs.translation.y()));
            glm::quat q(trs.rotation.w(), trs.rotation.x(), -trs.rotation.z(), trs.rotation.y());
            game::matrix_type R = glm::mat4_cast(q);
            game::matrix_type S = glm::scale(game::matrix_type(1.0f),
                glm::vec3(trs.scale.x(), trs.scale.z(), trs.scale.y()));
            return T * R * S;
        },
        [](const fastgltf::math::fmat4x4& m) -> game::matrix_type
        {
            game::matrix_type r;
            for (int c = 0; c < 4; ++c)
                r[c] = glm::vec4(m.col(c).x(), m.col(c).y(), m.col(c).z(), m.col(c).w());
            return r;
        }
    }, node.transform);
}

vulkan::TextureHandle load_image(vulkan::IRenderer& renderer,
                                  const fastgltf::Asset& asset,
                                  std::size_t image_index,
                                  const std::string& id,
                                  const std::filesystem::path& gltf_dir)
{
    const auto& image = asset.images[image_index];

    auto upload_from_bytes = [&](const std::byte* data, std::size_t size) -> vulkan::TextureHandle
    {
        int w, h, ch;
        stbi_uc* pixels = stbi_load_from_memory(
            reinterpret_cast<const stbi_uc*>(data), static_cast<int>(size),
            &w, &h, &ch, STBI_rgb_alpha);
        if (!pixels)
            return {};
        vulkan::TextureData td{VK_FORMAT_R8G8B8A8_SRGB, {{pixels, static_cast<size_t>(w) * h * 4, static_cast<uint32_t>(w), static_cast<uint32_t>(h)}}};
        auto handle = renderer.upload_texture(id, td);
        stbi_image_free(pixels);
        return handle;
    };

    return std::visit(fastgltf::visitor{
        [](auto&) -> vulkan::TextureHandle { return {}; },
        [&](const fastgltf::sources::URI& uri) -> vulkan::TextureHandle
        {
            auto path = gltf_dir / uri.uri.fspath();
            int w, h, ch;
            stbi_uc* pixels = stbi_load(path.string().c_str(), &w, &h, &ch, STBI_rgb_alpha);
            if (!pixels)
                return {};
            vulkan::TextureData td{VK_FORMAT_R8G8B8A8_SRGB, {{pixels, static_cast<size_t>(w) * h * 4, static_cast<uint32_t>(w), static_cast<uint32_t>(h)}}};
            auto handle = renderer.upload_texture(id, td);
            stbi_image_free(pixels);
            return handle;
        },
        [&](const fastgltf::sources::Array& arr) -> vulkan::TextureHandle
        {
            return upload_from_bytes(arr.bytes.data(), arr.bytes.size());
        },
        [&](const fastgltf::sources::BufferView& bv_source) -> vulkan::TextureHandle
        {
            const auto& bv = asset.bufferViews[bv_source.bufferViewIndex];
            const auto& buf = asset.buffers[bv.bufferIndex];
            return std::visit(fastgltf::visitor{
                [](auto&) -> vulkan::TextureHandle { return {}; },
                [&](const fastgltf::sources::Array& arr) -> vulkan::TextureHandle
                {
                    return upload_from_bytes(arr.bytes.data() + bv.byteOffset, bv.byteLength);
                }
            }, buf.data);
        }
    }, image.data);
}

void load_primitive(Scene& scene,
                    Node& parent,
                    vulkan::IRenderer& renderer,
                    AssetManager& assets,
                    const fastgltf::Asset& asset,
                    const fastgltf::Primitive& prim,
                    const std::string& id,
                    const std::filesystem::path& gltf_dir,
                    ShaderManager& shaders)
{
    using V = vulkan::VertexPositionNormalTexcoord;

    auto* pos_it = prim.findAttribute("POSITION");
    if (pos_it == prim.attributes.end())
        return;

    auto& pos_acc = asset.accessors[pos_it->accessorIndex];
    std::vector<V> vertices(pos_acc.count);

    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, pos_acc,
        [&](fastgltf::math::fvec3 v, std::size_t i)
        {
            vertices[i].position = {v.x(), -v.z(), v.y()};
        });

    if (auto* n = prim.findAttribute("NORMAL"); n != prim.attributes.end())
    {
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset,
            asset.accessors[n->accessorIndex],
            [&](fastgltf::math::fvec3 v, std::size_t i)
            {
                vertices[i].normal = {v.x(), -v.z(), v.y()};
            });
    }

    if (auto* tc = prim.findAttribute("TEXCOORD_0"); tc != prim.attributes.end())
    {
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(asset,
            asset.accessors[tc->accessorIndex],
            [&](fastgltf::math::fvec2 v, std::size_t i)
            {
                vertices[i].texcoord = {v.x(), 1.0f - v.y()};
            });
    }

    std::vector<uint32_t> indices;
    if (prim.indicesAccessor.has_value())
    {
        auto& idx_acc = asset.accessors[prim.indicesAccessor.value()];
        indices.resize(idx_acc.count);
        fastgltf::copyFromAccessor<uint32_t>(asset, idx_acc, indices.data());
    }

    vulkan::GeometryHandle geometry = renderer.upload_geometry<V>(
        id + "_geo", std::span<const V>(vertices), std::span<const uint32_t>(indices));

    static const uint32_t white = 0xFFFFFFFF;
    vulkan::TextureData white_td{VK_FORMAT_R8G8B8A8_SRGB, {{&white, 4, 1u, 1u}}};
    vulkan::TextureHandle texture = renderer.upload_texture("__default_white", white_td);
    std::string effect_name = "emissive";

    if (prim.materialIndex.has_value())
    {
        const auto& mat = asset.materials[prim.materialIndex.value()];
        if (mat.pbrData.baseColorTexture.has_value())
        {
            const auto& tex = asset.textures[mat.pbrData.baseColorTexture->textureIndex];
            if (tex.imageIndex.has_value())
                texture = load_image(renderer, asset, tex.imageIndex.value(), id + "_tex", gltf_dir);
            effect_name = "pnt";
        }
        else
        {
            const auto& ef = mat.emissiveFactor;
            uint8_t r = static_cast<uint8_t>(std::clamp(ef[0], 0.0f, 1.0f) * 255.0f);
            uint8_t g = static_cast<uint8_t>(std::clamp(ef[1], 0.0f, 1.0f) * 255.0f);
            uint8_t b = static_cast<uint8_t>(std::clamp(ef[2], 0.0f, 1.0f) * 255.0f);
            uint32_t pixel = (0xFFu << 24) | (static_cast<uint32_t>(b) << 16) | (static_cast<uint32_t>(g) << 8) | r;
            vulkan::TextureData emissive_td{VK_FORMAT_R8G8B8A8_SRGB, {{&pixel, 4, 1u, 1u}}};
            texture = renderer.upload_texture(id + "_emissive", emissive_td);
        }
    }

    vulkan::EffectHandle effect = renderer.create_effect<V>(
        shaders.vert_path(effect_name).string(),
        shaders.frag_path(effect_name).string());

    vulkan::MaterialHandle material = renderer.create_material(effect, texture);

    auto mat_node = std::make_shared<MaterialNode>(id + "_mat", material);
    Node& mat_ref = scene.insert(parent, std::move(mat_node));
    scene.insert(mat_ref, std::make_shared<GeometryNode>(id + "_geo", geometry));
}

void load_node(Scene& scene,
               Node& parent,
               vulkan::IRenderer& renderer,
               AssetManager& assets,
               const fastgltf::Asset& asset,
               std::size_t node_index,
               const std::string& id_prefix,
               const std::filesystem::path& gltf_dir,
               ShaderManager& shaders)
{
    const auto& node = asset.nodes[node_index];
    std::string node_id = id_prefix +
        (node.name.empty() ? std::to_string(node_index) : std::string(node.name.c_str()));

    auto transform_node = std::make_shared<TransformNode>(node_id, to_matrix(node));
    Node& xform_ref = scene.insert(parent, std::move(transform_node));

    if (node.meshIndex.has_value())
    {
        const auto& mesh = asset.meshes[node.meshIndex.value()];
        for (std::size_t i = 0; i < mesh.primitives.size(); ++i)
        {
            std::string prim_id = node_id + "_p" + std::to_string(i);
            load_primitive(scene, xform_ref, renderer, assets, asset,
                mesh.primitives[i], prim_id, gltf_dir, shaders);
        }
    }

    for (std::size_t child_idx : node.children)
    {
        load_node(scene, xform_ref, renderer, assets, asset,
            child_idx, id_prefix, gltf_dir, shaders);
    }
}

} // namespace

void load_gltf(Scene& scene,
               Node& parent,
               vulkan::IRenderer& renderer,
               AssetManager& assets,
               const std::filesystem::path& path,
               ShaderManager& shaders)
{
    auto data = fastgltf::MappedGltfFile::FromPath(path);
    if (!bool(data))
        throw std::runtime_error("Failed to open glTF: " +
            std::string(fastgltf::getErrorMessage(data.error())));

    fastgltf::Parser parser;
    auto result = parser.loadGltf(data.get(), path.parent_path(),
        fastgltf::Options::LoadExternalBuffers |
        fastgltf::Options::LoadExternalImages |
        fastgltf::Options::GenerateMeshIndices);

    if (result.error() != fastgltf::Error::None)
        throw std::runtime_error("Failed to parse glTF: " +
            std::string(fastgltf::getErrorMessage(result.error())));

    const fastgltf::Asset& asset = result.get();
    std::size_t scene_index = asset.defaultScene.value_or(0);
    if (scene_index >= asset.scenes.size())
        return;

    const std::string id_prefix = path.stem().string() + "_";
    const std::filesystem::path gltf_dir = path.parent_path();

    for (std::size_t node_idx : asset.scenes[scene_index].nodeIndices)
    {
        load_node(scene, parent, renderer, assets, asset,
            node_idx, id_prefix, gltf_dir, shaders);
    }
}

} // namespace quetzal::scene_graph
