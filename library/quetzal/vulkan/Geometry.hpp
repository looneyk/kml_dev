#if !defined(QUETZAL_VULKAN_GEOMETRY_HPP)
#define QUETZAL_VULKAN_GEOMETRY_HPP

#include "Buffer.hpp"
#include "Device.hpp"
#include "SingleSubmitCommands.hpp"
#include "StagingBuffer.hpp"
#include <vulkan/vulkan.hpp>
#include <span>
#include <vector>

namespace quetzal::vulkan
{

    class Geometry
    {
    public:

        using index_type = uint32_t;
        using indices_type = std::vector<index_type>;
        using buffer_type = std::span<const uint8_t>;

        Geometry(const Device& device, const CommandPool& command_pool) :
            m_device(device),
            m_command_pool(command_pool),
            m_vertex_buffer(device),
            m_index_buffer(device),
            m_index_count(0)
        {
        }

        ~Geometry()
        {
            clear();
        }

        void upload(buffer_type vertices, std::span<const index_type> indices)
        {
            m_index_count = indices.size();
            create_vertex_buffer(vertices);
            create_index_buffer({reinterpret_cast<const uint8_t*>(indices.data()), indices.size_bytes()});
        }

        template<typename V>
        void upload(std::span<const V> vertices, std::span<const index_type> indices)
        {
            upload({reinterpret_cast<const uint8_t*>(vertices.data()), vertices.size_bytes()}, indices);
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
            return m_index_count;
        }

    private:

        void create_vertex_buffer(buffer_type data)
        {
            StagingBuffer staging_buffer(m_device, data.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
            staging_buffer.copy(data);
            m_vertex_buffer.create(data.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0);
            copy_buffer(staging_buffer, m_vertex_buffer, data.size());
        }

        void create_index_buffer(buffer_type data)
        {
            StagingBuffer staging_buffer(m_device, data.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
            staging_buffer.copy(data);
            m_index_buffer.create(data.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 0);
            copy_buffer(staging_buffer, m_index_buffer, data.size());
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
        size_t m_index_count;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_GEOMETRY_HPP
