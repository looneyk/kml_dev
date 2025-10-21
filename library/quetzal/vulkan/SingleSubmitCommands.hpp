#if !defined(QUETZAL_VULKAN_SINGLESUBMITCOMMANDS_HPP)
#define QUETZAL_VULKAN_SINGLESUBMITCOMMANDS_HPP

#include "CommandBuffer.hpp"
#include "CommandPool.hpp"
#include "Device.hpp"
#include "Queue.hpp"
#include <vulkan/vulkan.hpp>
#include <array>

namespace quetzal::vulkan
{

    class SingleSubmitCommands
    {
    public:

        SingleSubmitCommands(const Device& device, const CommandPool& command_pool, const Queue& queue) :
            m_device(device),
            m_queue(queue),
            m_command_buffer(m_device, command_pool)
        {
            m_command_buffer.create();
            m_command_buffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        }

        ~SingleSubmitCommands()
        {
            m_queue.wait_idle();
            m_command_buffer.destroy();
        }

        operator VkCommandBuffer() const
        {
            return m_command_buffer.handle();
        }

        void submit()
        {
            m_command_buffer.end();

            std::array<VkCommandBuffer, 1> handles = {m_command_buffer};
            VkSubmitInfo submit_info = vk_submit_info(handles);
            m_queue.submit({&submit_info, 1}, VK_NULL_HANDLE);
        }

    private:

        const Device& m_device;
        const Queue& m_queue;
        CommandBuffer m_command_buffer;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_SINGLESUBMITCOMMANDS_HPP
