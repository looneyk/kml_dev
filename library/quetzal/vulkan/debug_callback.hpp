#if !defined(QUETZAL_VULKAN_DEBUG_CALLBACK_HPP)
#define QUETZAL_VULKAN_DEBUG_CALLBACK_HPP

#include <vulkan/vulkan.hpp>

namespace quetzal::vulkan
{

    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info();

    VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_DEBUG_CALLBACK_HPP
