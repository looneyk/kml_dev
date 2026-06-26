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

        MaterialNode(std::string id, vulkan::MaterialInstanceHandle material) :
            Node(std::move(id)),
            m_material(std::move(material))
        {
        }

        void accept(NodeVisitor& visitor) override { visitor.apply(*this); }

        const vulkan::MaterialInstanceHandle& material() const { return m_material; }

        void set_material(vulkan::MaterialInstanceHandle material) { m_material = std::move(material); }

    private:

        vulkan::MaterialInstanceHandle m_material;
    };

} // namespace quetzal::scene_graph

#endif // QUETZAL_SCENE_GRAPH_MATERIALNODE_HPP
