#if !defined(QUETZAL_VULKAN_ERROR_HPP)
#define QUETZAL_VULKAN_ERROR_HPP

#include <vulkan/vulkan.hpp>
#include <format>
#include <utility>

namespace quetzal::vulkan
{

    class VulkanError : public std::runtime_error
    {
    public:

        VulkanError(VkResult vk_result, const std::string& context = "Vulkan operation") :
            std::runtime_error(format_error_message(vk_result, context)),
            m_vk_result(vk_result),
            m_context(context)
        {
        }

        VkResult vk_result() const { return m_vk_result; }
        const std::string& context() const { return m_context; }

    private:

        static std::string format_error_message(VkResult vk_result, const std::string& context)
        {
            return std::format("{}: VkResult = {} ({})", context, std::to_underlying(vk_result), to_string(vk_result));
        }

        static std::string to_string(VkResult vk_result)
        {
            switch (vk_result)
            {
                case VK_SUCCESS:
                    return "VK_SUCCESS";
                case VK_NOT_READY:
                    return "VK_NOT_READY";
                case VK_TIMEOUT:
                    return "VK_TIMEOUT";
                case VK_EVENT_SET:
                    return "VK_EVENT_SET";
                case VK_EVENT_RESET:
                    return "VK_EVENT_RESET";
                case VK_INCOMPLETE:
                    return "VK_INCOMPLETE";
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                    return "VK_ERROR_OUT_OF_HOST_MEMORY";
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
                case VK_ERROR_INITIALIZATION_FAILED:
                    return "VK_ERROR_INITIALIZATION_FAILED";
                case VK_ERROR_DEVICE_LOST:
                    return "VK_ERROR_DEVICE_LOST";
                case VK_ERROR_MEMORY_MAP_FAILED:
                    return "VK_ERROR_MEMORY_MAP_FAILED";
                case VK_ERROR_LAYER_NOT_PRESENT:
                    return "VK_ERROR_LAYER_NOT_PRESENT";
                case VK_ERROR_EXTENSION_NOT_PRESENT:
                    return "VK_ERROR_EXTENSION_NOT_PRESENT";
                case VK_ERROR_FEATURE_NOT_PRESENT:
                    return "VK_ERROR_FEATURE_NOT_PRESENT";
                case VK_ERROR_INCOMPATIBLE_DRIVER:
                    return "VK_ERROR_INCOMPATIBLE_DRIVER";
                case VK_ERROR_TOO_MANY_OBJECTS:
                    return "VK_ERROR_TOO_MANY_OBJECTS";
                case VK_ERROR_FORMAT_NOT_SUPPORTED:
                    return "VK_ERROR_FORMAT_NOT_SUPPORTED";
                case VK_ERROR_FRAGMENTED_POOL:
                    return "VK_ERROR_FRAGMENTED_POOL";
                case VK_ERROR_UNKNOWN:
                    return "VK_ERROR_UNKNOWN";
                default:
                    return std::format("VkResult({})", std::to_underlying(vk_result));
            }
        }

        VkResult m_vk_result;
        std::string m_context;
    };

    inline void check_result(VkResult vk_result, const std::string& context = "Vulkan operation")
    {
        if (vk_result != VK_SUCCESS)
        {
            throw VulkanError(vk_result, context);
        }
    }

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_ERROR_HPP
