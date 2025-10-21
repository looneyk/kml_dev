#if !defined(QUETZAL_VULKAN_DESCRIPTORS_HPP)
#define QUETZAL_VULKAN_DESCRIPTORS_HPP

#include "Device.hpp"
#include <vulkan/vulkan.hpp>
#include <span>
#include <unordered_map>
#include <vector>

namespace quetzal::vulkan
{

    //--------------------------------------------------------------------------
    class DescriptorAllocator
    {
    public:

        // make this private and add set_pool_size_factor and reset_pool_sizes ...
        using pool_sizes_type = std::vector<std::tuple<VkDescriptorType, float>>;

        DescriptorAllocator(const Device& device);

        void clear();

        bool allocate(VkDescriptorSet* set, const VkDescriptorSetLayout& layout);
        void reset_pools();

        const Device& device() const;

    private:

        VkDescriptorPool grab_pool();

        pool_sizes_type m_pool_sizes =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f }
        };

        const Device& m_device;
        VkDescriptorPool m_pool{ VK_NULL_HANDLE };
        std::vector<VkDescriptorPool> m_pools_used;
        std::vector<VkDescriptorPool> m_pools_free;
    };

    //--------------------------------------------------------------------------
    class DescriptorLayoutCache
    {
    public:

        DescriptorLayoutCache(const Device& device);

        void clear();

        VkDescriptorSetLayout create_descriptor_layout(std::span<VkDescriptorSetLayoutBinding> bindings);

        struct DescriptorLayoutInfo
        {
            //good idea to turn this into a inlined array
            std::vector<VkDescriptorSetLayoutBinding> bindings;

            bool operator==(const DescriptorLayoutInfo& other) const;

            size_t hash() const;
        };

    private:

        struct DescriptorLayoutHash
        {
            std::size_t operator()(const DescriptorLayoutInfo& k) const
            {
                return k.hash();
            }
        };

        const Device& m_device;
        std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout, DescriptorLayoutHash> m_cache;
    };

    //--------------------------------------------------------------------------
    class DescriptorBuilder
    {
    public:

        static DescriptorBuilder begin(DescriptorAllocator* allocator, DescriptorLayoutCache* layout_cache);

        DescriptorBuilder& bind(uint32_t binding, const VkDescriptorBufferInfo& buffer_info, VkDescriptorType type, VkShaderStageFlags stage_flags);
        DescriptorBuilder& bind(uint32_t binding, const VkDescriptorImageInfo& image_info, VkDescriptorType type, VkShaderStageFlags stage_flags);

        bool build(VkDescriptorSet& set, VkDescriptorSetLayout& layout);
        bool build(VkDescriptorSet& set);

    private:

        std::vector<VkWriteDescriptorSet> m_writes;
        std::vector<VkDescriptorSetLayoutBinding> m_bindings;

        DescriptorAllocator* m_allocator;
        DescriptorLayoutCache* m_layout_cache;
    };

} // quetzal::vulkan

#endif // QUETZAL_VULKAN_DESCRIPTORS_HPP
