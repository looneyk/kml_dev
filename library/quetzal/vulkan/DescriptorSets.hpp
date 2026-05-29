#if !defined(QUETZAL_VULKAN_DESCRIPTORSETS_HPP)
#define QUETZAL_VULKAN_DESCRIPTORSETS_HPP

#include "DescriptorPool.hpp"
#include "DescriptorSetLayout.hpp"
#include "Device.hpp"
#include "VulkanError.hpp"
#include "initializer.hpp"
#include <vulkan/vulkan.hpp>

namespace quetzal::vulkan
{

    class DescriptorSets
    {
    public:

        DescriptorSets(const Device& device, const DescriptorPool& descriptorPool) :
            m_device(device),
            m_descriptorPool(descriptorPool),
            m_handles()
        {
        }

        VkDescriptorSet operator[](size_t i) const
        {
            assert(i < m_handles.size());
            assert(m_handles[i] != VK_NULL_HANDLE);
            return m_handles[i];
        }

        void create(std::span<VkDescriptorSetLayout> layouts)
        {
            VkDescriptorSetAllocateInfo allocateInfo{};
            allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocateInfo.descriptorPool = m_descriptorPool;
            allocateInfo.descriptorSetCount = to_uint32(layouts.size());
            allocateInfo.pSetLayouts = layouts.data();

            m_handles.resize(layouts.size());
            VkResult vk_result = vkAllocateDescriptorSets(m_device, &allocateInfo, m_handles.data());
            check_result(vk_result, "vkAllocateDescriptorSets");
            return;
        }

        /* device operation ...
        void update(std::span<VkWriteDescriptorSet> writes, std::span<VkCopyDescriptorSet> copies = {})
        {
            vkUpdateDescriptorSets(m_device, to_uint32(writes.size()), writes.data(), to_uint32(copies.size()), copies.data());
        }
        */

        size_t size() const
        {
            return m_handles.size();
        }

        const VkDescriptorSet* data() const
        {
            return m_handles.data();
        }

    private:

        const Device& m_device;
        const DescriptorPool& m_descriptorPool;
        std::vector<VkDescriptorSet> m_handles;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_DESCRIPTORSETS_HPP
