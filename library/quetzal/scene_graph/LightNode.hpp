#if !defined(QUETZAL_SCENE_GRAPH_LIGHTNODE_HPP)
#define QUETZAL_SCENE_GRAPH_LIGHTNODE_HPP

#include "Node.hpp"
#include "NodeVisitor.hpp"
#include "quetzal/game/types.hpp"

namespace quetzal::scene_graph
{

    class LightNode : public Node
    {
    public:

        LightNode(std::string id, const game::vector_type& color, float intensity) :
            Node(std::move(id)),
            m_color(color),
            m_intensity(intensity)
        {
        }

        void accept(NodeVisitor& visitor) override { visitor.apply(*this); }

        const game::vector_type& color() const { return m_color; }
        float intensity() const { return m_intensity; }

    private:

        game::vector_type m_color;
        float m_intensity;
    };

} // namespace quetzal::scene_graph

#endif // QUETZAL_SCENE_GRAPH_LIGHTNODE_HPP
