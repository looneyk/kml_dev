#if !defined(QUETZAL_VULKAN_INSTANCE_HPP)
#define QUETZAL_VULKAN_INSTANCE_HPP

#include "VulkanError.hpp"
#include "debug_callback.hpp"
#include "initializer.hpp"
#include "vulkan_util.hpp"
#include "quetzal/common/Config.hpp"
#include <vulkan/vulkan.hpp>
#include <format>
#include <vector>

namespace quetzal::vulkan
{

    class Instance
    {
    public:

        Instance(const Config& config, const std::vector<const char*>& extensions, const std::vector<const char*>& layers) :
            m_handle(VK_NULL_HANDLE)
        {
            bool validation = !layers.empty();

            if (validation && !check_validation_layer_support(layers))
            {
                throw std::runtime_error("validation layers requested, but not available!");
            }

            std::string applicationName = config["application_name"];
            std::string engineName = config["engine_name"];

            VkApplicationInfo application_info{};
            application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            application_info.pApplicationName = applicationName.c_str();
            application_info.applicationVersion = to_uint32(std::stoul(config["application_version"]));
            application_info.pEngineName = engineName.c_str();
            application_info.engineVersion = to_uint32(std::stoul(config["engine_version"]));
            application_info.apiVersion = VK_API_VERSION_1_4;

            VkInstanceCreateInfo ci = vk_instance_create_info(application_info, extensions, layers); // ...

            if (validation)
            {
                VkDebugUtilsMessengerCreateInfoEXT debug_ci = debug_messenger_create_info();
                ci.enabledLayerCount = to_uint32(layers.size());
                ci.ppEnabledLayerNames = layers.data();
                ci.pNext = &debug_ci;
            }

            VkResult vk_result = vkCreateInstance(&ci, nullptr, &m_handle);
            check_result(vk_result, "vkCreateInstance");
        }

        ~Instance()
        {
            assert(m_handle != VK_NULL_HANDLE);
            vkDestroyInstance(m_handle, nullptr);
        }

        operator VkInstance() const
        {
            assert(m_handle != VK_NULL_HANDLE);
            return m_handle;
        }

    private:

        class LayerProperties
        {
        public:

            LayerProperties() :
                m_properties()
            {
                uint32_t count = 0;
                vkEnumerateInstanceLayerProperties(&count, nullptr);

                m_properties.resize(count);
                vkEnumerateInstanceLayerProperties(&count, m_properties.data());
            };

            auto begin() const { return m_properties.begin(); }
            auto end() const { return m_properties.end(); }

        private:

            std::vector<VkLayerProperties> m_properties;
        };

        bool check_validation_layer_support(const std::vector<const char*>& layers)
        {
            LayerProperties layerProperties;

            for (auto layer : layers)
            {
                bool found = false;

                for (const auto& properties : layerProperties)
                {
                    if (strcmp(layer, properties.layerName) == 0)
                    {
                        found = true;
                        break;
                    }
                }

                if (!found)
                {
                    return false;
                }
            }

            return true;
        }

        VkInstance m_handle;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_INSTANCE_HPP
