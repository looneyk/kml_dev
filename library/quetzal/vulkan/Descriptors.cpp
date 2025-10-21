
#include "DescriptorSetLayout.hpp"
#include "Descriptors.hpp"
#include "initializer.hpp"
#include "vulkan_util.hpp"
#include <algorithm>
#include <functional>
#include <span>

using namespace std;
using namespace quetzal::vulkan;

namespace
{

    //--------------------------------------------------------------------------
    bool operator==(const VkDescriptorSetLayoutBinding& lhs, const VkDescriptorSetLayoutBinding& rhs)
    {
        return lhs.binding == rhs.binding
            && lhs.descriptorType == rhs.descriptorType
            && lhs.descriptorCount == rhs.descriptorCount
            && lhs.stageFlags == rhs.stageFlags;
    }

    //--------------------------------------------------------------------------
    VkDescriptorPool create_pool(VkDevice device, const DescriptorAllocator::pool_sizes_type& pool_sizes, uint32_t count, VkDescriptorPoolCreateFlags flags)
    {
        std::vector<VkDescriptorPoolSize> sizes;
        sizes.reserve(pool_sizes.size());
        for (auto ps : pool_sizes)
        {
            sizes.emplace_back(get<0>(ps), uint32_t(get<1>(ps) * count));
        }

        VkDescriptorPoolCreateInfo ci = vk_descriptor_pool_create_info(sizes, count, flags);
        VkDescriptorPool descriptor_pool;
        vkCreateDescriptorPool(device, &ci, nullptr, &descriptor_pool);

        return descriptor_pool;
    }

} // namespace

//------------------------------------------------------------------------------
DescriptorAllocator::DescriptorAllocator(const Device& device) :
    m_device(device),
    m_pool(VK_NULL_HANDLE),
    m_pools_used(),
    m_pools_free()
{
}

//------------------------------------------------------------------------------
void DescriptorAllocator::clear()
{
    for (auto pool : m_pools_free)
    {
        vkDestroyDescriptorPool(m_device, pool, nullptr);
    }
    m_pools_free.clear();

    for (auto pool : m_pools_used)
    {
        vkDestroyDescriptorPool(m_device, pool, nullptr);
    }
    m_pools_used.clear();
}

