#if !defined(QUETZAL_VULKAN_DEBUGMESSENGER_HPP)
#define QUETZAL_VULKAN_DEBUGMESSENGER_HPP

#include "Instance.hpp"
#include "debug_callback.hpp"
#include "vulkan_util.hpp"
#include <vulkan/vulkan.hpp>

namespace quetzal::vulkan
{

    class DebugMessenger
    {
    public:

        DebugMessenger(const Instance& instance, bool validation) :
            m_instance(instance),
            m_validation(validation),
            m_debugMessenger()
        {
            if (m_validation)
            {
                VkDebugUtilsMessengerCreateInfoEXT ci = debug_messenger_create_info();
                VkResult result = CreateDebugUtilsMessengerEXT(&ci, nullptr);
                if (result != VK_SUCCESS)
                {
                    throw std::runtime_error(std::format("Error CreateDebugUtilsMessengerEXT {}", result));
                }
            }
        }

        ~DebugMessenger()
        {
            if (m_validation)
            {
                DestroyDebugUtilsMessengerEXT(nullptr);
            }
        }

    private:

        VkResult CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator)
        {
            auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
            if (func != nullptr)
            {
                return func(m_instance, pCreateInfo, pAllocator, &m_debugMessenger);
            }
            else
            {
                return VK_ERROR_EXTENSION_NOT_PRESENT;
            }
        }

        void DestroyDebugUtilsMessengerEXT(const VkAllocationCallbacks* pAllocator)
        {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func != nullptr)
            {
                func(m_instance, m_debugMessenger, pAllocator);
            }
        }

        const Instance& m_instance;
        bool m_validation;
        VkDebugUtilsMessengerEXT m_debugMessenger;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_DEBUGMESSENGER_HPP
