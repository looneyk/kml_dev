//------------------------------------------------------------------------------
// quetzal::vulkan
// PipelineLayout.cpp
//------------------------------------------------------------------------------

#include "PipelineLayout.hpp"
#include "VulkanError.hpp"
#include "initializer.hpp"

//------------------------------------------------------------------------------
quetzal::vulkan::PipelineLayout::PipelineLayout(const Device& device) :
    m_device(device),
    m_handle(VK_NULL_HANDLE)
{
}

//------------------------------------------------------------------------------
quetzal::vulkan::PipelineLayout::PipelineLayout(PipelineLayout&& other) :
    m_device(other.m_device),
    m_handle(other.m_handle)
{
    other.m_handle = VK_NULL_HANDLE;
}

//------------------------------------------------------------------------------
quetzal::vulkan::PipelineLayout::~PipelineLayout()
{
    if (m_handle != VK_NULL_HANDLE)
    {
        destroy();
    }
}

//------------------------------------------------------------------------------
quetzal::vulkan::PipelineLayout::operator VkPipelineLayout() const
{
    assert(m_handle != VK_NULL_HANDLE);
    return m_handle;
}

//------------------------------------------------------------------------------
VkPipelineLayout quetzal::vulkan::PipelineLayout::handle() const
{
    assert(m_handle != VK_NULL_HANDLE);
    return m_handle;
}

//------------------------------------------------------------------------------
void quetzal::vulkan::PipelineLayout::create(std::span<VkDescriptorSetLayout> set_layouts, std::span<VkPushConstantRange> push_constant_ranges)
{
    VkPipelineLayoutCreateInfo ci = vk_pipeline_layout_create_info(set_layouts, push_constant_ranges);
    VkResult vk_result = vkCreatePipelineLayout(m_device, &ci, nullptr, &m_handle);
    check_result(vk_result, "vkCreatePipelineLayout");
    return;
}

//------------------------------------------------------------------------------
void quetzal::vulkan::PipelineLayout::destroy()
{
    assert(m_handle != VK_NULL_HANDLE);
    vkDestroyPipelineLayout(m_device, m_handle, nullptr);
    m_handle = VK_NULL_HANDLE;
    return;
}
