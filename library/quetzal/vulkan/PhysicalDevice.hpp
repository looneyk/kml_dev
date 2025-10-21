#if !defined(QUETZAL_VULKAN_PHYSICALDEVICE_HPP)
#define QUETZAL_VULKAN_PHYSICALDEVICE_HPP

#include "Instance.hpp"
#include "PhysicalDevices.hpp"
#include "Surface.hpp"
#include <vulkan/vulkan.hpp>
#include <functional>
#include <optional>
#include <set>

namespace quetzal::vulkan
{

    struct SurfaceSupport
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> present_modes;
    };

    class PhysicalDevice
    {
    public:

        using selector_type = std::function<bool(const PhysicalDevice&, const Surface&)>;

        PhysicalDevice(const PhysicalDevices& physical_devices, const Surface& surface, selector_type selector) :
            m_handle(VK_NULL_HANDLE)
        {
            for (auto physical_device : physical_devices)
            {
                if (selector(physical_device, surface))
                {
                    m_handle = physical_device;
                    break;
                }
            }

            if (m_handle == VK_NULL_HANDLE)
            {
                throw std::runtime_error("Unable to find a suitable physical device");
            }
        }

        PhysicalDevice(VkPhysicalDevice handle) :
            m_handle(handle)
        {
        }

        operator VkPhysicalDevice() const
        {
            assert(m_handle != VK_NULL_HANDLE);
            return m_handle;
        }

        bool queue_family_presentable(uint32_t family_index, const Surface& surface) const
        {
            auto qfp = queue_family_properties();

            auto flags = qfp[family_index].queueFlags;
            if (flags & VK_QUEUE_GRAPHICS_BIT)
            {
                VkBool32 present_support = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(m_handle, family_index, surface, &present_support);
                if (present_support)
                {
                    return true;
                }
            }

            return false;
        }

        bool queue_family_presentable(const Surface& surface) const
        {
            auto qfp = queue_family_properties();

            int family_index = 0;
            for (const auto& properties : qfp)
            {
                auto flags = properties.queueFlags;
                if (flags & VK_QUEUE_GRAPHICS_BIT)
                {
                    VkBool32 present_support = false;
                    vkGetPhysicalDeviceSurfaceSupportKHR(m_handle, family_index, surface, &present_support);
                    if (present_support)
                    {
                        return true;
                    }
                }

                ++family_index;
            }

            return false;
        }

        VkPhysicalDeviceProperties properties() const
        {
            assert(m_handle != VK_NULL_HANDLE);
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(m_handle, &properties);
            return properties;
        }

        VkPhysicalDeviceMemoryProperties memory_properties() const
        {
            assert(m_handle != VK_NULL_HANDLE);
            VkPhysicalDeviceMemoryProperties properties{};
            vkGetPhysicalDeviceMemoryProperties(m_handle, &properties);
            return properties;
        }

        const std::vector<VkQueueFamilyProperties> queue_family_properties() const
        {
            assert(m_handle != VK_NULL_HANDLE);
            uint32_t count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(m_handle, &count, nullptr);

            std::vector<VkQueueFamilyProperties> properties(count);
            vkGetPhysicalDeviceQueueFamilyProperties(m_handle, &count, properties.data());

            return properties;
        }

        VkFormatProperties format_properties(VkFormat format) const
        {
            assert(m_handle != VK_NULL_HANDLE);
            VkFormatProperties format_properties{};
            vkGetPhysicalDeviceFormatProperties(m_handle, format, &format_properties);
            return format_properties;
        }

        VkFormat find_supported_format(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags features) const
        {
            for (VkFormat format : formats)
            {
                VkFormatProperties properties = format_properties(format);
                if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
                {
                    return format;
                }
                else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
                {
                    return format;
                }
            }

            throw std::runtime_error("Error finding supported format");
        }

        VkFormat find_depth_format() const
        {
            return find_supported_format({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
        }

        VkSampleCountFlagBits max_sample_count() const
        {
            auto physical_device_properties = properties();
            VkSampleCountFlags count_flags = physical_device_properties.limits.framebufferColorSampleCounts & physical_device_properties.limits.framebufferDepthSampleCounts;

            if (count_flags & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
            if (count_flags & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
            if (count_flags & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
            if (count_flags & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
            if (count_flags & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
            if (count_flags & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }
            return VK_SAMPLE_COUNT_1_BIT;
        }

        uint32_t find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
        {
            VkPhysicalDeviceMemoryProperties memProperties = memory_properties();

            for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
            {
                if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
                {
                    return i;
                }
            }

            throw std::runtime_error("Error finding suitable memory type");
        }

        bool check_extension_support(const std::vector<const char*>& extensions) const
        {
            ExtensionProperties extensions_available(m_handle);
            std::set<std::string> extensions_required(extensions.begin(), extensions.end());

            for (const auto& extension : extensions_available)
            {
                extensions_required.erase(extension.extensionName);
            }

            return extensions_required.empty();
        }

        VkSurfaceCapabilitiesKHR surface_capabilities(VkSurfaceKHR surface) const
        {
            assert(m_handle != VK_NULL_HANDLE);

            VkSurfaceCapabilitiesKHR capabilities{};
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_handle, surface, &capabilities);
            return capabilities;
        }

        std::vector<VkSurfaceFormatKHR> surface_formats(VkSurfaceKHR surface) const
        {
            assert(m_handle != VK_NULL_HANDLE);

            std::vector<VkSurfaceFormatKHR> formats;

            uint32_t count;
            vkGetPhysicalDeviceSurfaceFormatsKHR(m_handle, surface, &count, nullptr);
            if (count != 0)
            {
                formats.resize(count);
                vkGetPhysicalDeviceSurfaceFormatsKHR(m_handle, surface, &count, formats.data());
            }

            return formats;
        }

        std::vector<VkPresentModeKHR> present_modes(VkSurfaceKHR surface) const
        {
            assert(m_handle != VK_NULL_HANDLE);

            std::vector<VkPresentModeKHR> modes;

            uint32_t count;
            vkGetPhysicalDeviceSurfacePresentModesKHR(m_handle, surface, &count, nullptr);
            if (count != 0)
            {
                modes.resize(count);
                vkGetPhysicalDeviceSurfacePresentModesKHR(m_handle, surface, &count, modes.data());
            }

            return modes;
        }

        // cached, reevaluated on surface change ...
        SurfaceSupport surface_support(VkSurfaceKHR surface) const
        {
            SurfaceSupport support;
            support.capabilities = surface_capabilities(surface);
            support.formats = surface_formats(surface);
            support.present_modes = present_modes(surface);
            return support;
        }

    private:

        class ExtensionProperties
        {
        public:

            ExtensionProperties(VkPhysicalDevice physical_device) :
                m_properties()
            {
                uint32_t count = 0;
                vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &count, nullptr);

                m_properties.resize(count);
                vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &count, m_properties.data());
            };

            auto begin() { return m_properties.begin(); }
            auto end() { return m_properties.end(); }

        private:

            std::vector<VkExtensionProperties> m_properties;
        };

        VkPhysicalDevice m_handle;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_PHYSICALDEVICE_HPP
