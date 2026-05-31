//------------------------------------------------------------------------------
// quetzal::vulkan
// ResourceManager.cpp
//------------------------------------------------------------------------------

#include "ResourceManager.hpp"

//------------------------------------------------------------------------------
quetzal::vulkan::ResourceManager::ResourceManager(const Device& device) :
    m_device(device),
    m_precorder(nullptr),
    m_pipeline_layouts(device)
{
}

//------------------------------------------------------------------------------
quetzal::vulkan::PipelineLayout& quetzal::vulkan::ResourceManager::request_pipeline_layout(std::span<VkDescriptorSetLayout> set_layouts, std::span<VkPushConstantRange> push_constant_ranges)
{
    return m_pipeline_layouts.request(set_layouts, push_constant_ranges);
}

//------------------------------------------------------------------------------
void quetzal::vulkan::ResourceManager::clear()
{
    m_pipeline_layouts.clear();
    return;
}

//------------------------------------------------------------------------------
std::vector<uint8_t> quetzal::vulkan::ResourceManager::serialize()
{
//    return m_recorder.data();
    return {};
}

//------------------------------------------------------------------------------
void quetzal::vulkan::ResourceManager::load(std::span<uint8_t> data)
{
    data;
//    m_recorder.data(data);
//    m_replayer.play(*this, m_recorder);
    return;
}
