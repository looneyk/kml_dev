#if !defined(QUETZAL_VULKAN_VERTEX_HPP)
#define QUETZAL_VULKAN_VERTEX_HPP

#include "IRenderer.hpp"
#include "quetzal/game/types.hpp"
#pragma warning(push)
#pragma warning(disable: 4201)
#include <glm/gtx/hash.hpp>
#pragma warning(pop)
#include <vulkan/vulkan.hpp>
#include <vector>

namespace quetzal::vulkan
{
/*
    struct Vertex
    {
        using binding_description_type = VkVertexInputBindingDescription;
        using attribute_descriptions_type = std::vector<VkVertexInputAttributeDescription>;

        virtual binding_description_type binding_description() = 0;
        virtual attribute_descriptions_type attribute_descriptions() = 0;
    };
*/
    struct VertexPositionColorTexcoord
    {
        glm::vec3 position;
        glm::vec3 color;
        glm::vec2 texcoord;

        using binding_description_type = VkVertexInputBindingDescription;
        using attribute_descriptions_type = std::vector<VkVertexInputAttributeDescription>;
/*
        VertexPositionColorTexcoord(const tinyobj::attrib_t& attrib, const tinyobj::index_t& index) :
            position{
            attrib.vertices[3 * index.vertex_index + 0],
            attrib.vertices[3 * index.vertex_index + 1],
            attrib.vertices[3 * index.vertex_index + 2]
        },
            color{1.0f, 1.0f, 1.0f},
            texcoord{
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            }
        {
        }
*/
        bool operator==(const VertexPositionColorTexcoord& other) const
        {
            return position == other.position && color == other.color && texcoord == other.texcoord;
        }

        static const binding_description_type& binding_description()
        {
            return m_binding_description;
        }

        static attribute_descriptions_type attribute_descriptions()
        {
            return m_attribute_descriptions;
        }

    private:

        static binding_description_type m_binding_description;
        static attribute_descriptions_type m_attribute_descriptions;
    };

    struct VertexPositionNormalTexcoord
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texcoord;

        using binding_description_type = VkVertexInputBindingDescription;
        using attribute_descriptions_type = std::vector<VkVertexInputAttributeDescription>;
/*
        VertexPositionNormalTexcoord(const tinyobj::attrib_t& attrib, const tinyobj::index_t& index) :
            position{
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            },
            normal{
                attrib.normals[3 * index.vertex_index + 0],
                attrib.normals[3 * index.vertex_index + 1],
                attrib.normals[3 * index.vertex_index + 2]
            },
            texcoord{
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            }
        {
        }
*/
        bool operator==(const VertexPositionNormalTexcoord& other) const
        {
            return position == other.position && normal == other.normal && texcoord == other.texcoord;
        }

        static const binding_description_type& binding_description()
        {
            return m_binding_description;
        }

        static attribute_descriptions_type attribute_descriptions()
        {
            return m_attribute_descriptions;
        }

    private:

        static binding_description_type m_binding_description;
        static attribute_descriptions_type m_attribute_descriptions;
    };

    template<>
    inline VertexBinding vertex_binding<VertexPositionColorTexcoord>()
    {
        const auto& bd = VertexPositionColorTexcoord::binding_description();
        const auto ads = VertexPositionColorTexcoord::attribute_descriptions();
        VertexBinding layout{bd.binding, bd.stride, {}};
        for (const auto& ad : ads)
            layout.attributes.push_back({ad.location, static_cast<uint32_t>(ad.format), ad.offset});
        return layout;
    }

    template<>
    inline VertexBinding vertex_binding<VertexPositionNormalTexcoord>()
    {
        const auto& bd = VertexPositionNormalTexcoord::binding_description();
        const auto ads = VertexPositionNormalTexcoord::attribute_descriptions();
        VertexBinding layout{bd.binding, bd.stride, {}};
        for (const auto& ad : ads)
            layout.attributes.push_back({ad.location, static_cast<uint32_t>(ad.format), ad.offset});
        return layout;
    }

} // namespace quetzal::vulkan

namespace std
{
    template<> struct hash<quetzal::vulkan::VertexPositionColorTexcoord>
    {
        size_t operator()(quetzal::vulkan::VertexPositionColorTexcoord const& vertex) const
        {
            return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texcoord) << 1);
        }
    };

    template<> struct hash<quetzal::vulkan::VertexPositionNormalTexcoord>
    {
        size_t operator()(quetzal::vulkan::VertexPositionNormalTexcoord const& vertex) const
        {
            return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texcoord) << 1);
        }
    };
}

#endif // QUETZAL_VULKAN_VERTEX_HPP
