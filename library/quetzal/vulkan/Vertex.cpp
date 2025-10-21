
#include "Vertex.hpp"

using namespace quetzal::vulkan;

VertexPositionColorTexcoord::binding_description_type VertexPositionColorTexcoord::m_binding_description = {0, sizeof (VertexPositionColorTexcoord), VK_VERTEX_INPUT_RATE_VERTEX};
VertexPositionColorTexcoord::attribute_descriptions_type VertexPositionColorTexcoord::m_attribute_descriptions =
{
    {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexPositionColorTexcoord, position)},
    {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexPositionColorTexcoord, color)},
    {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexPositionColorTexcoord, texcoord)},
};

VertexPositionNormalTexcoord::binding_description_type VertexPositionNormalTexcoord::m_binding_description = {0, sizeof (VertexPositionNormalTexcoord), VK_VERTEX_INPUT_RATE_VERTEX};
VertexPositionNormalTexcoord::attribute_descriptions_type VertexPositionNormalTexcoord::m_attribute_descriptions =
{
    {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexPositionNormalTexcoord, position)},
    {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexPositionNormalTexcoord, normal)},
    {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexPositionNormalTexcoord, texcoord)},
};
