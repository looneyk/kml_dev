
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "TinyobjReader.hpp"
#include <functional>
#include <stdexcept>
#include <unordered_map>

namespace quetzal::scene_graph
{

namespace
{

struct VertexKey
{
    int vertex_index;
    int normal_index;
    int texcoord_index;

    bool operator==(const VertexKey&) const = default;
};

struct VertexKeyHash
{
    size_t operator()(const VertexKey& k) const
    {
        size_t h = std::hash<int>{}(k.vertex_index);
        h ^= std::hash<int>{}(k.normal_index) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>{}(k.texcoord_index) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

} // namespace

bool TinyobjReader::can_read(const std::filesystem::path& path) const
{
    return path.extension() == ".obj";
}

MeshData TinyobjReader::read(const std::filesystem::path& path) const
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.string().c_str()))
    {
        throw std::runtime_error(warn + err);
    }

    MeshData data;
    std::unordered_map<VertexKey, uint32_t, VertexKeyHash> unique_vertices;

    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            VertexKey key{index.vertex_index, index.normal_index, index.texcoord_index};
            auto [it, inserted] = unique_vertices.emplace(key, static_cast<uint32_t>(data.positions.size()));

            if (inserted)
            {
                data.positions.push_back({
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                });

                if (!attrib.normals.empty() && index.normal_index >= 0)
                {
                    data.normals.push_back({
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2]
                    });
                }
                else
                {
                    data.normals.push_back({0.0f, 0.0f, 1.0f});
                }

                if (!attrib.colors.empty())
                {
                    data.colors.push_back({
                        attrib.colors[3 * index.vertex_index + 0],
                        attrib.colors[3 * index.vertex_index + 1],
                        attrib.colors[3 * index.vertex_index + 2]
                    });
                }
                else
                {
                    data.colors.push_back({1.0f, 1.0f, 1.0f});
                }

                if (!attrib.texcoords.empty() && index.texcoord_index >= 0)
                {
                    data.texcoords.push_back({
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                    });
                }
                else
                {
                    data.texcoords.push_back({0.0f, 0.0f});
                }
            }

            data.indices.push_back(it->second);
        }
    }

    return data;
}

} // namespace quetzal::scene_graph
