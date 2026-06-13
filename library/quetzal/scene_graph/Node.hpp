#if !defined(QUETZAL_SCENE_GRAPH_NODE_HPP)
#define QUETZAL_SCENE_GRAPH_NODE_HPP

#include <memory>
#include <string>
#include <vector>

namespace quetzal::scene_graph
{

    class NodeVisitor;

    class Node
    {
    public:

        Node(std::string id) :
            m_id(std::move(id)),
            m_parent(nullptr),
            m_children()
        {
        }

        virtual ~Node() = default;

        virtual void accept(NodeVisitor& visitor);

        const std::string& id() const { return m_id; }

        Node* parent() const { return m_parent; }

        const std::vector<std::shared_ptr<Node>>& children() const { return m_children; }

        Node& add_child(std::shared_ptr<Node> child)
        {
            child->m_parent = this;
            return *m_children.emplace_back(std::move(child));
        }

        void clear_children()
        {
            m_children.clear();
        }

    protected:

        std::string m_id;
        Node* m_parent;
        std::vector<std::shared_ptr<Node>> m_children;
    };

} // namespace quetzal::scene_graph

#endif // QUETZAL_SCENE_GRAPH_NODE_HPP
