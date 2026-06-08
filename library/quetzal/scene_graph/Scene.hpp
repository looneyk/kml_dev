#if !defined(QUETZAL_SCENE_GRAPH_SCENE_HPP)
#define QUETZAL_SCENE_GRAPH_SCENE_HPP

#include "CameraNode.hpp"
#include "GeometryNode.hpp"
#include "MaterialNode.hpp"
#include "Node.hpp"
#include "RenderVisitor.hpp"
#include "TransformNode.hpp"
#include "quetzal/game/types.hpp"
#include "quetzal/vulkan/IRenderer.hpp"
#include "quetzal/vulkan/Vertex.hpp"
#include <cassert>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

namespace quetzal::scene_graph
{

    class Scene
    {
    public:

        Scene(vulkan::IRenderer& renderer, const std::filesystem::path& resource_path) :
            m_renderer(renderer),
            m_resource_path(resource_path),
            m_root(""),
            m_index(),
            m_camera_node(nullptr)
        {
        }

        Node& root() { return m_root; }
        const Node& root() const { return m_root; }

        Node& insert(Node& parent, std::unique_ptr<Node> node)
        {
            register_subtree(*node);
            return parent.add_child(std::move(node));
        }

        Node& insert(const std::string& parent_id, std::unique_ptr<Node> node)
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

        template<typename V>
        void insert(const std::string& id,
                    const std::filesystem::path& model_path,
                    const std::filesystem::path& texture_path,
                    const game::matrix_type& transform,
                    const std::string& vert_spv = "shaders/vert_pct_pc.spv",
                    const std::string& frag_spv = "shaders/frag_pct.spv");

        void set_camera(const game::matrix_type& view, const game::matrix_type& proj)
        {
            if (m_camera_node == nullptr)
            {
                auto camera = std::make_unique<CameraNode>("__camera__", view, proj);
                m_camera_node = static_cast<CameraNode*>(&insert(m_root, std::move(camera)));
            }
            else
            {
                m_camera_node->set_view(view);
                m_camera_node->set_proj(proj);
            }
        }

        void update_model_instance(const std::string& id, const game::matrix_type& matrix)
        {
            Node* node = find(id);
            assert(node != nullptr);
            TransformNode* transform = dynamic_cast<TransformNode*>(node);
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
            RenderVisitor visitor(m_renderer);
            accept(visitor);
            m_renderer.end_frame();
        }

        void clear()
        {
            m_root.clear_children();
            m_index.clear();
            m_camera_node = nullptr;
        }

    private:

        template<typename V>
        vulkan::GeometryHandle load_model(const std::string& id, const std::filesystem::path& model_path);

        vulkan::TextureHandle load_texture(const std::string& id, const std::filesystem::path& texture_path);

        void register_subtree(Node& node)
        {
            if (!node.id().empty())
                m_index.emplace(node.id(), &node);
            for (const auto& child : node.children())
                register_subtree(*child);
        }

        vulkan::IRenderer& m_renderer;
        std::filesystem::path m_resource_path;
        Node m_root;
        std::unordered_map<std::string, Node*> m_index;
        CameraNode* m_camera_node;
    };

    extern template void Scene::insert<vulkan::VertexPositionColorTexcoord>(const std::string&, const std::filesystem::path&, const std::filesystem::path&, const game::matrix_type&, const std::string&, const std::string&);
    extern template void Scene::insert<vulkan::VertexPositionNormalTexcoord>(const std::string&, const std::filesystem::path&, const std::filesystem::path&, const game::matrix_type&, const std::string&, const std::string&);

} // namespace quetzal::scene_graph

#endif // QUETZAL_SCENE_GRAPH_SCENE_HPP
