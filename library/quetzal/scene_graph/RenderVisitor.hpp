#if !defined(QUETZAL_SCENE_GRAPH_RENDERVISITOR_HPP)
#define QUETZAL_SCENE_GRAPH_RENDERVISITOR_HPP

#include "CameraNode.hpp"
#include "GeometryNode.hpp"
#include "MaterialNode.hpp"
#include "NodeVisitor.hpp"
#include "TransformNode.hpp"
#include "quetzal/game/types.hpp"
#include "quetzal/vulkan/IRenderer.hpp"
#include <vector>

namespace quetzal::scene_graph
{

    class RenderVisitor : public NodeVisitor
    {
    public:

        RenderVisitor(vulkan::IRenderer& renderer) :
            m_renderer(renderer),
            m_transform_stack(1, game::matrix_type(1.0f)),
            m_material_stack()
        {
        }

        void apply(CameraNode& node) override
        {
            m_renderer.set_camera(node.view(), node.proj());
            traverse(node);
        }

        void apply(TransformNode& node) override
        {
            m_transform_stack.push_back(m_transform_stack.back() * node.local_transform());
            traverse(node);
            m_transform_stack.pop_back();
        }

        void apply(MaterialNode& node) override
        {
            m_material_stack.push_back(node.material());
            traverse(node);
            m_material_stack.pop_back();
        }

        void apply(GeometryNode& node) override
        {
            if (!m_material_stack.empty())
            {
                m_renderer.draw_geometry(node.geometry(), m_material_stack.back(), m_transform_stack.back());
            }
            traverse(node);
        }

    private:

        vulkan::IRenderer& m_renderer;
        std::vector<game::matrix_type> m_transform_stack;
        std::vector<vulkan::MaterialHandle> m_material_stack;
    };

} // namespace quetzal::scene_graph

#endif // QUETZAL_SCENE_GRAPH_RENDERVISITOR_HPP
