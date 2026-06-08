#if !defined(QUETZAL_SCENE_GRAPH_TRANSFORMNODE_HPP)
#define QUETZAL_SCENE_GRAPH_TRANSFORMNODE_HPP

#include "Node.hpp"
#include "NodeVisitor.hpp"
#include "quetzal/game/types.hpp"

namespace quetzal::scene_graph
{

    class TransformNode : public Node
    {
    public:

        TransformNode(std::string id, const game::matrix_type& local_transform = game::matrix_type(1.0f)) :
            Node(std::move(id)),
            m_local_transform(local_transform)
        {
        }

        void accept(NodeVisitor& visitor) override { visitor.apply(*this); }

        const game::matrix_type& local_transform() const { return m_local_transform; }

        void set_local_transform(const game::matrix_type& m) { m_local_transform = m; }

    private:

        game::matrix_type m_local_transform;
    };

} // namespace quetzal::scene_graph

#endif // QUETZAL_SCENE_GRAPH_TRANSFORMNODE_HPP
