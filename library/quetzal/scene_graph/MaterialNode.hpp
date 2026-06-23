#if !defined(QUETZAL_SCENE_GRAPH_MATERIALNODE_HPP)
#define QUETZAL_SCENE_GRAPH_MATERIALNODE_HPP

#include "Node.hpp"
#include "NodeVisitor.hpp"
#include "quetzal/vulkan/handles.hpp"
#include "quetzal/game/types.hpp"

namespace quetzal::scene_graph
{

    class MaterialNode : public Node
    {
    public:

        MaterialNode(std::string id, vulkan::MaterialHandle material, glm::vec4 color = glm::vec4(1.0f)) :
            Node(std::move(id)),
            m_material(std::move(material)),
            m_color(color)
        {
        }

        void accept(NodeVisitor& visitor) override { visitor.apply(*this); }

        const vulkan::MaterialHandle& material() const { return m_material; }

        void set_material(vulkan::MaterialHandle material) { m_material = std::move(material); }

        const glm::vec4& color() const { return m_color; }

        void set_color(const glm::vec4& color) { m_color = color; }

    private:

        vulkan::MaterialHandle m_material;
        glm::vec4 m_color;
    };

} // namespace quetzal::scene_graph

#endif // QUETZAL_SCENE_GRAPH_MATERIALNODE_HPP
