#if !defined(QUETZAL_VULKAN_COMMANDBUFFERS_HPP)
#define QUETZAL_VULKAN_COMMANDBUFFERS_HPP

#include "CommandBuffer.hpp"
#include "CommandPool.hpp"
#include "Device.hpp"
#include <vulkan/vulkan.hpp>
#include <vector>

namespace quetzal::vulkan
{

    class CommandBuffers
    {
    public:

        CommandBuffers(const Device& device, const CommandPool& command_pool) :
            m_device(device),
            m_command_pool(command_pool),
            m_command_buffers()
        {
        }

        CommandBuffer& operator[](size_t i)
        {
            assert(i < m_command_buffers.size());
            assert(m_command_buffers[i] != VK_NULL_HANDLE);
            return m_command_buffers[i];
        }

        void create(size_t count)
        {
            assert(m_command_buffers.size() == 0);

            m_command_buffers.resize(count, CommandBuffer(m_device, m_command_pool));
            for (auto& command_buffer : m_command_buffers)
            {
                command_buffer.create();
            }
        }

        void destroy()
        {
            std::vector<VkCommandBuffer> handles;
            std::transform(m_command_buffers.begin(), m_command_buffers.end(), handles.begin(), [](const CommandBuffer& command_buffer) { return command_buffer.handle(); });

            vkFreeCommandBuffers(m_device, m_command_pool, to_uint32(handles.size()), handles.data());
            m_command_buffers.clear();
        }

        size_t size() const
        {
            return m_command_buffers.size();
        }

        const CommandBuffer* data() const
        {
            return m_command_buffers.data();
        }

    private:

        const Device& m_device;
        const CommandPool& m_command_pool;
        std::vector<CommandBuffer> m_command_buffers;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_COMMANDBUFFERS_HPP
