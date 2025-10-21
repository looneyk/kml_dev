#if !defined(QUETZAL_VULKAN_BUFFER_HPP)
#define QUETZAL_VULKAN_BUFFER_HPP

#include "Device.hpp"
#include "initializer.hpp"
#include <vulkan/vulkan.hpp>

namespace quetzal::vulkan
{

    class Buffer
    {
    public:

        Buffer(const Device& device) :
            m_device(device),
            m_handle(VK_NULL_HANDLE),
            m_allocation(VK_NULL_HANDLE)
        {
        }

        virtual ~Buffer()
        {
        }

        operator VkBuffer() const
        {
            assert(m_handle != VK_NULL_HANDLE);
            return m_handle;
        }

        operator VmaAllocation() const
        {
            assert(m_allocation != VK_NULL_HANDLE);
            return m_allocation;
        }

        void create(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags)
        {
            VkBufferCreateInfo buffer_ci = vk_buffer_create_info(size, usage);
            VmaAllocationCreateInfo allocation_ci = Allocator::allocation_create_info(flags);

            VkResult result = vmaCreateBuffer(m_device.allocator(), &buffer_ci, &allocation_ci, &m_handle, &m_allocation, nullptr);
            if (result != VK_SUCCESS)
            {
                throw std::runtime_error(std::format("Error vmaCreateBuffer {}", result));
            }
        }

        void destroy()
        {
            if (m_handle != VK_NULL_HANDLE)
            {
                vmaDestroyBuffer(m_device.allocator(), m_handle, m_allocation);
                m_handle = VK_NULL_HANDLE;
            }
        }

        VkDescriptorBufferInfo descriptor() const
        {
            VmaAllocationInfo allocationInfo{};
            vmaGetAllocationInfo(m_device.allocator(), m_allocation, &allocationInfo);

            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = m_handle;
            bufferInfo.offset = 0; // allocationInfo.offset; ? ...
            bufferInfo.range = allocationInfo.size;
            return bufferInfo;
        }

        void map(void*& data)
        {
            vmaMapMemory(m_device.allocator(), m_allocation, &data);
        }

        void unmap()
        {
            vmaUnmapMemory(m_device.allocator(), m_allocation);
        }

        void copy(std::span<const uint8_t> buffer)
        {
            void* data = nullptr;
            map(data);
            memcpy(data, buffer.data(), buffer.size());
            unmap();
        }

        void copy(const void* bytes, size_t size) // deprecated ...
        {
            void* data;
            map(data);
            memcpy(data, bytes, size);
            unmap();
        }

    protected:

        const Device& m_device;
        VkBuffer m_handle;
        VmaAllocation m_allocation;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_BUFFER_HPP
