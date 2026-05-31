#if !defined(QUETZAL_VULKAN_LOGICALDEVICE_HPP)
#define QUETZAL_VULKAN_LOGICALDEVICE_HPP

#include "PhysicalDevice.hpp"
#include "VulkanError.hpp"
#include "initializer.hpp"
#include "vulkan_util.hpp"
#include <vulkan/vulkan.hpp>
#include <ranges>
#include <set>
#include <vector>

namespace quetzal::vulkan
{

    class LogicalDevice
    {
    public:

        LogicalDevice(const PhysicalDevice& physical_device, const std::vector<const char*>& extensions) :
            m_handle(VK_NULL_HANDLE)
        {
            const auto& properties_max = std::ranges::max(physical_device.queue_family_properties(), {}, [](const auto& properties) { return properties.queueCount; });

            std::vector<VkDeviceQueueCreateInfo> queue_cis;
            std::vector<float> queue_priorities(properties_max.queueCount, 1.0f);

            for (int32_t family_index = 0; family_index < physical_device.queue_family_properties().size(); ++family_index)
            {
                int32_t count = physical_device.queue_family_properties()[family_index].queueCount;
                VkDeviceQueueCreateInfo queue_ci{};
                queue_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queue_ci.queueFamilyIndex = family_index;
                queue_ci.queueCount = count;
                queue_ci.pQueuePriorities = queue_priorities.data();
                queue_cis.push_back(queue_ci);
            }

            VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_features = vk_physical_device_dynamic_rendering_features();
            VkPhysicalDeviceSynchronization2Features synchronization2_features = vk_physical_device_synchronization2_features(VK_TRUE, &dynamic_rendering_features);
            VkPhysicalDeviceFeatures2 features2 = vk_physical_device_features2(&synchronization2_features);
            features2.features.samplerAnisotropy = VK_TRUE;

            VkDeviceCreateInfo device_ci = vk_device_create_info(queue_cis, extensions, &features2);
            VkResult vk_result = vkCreateDevice(physical_device, &device_ci, nullptr, &m_handle);
            check_result(vk_result, "vkCreateDevice");
        }

        ~LogicalDevice()
        {
            assert(m_handle != VK_NULL_HANDLE);
            vkDestroyDevice(m_handle, nullptr);
        }

        operator VkDevice() const
        {
            assert(m_handle != VK_NULL_HANDLE);
            return m_handle;
        }

    private:

        VkDevice m_handle;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_LOGICALDEVICE_HPP
