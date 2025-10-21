#if !defined(QUETZAL_VULKAN_IMAGE_HPP)
#define QUETZAL_VULKAN_IMAGE_HPP

#include "Device.hpp"
#include <vulkan/vulkan.hpp>

namespace quetzal::vulkan
{

    class Image
    {
    public:

        Image(const Device& device) :
            m_device(device),
            m_handle(VK_NULL_HANDLE),
            m_allocation(VK_NULL_HANDLE)
        {
        }

        Image(const Device& device, VkImage handle) :
            m_device(device),
            m_handle(handle),
            m_allocation(VK_NULL_HANDLE)
        {
        }

        operator VkImage() const
        {
            assert(m_handle != VK_NULL_HANDLE);
            return m_handle;
        }

        void create(VkExtent2D extent, VkFormat format, VkImageUsageFlags usage, VkSampleCountFlagBits sampleCount, uint32_t mipLevels, VkImageTiling tiling, VmaAllocationCreateFlags flags)
        {
            VkImageCreateInfo image_ci = vk_image_create_info(extent, format, usage, sampleCount, mipLevels, tiling);
            VmaAllocationCreateInfo allocation_ci = Allocator::allocation_create_info(flags);
            VkResult result = vmaCreateImage(m_device.allocator(), &image_ci, &allocation_ci, &m_handle, &m_allocation, nullptr);
            if (result != VK_SUCCESS)
            {
                throw std::runtime_error(std::format("Error vmaCreateImage {}", result));
            }
        }

        void destroy()
        {
            vmaDestroyImage(m_device.allocator(), m_handle, m_allocation);
        }

    private:

        const Device& m_device;
        VkImage m_handle;
        VmaAllocation m_allocation;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_IMAGE_HPP
