#if !defined(QUETZAL_VULKAN_STAGINGBUFFER_HPP)
#define QUETZAL_VULKAN_STAGINGBUFFER_HPP

#include "Device.hpp"
#include "Buffer.hpp"
#include <vulkan/vulkan.hpp>

namespace quetzal::vulkan
{

    class StagingBuffer : public Buffer
    {
    public:

        StagingBuffer(const Device& device, VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags) :
            Buffer(device)
        {
            create(size, usage, flags);
        }

        ~StagingBuffer()
        {
            vmaDestroyBuffer(m_device.allocator(), m_handle, m_allocation);
        }
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_STAGINGBUFFER_HPP
