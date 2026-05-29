#if !defined(QUETZAL_VULKAN_ALLOCATOR_HPP)
#define QUETZAL_VULKAN_ALLOCATOR_HPP

#include "Instance.hpp"
#include "LogicalDevice.hpp"
#include "PhysicalDevice.hpp"
#include "VulkanError.hpp"
#include "vulkan_util.hpp"
#include <vk_mem_alloc.h>

namespace quetzal::vulkan
{

    class Allocator
    {
    public:

        Allocator(const Instance& instance, const PhysicalDevice& physicalDevice, const LogicalDevice& logicalDevice) :
            m_handle(VK_NULL_HANDLE)
        {
            VmaAllocatorCreateInfo ci{};
            ci.flags = 0;
            ci.vulkanApiVersion = VK_API_VERSION_1_3;
            ci.physicalDevice = physicalDevice;
            ci.device = logicalDevice;
            ci.instance = instance;
            ci.pVulkanFunctions = nullptr;

            VkResult vk_result = vmaCreateAllocator(&ci, &m_handle);
            check_result(vk_result, "vmaCreateAllocator");
        }

        ~Allocator()
        {
            vmaDestroyAllocator(m_handle);
        }

        operator VmaAllocator() const
        {
            assert(m_handle != VK_NULL_HANDLE);
            return m_handle;
        }

        static VmaAllocationCreateInfo allocation_create_info(VmaAllocationCreateFlags flags, VmaMemoryUsage usage = VMA_MEMORY_USAGE_AUTO)
        {
            VmaAllocationCreateInfo ci{};
            ci.flags = flags;
            ci.usage = usage;
            return ci;
        }

    private:

        VmaAllocator m_handle;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_ALLOCATOR_HPP
