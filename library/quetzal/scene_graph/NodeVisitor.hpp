#if !defined(QUETZAL_SCENE_GRAPH_NODEVISITOR_HPP)
#define QUETZAL_SCENE_GRAPH_NODEVISITOR_HPP

#include "Node.hpp"

namespace quetzal::scene_graph
{

    class GeometryNode;
    class LightNode;
    class MaterialNode;
    class TransformNode;

    class NodeVisitor
    {
    public:

        virtual ~NodeVisitor() = default;

        virtual void apply(GeometryNode&) {}
        virtual void apply(LightNode&) {}
        virtual void apply(MaterialNode&) {}
        virtual void apply(TransformNode&) {}

        void traverse(Node& node)
        {
            for (auto& child : node.children())
                child->accept(*this);
        }
    };

    inline void Node::accept(NodeVisitor& visitor) { visitor.traverse(*this); }

} // namespace quetzal::scene_graph

#endif // QUETZAL_SCENE_GRAPH_NODEVISITOR_HPP
