#if !defined(QUETZAL_VULKAN_SHADERMODULE_HPP)
#define QUETZAL_VULKAN_SHADERMODULE_HPP

#include "vulkan_util.hpp"
#include <vulkan/vulkan.hpp>
#include <fstream>

namespace quetzal::vulkan
{

    class ShaderModule
    {
    public:

        ShaderModule(VkDevice device, const std::string& filepath) :
            m_device(device),
            m_handle(create(read(filepath)))
        {
        }

        ~ShaderModule()
        {
            assert(m_handle != VK_NULL_HANDLE);
            vkDestroyShaderModule(m_device, m_handle, nullptr);
        }

        operator VkShaderModule() const
        {
            assert(m_handle != VK_NULL_HANDLE);
            return m_handle;
        }

        static std::vector<char> read(const std::string& filepath)
        {
            std::ifstream ifs(filepath, std::ios::ate | std::ios_base::binary);
            if (!ifs.is_open())
            {
                throw std::runtime_error("Error opening shader file");
            }

            size_t size = (size_t)ifs.tellg();
            std::vector<char> buffer(size);

            ifs.seekg(0);
            ifs.read(buffer.data(), size);
            ifs.close();

            return buffer;
        }

        VkShaderModule create(const std::vector<char>& code)
        {
            VkShaderModuleCreateInfo ci = vk_shader_module_create_info(code);
            VkShaderModule handle;
            VkResult result = vkCreateShaderModule(m_device, &ci, nullptr, &handle);
            if (result != VK_SUCCESS)
            {
                throw std::runtime_error(std::format("Error vkCreateShaderModule {}", result));
            }

            return handle;
        }

    private:

        VkDevice m_device;
        VkShaderModule m_handle;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_SHADERMODULE_HPP
