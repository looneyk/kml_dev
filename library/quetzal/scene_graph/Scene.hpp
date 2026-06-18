#if !defined(QUETZAL_SCENE_GRAPH_SCENE_HPP)
#define QUETZAL_SCENE_GRAPH_SCENE_HPP

#include "AssetManager.hpp"
#include "GeometryNode.hpp"
#include "MaterialNode.hpp"
#include "Node.hpp"
#include "RenderVisitor.hpp"
#include "TransformNode.hpp"
#include "ShaderManager.hpp"
#include "quetzal/game/types.hpp"
#include "quetzal/vulkan/IRenderer.hpp"
#include "quetzal/vulkan/Vertex.hpp"
#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>

namespace quetzal::scene_graph
{

    class Scene
    {
    public:

        Scene(vulkan::IRenderer& renderer) :
            m_renderer(renderer),
            m_root(""),
            m_index(),
            m_view(1.0f),
            m_proj(1.0f)
        {
        }

        Node& root() { return m_root; }
        const Node& root() const { return m_root; }

        Node& insert(Node& parent, std::shared_ptr<Node> node)
        {
            register_subtree(*node);
            return parent.add_child(std::move(node));
        }

        Node& insert(const std::string& parent_id, std::shared_ptr<Node> node)
        {
            Node* parent = find(parent_id);
            assert(parent != nullptr);
            return insert(*parent, std::move(node));
        }

        Node* find(const std::string& id)
        {
            auto i = m_index.find(id);
            return i != m_index.end() ? i->second : nullptr;
        }

        const Node* find(const std::string& id) const
        {
            auto i = m_index.find(id);
            return i != m_index.end() ? i->second : nullptr;
        }

        template<typename T>
        T* find(const std::string& id)
        {
            return dynamic_cast<T*>(find(id));
        }

        template<typename T>
        const T* find(const std::string& id) const
        {
            return dynamic_cast<const T*>(find(id));
        }

        void set_camera(const game::matrix_type& view, const game::matrix_type& proj)
        {
            m_view = view;
            m_proj = proj;
        }

        void update_model_instance(const std::string& id, const game::matrix_type& matrix)
        {
            TransformNode* transform = find<TransformNode>(id);
            assert(transform != nullptr);
            transform->set_local_transform(transform->local_transform() * matrix);
        }

        void accept(NodeVisitor& visitor)
        {
            visitor.traverse(m_root);
        }

        void draw()
        {
            if (!m_renderer.begin_frame())
                return;
            m_renderer.set_camera(m_view, m_proj);
            RenderVisitor visitor(m_renderer);
            accept(visitor);
            m_renderer.set_lights(visitor.lights());
            m_renderer.end_frame();
        }

        void clear()
        {
            m_root.clear_children();
            m_index.clear();
        }

    private:

        void register_subtree(Node& node)
        {
            if (!node.id().empty())
                m_index.emplace(node.id(), &node);
            for (const auto& child : node.children())
                register_subtree(*child);
        }

        vulkan::IRenderer& m_renderer;
        Node m_root;
        std::unordered_map<std::string, Node*> m_index;
        game::matrix_type m_view;
        game::matrix_type m_proj;
    };

    template<typename V>
    void load(Scene& scene,
              vulkan::IRenderer& renderer,
              AssetManager& assets,
              const std::string& id,
              const std::filesystem::path& model_path,
              const std::filesystem::path& texture_path,
              const game::matrix_type& transform,
              ShaderManager& shaders,
              const std::string& effect_name);

    extern template void load<vulkan::VertexPositionColorTexcoord>(Scene&, vulkan::IRenderer&, AssetManager&, const std::string&, const std::filesystem::path&, const std::filesystem::path&, const game::matrix_type&, ShaderManager&, const std::string&);
    extern template void load<vulkan::VertexPositionNormalTexcoord>(Scene&, vulkan::IRenderer&, AssetManager&, const std::string&, const std::filesystem::path&, const std::filesystem::path&, const game::matrix_type&, ShaderManager&, const std::string&);

} // namespace quetzal::scene_graph

#endif // QUETZAL_SCENE_GRAPH_SCENE_HPP
