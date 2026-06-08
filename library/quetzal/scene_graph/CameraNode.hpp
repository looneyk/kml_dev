#if !defined(QUETZAL_SCENE_GRAPH_CAMERANODE_HPP)
#define QUETZAL_SCENE_GRAPH_CAMERANODE_HPP

#include "Node.hpp"
#include "NodeVisitor.hpp"
#include "quetzal/game/types.hpp"

namespace quetzal::scene_graph
{

    class CameraNode : public Node
    {
    public:

        CameraNode(std::string id, const game::matrix_type& view, const game::matrix_type& proj) :
            Node(std::move(id)),
            m_view(view),
            m_proj(proj)
        {
        }

        void accept(NodeVisitor& visitor) override { visitor.apply(*this); }

        const game::matrix_type& view() const { return m_view; }
        const game::matrix_type& proj() const { return m_proj; }

        void set_view(const game::matrix_type& m) { m_view = m; }
        void set_proj(const game::matrix_type& m) { m_proj = m; }

    private:

        game::matrix_type m_view;
        game::matrix_type m_proj;
    };

} // namespace quetzal::scene_graph

#endif // QUETZAL_SCENE_GRAPH_CAMERANODE_HPP
