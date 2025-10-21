#if !defined(QUETZAL_VULKAN_DESCRIPTORSETLAYOUT_HPP)
#define QUETZAL_VULKAN_DESCRIPTORSETLAYOUT_HPP

#include <vulkan/vulkan.hpp>
#include "Device.hpp"
#include "initializer.hpp"

namespace quetzal::vulkan
{

    class DescriptorSetLayout
    {
    public:

        DescriptorSetLayout(const Device& device) :
            m_device(device),
            m_handle(VK_NULL_HANDLE)
        {
        }

        operator VkDescriptorSetLayout() const
        {
            assert(m_handle != VK_NULL_HANDLE);
            return m_handle;
        }

        void create(std::span<VkDescriptorSetLayoutBinding> bindings)
        {
            VkDescriptorSetLayoutCreateInfo ci = vk_descriptor_set_layout_create_info(bindings);
            VkResult result = vkCreateDescriptorSetLayout(m_device, &ci, nullptr, &m_handle);
            if (result != VK_SUCCESS)
            {
                throw std::runtime_error(std::format("Error vkCreateDescriptorSetLayout {}", result));
            }
        }

        void destroy()
        {
            assert(m_handle != VK_NULL_HANDLE);
            vkDestroyDescriptorSetLayout(m_device, m_handle, nullptr);
        }

    private:

        const Device& m_device;
        VkDescriptorSetLayout m_handle;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_DESCRIPTORSETLAYOUT_HPP
