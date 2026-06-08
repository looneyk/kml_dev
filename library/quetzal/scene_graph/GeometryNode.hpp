#if !defined(QUETZAL_SCENE_GRAPH_GEOMETRYNODE_HPP)
#define QUETZAL_SCENE_GRAPH_GEOMETRYNODE_HPP

#include "Node.hpp"
#include "NodeVisitor.hpp"
#include "quetzal/vulkan/handles.hpp"

namespace quetzal::scene_graph
{

    class GeometryNode : public Node
    {
    public:

        GeometryNode(std::string id, vulkan::GeometryHandle geometry) :
            Node(std::move(id)),
            m_geometry(std::move(geometry))
        {
        }

        void accept(NodeVisitor& visitor) override { visitor.apply(*this); }

        const vulkan::GeometryHandle& geometry() const { return m_geometry; }

    private:

        vulkan::GeometryHandle m_geometry;
    };

} // namespace quetzal::scene_graph

#endif // QUETZAL_SCENE_GRAPH_GEOMETRYNODE_HPP
