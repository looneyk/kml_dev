#if !defined(QUETZAL_VULKAN_MODEL_HPP)
#define QUETZAL_VULKAN_MODEL_HPP

#include "Buffer.hpp"
#include "Device.hpp"
#include "SingleSubmitCommands.hpp"
#include "StagingBuffer.hpp"
#include <tiny_obj_loader.h>
#include <vulkan/vulkan.hpp>
#include <filesystem>
#include <string>
#include <vector>

namespace quetzal::vulkan
{

    class Model
    {
    public:

        using index_type = uint32_t;
        using indices_type = std::vector<index_type>;
        using buffer_type = std::span<const uint8_t>;

        Model(const Device& device, const CommandPool& command_pool) :
            m_device(device),
            m_command_pool(command_pool),
            m_vertex_buffer(device),
            m_index_buffer(device),
            m_nIndices(0)
        {
        }

        ~Model()
        {
            clear();
        }

        template<typename V>
        void load(const std::filesystem::path& filepath)
        {
            using vertices_type = std::vector<V>;

            vertices_type vertices;
            indices_type indices;

            tinyobj::attrib_t attrib;
            std::vector<tinyobj::shape_t> shapes;
            std::vector<tinyobj::material_t> materials;
            std::string warn, err;

            if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.string().c_str()))
            {
                throw std::runtime_error(warn + err);
            }

            std::unordered_map<V, uint32_t> uniqueVertices{};

            for (const auto& shape : shapes)
            {
                for (const auto& index : shape.mesh.indices)
                {
                    V vertex(attrib, index);

                    if (uniqueVertices.count(vertex) == 0)
                    {
                        uniqueVertices[vertex] = to_uint32(vertices.size());
                        vertices.push_back(vertex);
                    }

                    indices.push_back(uniqueVertices[vertex]);
                }
            }

            m_nIndices = indices.size();

            create_vertex_buffer(vertices);
            create_index_buffer(indices);

            vertices.clear();
            indices.clear();
            return;
        }

        void clear()
        {
            m_vertex_buffer.destroy();
            m_index_buffer.destroy();
            return;
        }

        Buffer vertex_buffer() const
        {
            return m_vertex_buffer;
        }

        Buffer index_buffer() const
        {
            return m_index_buffer;
        }

        size_t index_count() const
        {
            return m_nIndices;
        }

    private:

        template<typename V>
        buffer_type vertex_data_buffer(const std::vector<V>& vertices) const
        {
            return {reinterpret_cast<const uint8_t*>(vertices.data()), sizeof(V) * vertices.size()};
        }

        buffer_type index_data_buffer(const indices_type& indices) const
        {
            return {reinterpret_cast<const uint8_t*>(indices.data()), sizeof(index_type) * indices.size()};
        }

        template<typename V>
        void create_vertex_buffer(const std::vector<V>& vertices)
        {
            auto data_buffer = vertex_data_buffer(vertices);

            StagingBuffer staging_buffer(m_device, data_buffer.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
            staging_buffer.copy(data_buffer);

            m_vertex_buffer.create(data_buffer.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0);
            copy_buffer(staging_buffer, m_vertex_buffer, data_buffer.size());
        }

        void create_index_buffer(const indices_type& indices)
        {
            auto data_buffer = index_data_buffer(indices);

            StagingBuffer staging_buffer(m_device, data_buffer.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
            staging_buffer.copy(data_buffer);

            m_index_buffer.create(data_buffer.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 0);
            copy_buffer(staging_buffer, m_index_buffer, data_buffer.size());
        }

        void copy_buffer(VkBuffer buffer_from, VkBuffer buffer_to, VkDeviceSize size)
        {
            const Queue& graphics_queue = m_device.queues().get(VK_QUEUE_GRAPHICS_BIT, 0);
            SingleSubmitCommands commands(m_device, m_command_pool, graphics_queue);

            VkBufferCopy region{};
            region.size = size;
            vkCmdCopyBuffer(commands, buffer_from, buffer_to, 1, &region);

            commands.submit();
        }

        const Device& m_device;
        const CommandPool& m_command_pool;

        Buffer m_vertex_buffer;
        Buffer m_index_buffer;
        size_t m_nIndices;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_MODEL_HPP
