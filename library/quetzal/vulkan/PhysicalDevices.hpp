#if !defined(QUETZAL_VULKAN_PHYSICALDEVICES_HPP)
#define QUETZAL_VULKAN_PHYSICALDEVICES_HPP

#include <vulkan/vulkan.hpp>
#include "Instance.hpp"

namespace quetzal::vulkan
{

    class PhysicalDevices
    {
    public:

        PhysicalDevices(const Instance& instance) :
            m_devices()
        {
            uint32_t count = 0;
            vkEnumeratePhysicalDevices(instance, &count, nullptr);
            if (count == 0)
            {
                throw std::runtime_error("Error vkEnumeratePhysicalDevices");
            }

            m_devices.resize(count);
            vkEnumeratePhysicalDevices(instance, &count, m_devices.data());
        };

        ~PhysicalDevices() = default;

        auto begin() const { return m_devices.begin(); }
        auto end() const { return m_devices.end(); }

    private:

        std::vector<VkPhysicalDevice> m_devices;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_PHYSICALDEVICES_HPP