//------------------------------------------------------------------------------
bool DescriptorAllocator::allocate(VkDescriptorSet* set, const VkDescriptorSetLayout& layout)
{
    if (m_pool == VK_NULL_HANDLE)
    {
        m_pool = grab_pool();
        m_pools_used.push_back(m_pool);
    }

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.pSetLayouts = &layout;
    allocInfo.descriptorPool = m_pool;
    allocInfo.descriptorSetCount = 1;

    VkResult result = vkAllocateDescriptorSets(m_device, &allocInfo, set);
    if (result == VK_SUCCESS)
    {
        return true;
    }

    if (result == VK_ERROR_FRAGMENTED_POOL || result == VK_ERROR_OUT_OF_POOL_MEMORY)
    {
        //allocate a new pool and retry
        m_pool = grab_pool();
        m_pools_used.push_back(m_pool);

        result = vkAllocateDescriptorSets(m_device, &allocInfo, set);

        //if it still fails then we have big issues
        if (result == VK_SUCCESS)
        {
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
void DescriptorAllocator::reset_pools()
{
    for (auto pool : m_pools_used)
    {
        vkResetDescriptorPool(m_device, pool, 0);
    }

    m_pools_free = m_pools_used;
    m_pools_used.clear();
    m_pool = VK_NULL_HANDLE;
}

//------------------------------------------------------------------------------
const Device& DescriptorAllocator::device() const
{
    return m_device;
}

//------------------------------------------------------------------------------
VkDescriptorPool DescriptorAllocator::grab_pool()
{
    if (m_pools_free.size() > 0)
    {
        VkDescriptorPool pool = m_pools_free.back();
        m_pools_free.pop_back();
        return pool;
    }

    return create_pool(m_device, m_pool_sizes, 1000, 0);
}

//------------------------------------------------------------------------------
DescriptorLayoutCache::DescriptorLayoutCache(const Device& device) :
    m_device(device),
    m_cache()
{
}

//------------------------------------------------------------------------------
void DescriptorLayoutCache::clear()
{
    for (auto layout : m_cache)
    {
        vkDestroyDescriptorSetLayout(m_device, layout.second, nullptr);
    }
    m_cache.clear();
}

//------------------------------------------------------------------------------
VkDescriptorSetLayout DescriptorLayoutCache::create_descriptor_layout(std::span<VkDescriptorSetLayoutBinding> bindings)
{
    DescriptorLayoutInfo layoutinfo;
    layoutinfo.bindings.reserve(bindings.size());
    bool sorted = true;
    int32_t lastBinding = -1;

    for (uint32_t i = 0; i < bindings.size(); i++)
    {
        layoutinfo.bindings.push_back(bindings[i]);

        //check that the bindings are in strict increasing order
        if (static_cast<int32_t>(bindings[i].binding) > lastBinding)
        {
            lastBinding = bindings[i].binding;
        }
        else
        {
            sorted = false;
        }
    }

    if (!sorted)
    {
        std::sort(layoutinfo.bindings.begin(), layoutinfo.bindings.end(),
                [](const VkDescriptorSetLayoutBinding& lhs, const VkDescriptorSetLayoutBinding& rhs)
                {
                    return lhs.binding < rhs.binding;
                }
        );
    }

    auto it = m_cache.find(layoutinfo);
    if (it != m_cache.end())
    {
        return (*it).second;
    }
    else
    {
        DescriptorSetLayout layout(m_device);
        layout.create(layoutinfo.bindings);

        //m_cache.emplace()
        //add to cache
        m_cache[layoutinfo] = layout;
        return layout;
    }
}

//------------------------------------------------------------------------------
DescriptorBuilder DescriptorBuilder::begin(DescriptorAllocator* allocator, DescriptorLayoutCache* layout_cache)
{
    DescriptorBuilder builder;

    builder.m_allocator = allocator;
    builder.m_layout_cache = layout_cache;
    return builder;
}

//------------------------------------------------------------------------------
DescriptorBuilder& DescriptorBuilder::bind(uint32_t binding, const VkDescriptorBufferInfo& buffer_info, VkDescriptorType type, VkShaderStageFlags stage_flags)
{
    VkDescriptorSetLayoutBinding layoutBinding = vk_descriptor_set_layout_binding(binding, type, stage_flags);
    m_bindings.push_back(layoutBinding);

    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstBinding = binding;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pBufferInfo = &buffer_info;

    m_writes.push_back(write);
    return *this;
}

//------------------------------------------------------------------------------
DescriptorBuilder& DescriptorBuilder::bind(uint32_t binding, const VkDescriptorImageInfo& image_info, VkDescriptorType type, VkShaderStageFlags stage_flags)
{
    VkDescriptorSetLayoutBinding layoutBinding = vk_descriptor_set_layout_binding(binding, type, stage_flags);
    m_bindings.push_back(layoutBinding);

    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstBinding = binding;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = &image_info;

    m_writes.push_back(write);
    return *this;
}

//------------------------------------------------------------------------------
bool DescriptorBuilder::build(VkDescriptorSet& set, VkDescriptorSetLayout& layout)
{
    layout = m_layout_cache->create_descriptor_layout(m_bindings);

    bool success = m_allocator->allocate(&set, layout);
    if (!success)
    {
        return false;
    }

    for (VkWriteDescriptorSet& write : m_writes)
    {
        write.dstSet = set;
    }

    vkUpdateDescriptorSets(m_allocator->device(), to_uint32(m_writes.size()), m_writes.data(), 0, nullptr);
    return true;
}

//------------------------------------------------------------------------------
bool DescriptorBuilder::build(VkDescriptorSet& set)
{
    VkDescriptorSetLayout layout;
    return build(set, layout);
}

//------------------------------------------------------------------------------
bool DescriptorLayoutCache::DescriptorLayoutInfo::operator==(const DescriptorLayoutInfo& other) const
{
    if (other.bindings.size() != bindings.size())
    {
        return false;
    }

    //compare each of the bindings is the same. Bindings are sorted so they will match
    for (int i = 0; i < bindings.size(); i++)
    {
        if (other.bindings[i] != bindings[i])
        {
            return false;
        }
    }

    return true;
}

//------------------------------------------------------------------------------
size_t DescriptorLayoutCache::DescriptorLayoutInfo::hash() const
{
    using std::size_t;

    size_t result = std::hash<size_t>()(bindings.size());

    for (const VkDescriptorSetLayoutBinding& b : bindings)
    {
        //pack the binding data into a single int64. Not fully correct but its ok
        size_t binding_hash = b.binding | b.descriptorType << 8 | b.descriptorCount << 16 | b.stageFlags << 24;

        //shuffle the packed binding data and xor it with the main hash
        result ^= std::hash<size_t>()(binding_hash);
    }

    return result;
}
