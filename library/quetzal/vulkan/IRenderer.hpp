#if !defined(QUETZAL_VULKAN_IRENDERER_HPP)
#define QUETZAL_VULKAN_IRENDERER_HPP

#include "handles.hpp"
#include "quetzal/game/types.hpp"
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace quetzal::vulkan
{

    struct VertexAttribute
    {
        uint32_t location;
        uint32_t format;
        uint32_t offset;
    };

    struct VertexBinding
    {
        uint32_t binding;
        uint32_t stride;
        std::vector<VertexAttribute> attributes;
    };

    template<typename V>
    VertexBinding vertex_binding();

    struct Light
    {
        game::vector_type position;
        game::vector_type color;
        float intensity;
    };

    class IRenderer
    {
    public:

        virtual ~IRenderer() = default;

        virtual bool begin_frame() = 0;
        virtual void end_frame() = 0;
        virtual void set_camera(const game::matrix_type& view, const game::matrix_type& proj) = 0;
        virtual void draw_geometry(const GeometryHandle& geometry, const MaterialHandle& material, const game::matrix_type& model) = 0;
        virtual void set_lights(std::span<const Light> lights) = 0;
        virtual TextureHandle upload_texture(const std::string& id, const void* pixels, uint32_t width, uint32_t height) = 0;
        virtual GeometryHandle upload_geometry(const std::string& id, const void* vertex_data, size_t vertex_count, const VertexBinding& layout, std::span<const uint32_t> indices) = 0;
        virtual EffectHandle create_effect(const std::string& vert_spv, const std::string& frag_spv, const VertexBinding& layout) = 0;
        virtual MaterialHandle create_material(const EffectHandle& effect, const TextureHandle& texture) = 0;

        template<typename V>
        GeometryHandle upload_geometry(const std::string& id, std::span<const V> vertices, std::span<const uint32_t> indices)
        {
            return upload_geometry(id, vertices.data(), vertices.size(), vertex_binding<V>(), indices);
        }

        template<typename V>
        EffectHandle create_effect(const std::string& vert_spv, const std::string& frag_spv)
        {
            return create_effect(vert_spv, frag_spv, vertex_binding<V>());
        }
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_IRENDERER_HPP
