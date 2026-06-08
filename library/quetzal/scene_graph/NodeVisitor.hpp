#if !defined(QUETZAL_SCENE_GRAPH_NODEVISITOR_HPP)
#define QUETZAL_SCENE_GRAPH_NODEVISITOR_HPP

#include "Node.hpp"

namespace quetzal::scene_graph
{

    class CameraNode;
    class GeometryNode;
    class MaterialNode;
    class TransformNode;

    class NodeVisitor
    {
    public:

        virtual ~NodeVisitor() = default;

        virtual void apply(CameraNode&) {}
        virtual void apply(GeometryNode&) {}
        virtual void apply(MaterialNode&) {}
        virtual void apply(TransformNode&) {}

        void traverse(Node& node)
        {
            for (auto& child : node.children())
                child->accept(*this);
        }
    };

} // namespace quetzal::scene_graph

#endif // QUETZAL_SCENE_GRAPH_NODEVISITOR_HPP
