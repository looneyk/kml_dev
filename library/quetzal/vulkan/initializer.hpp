//------------------------------------------------------------------------------
// quetzal::vulkan
// initializer.hpp
//------------------------------------------------------------------------------
#if !defined(QUETZAL_VULKAN_INITIALIZER_HPP)
#define QUETZAL_VULKAN_INITIALIZER_HPP

#include <vulkan/vulkan.hpp>
//#include <vk_mem_alloc.h>
#include <functional>
#include <optional>
#include <span>

namespace quetzal::vulkan
{

    // CreateInfo
    VkBufferCreateInfo vk_buffer_create_info(VkDeviceSize size, VkBufferUsageFlags usage);
    VkCommandPoolCreateInfo vk_command_pool_create_info(uint32_t queue_family_index, VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VkDescriptorPoolCreateInfo vk_descriptor_pool_create_info(std::span<VkDescriptorPoolSize> pool_sizes, uint32_t max_sets, VkDescriptorPoolCreateFlags flags);
    VkDescriptorSetLayoutCreateInfo vk_descriptor_set_layout_create_info(std::span<VkDescriptorSetLayoutBinding> bindings);
    VkDeviceCreateInfo vk_device_create_info(std::span<VkDeviceQueueCreateInfo> queue_cis, const std::vector<const char*>& extensions, void* pNext = nullptr);
//    VkDeviceQueueCreateInfo vk_device_queue_create_info(); // used in arrays ...
    VkEventCreateInfo vk_event_create_info();
    VkFenceCreateInfo vk_fence_create_info(VkFenceCreateFlagBits flags = VK_FENCE_CREATE_SIGNALED_BIT);
    VkImageCreateInfo vk_image_create_info(VkExtent2D extent, VkFormat format, VkImageUsageFlags usage, VkSampleCountFlagBits sampleCount, uint32_t mipLevels, VkImageTiling tiling);
    VkImageViewCreateInfo vk_image_view_create_info(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
    VkInstanceCreateInfo vk_instance_create_info(const VkApplicationInfo& application_info, const std::vector<const char*>& extensions, const std::vector<const char*>& layers); // vector ...
    VkPipelineCacheCreateInfo vk_pipeline_cache_create_info(std::span<uint8_t> buffer = {});
    VkPipelineColorBlendStateCreateInfo vk_pipeline_color_blend_state_create_info(std::span<VkPipelineColorBlendAttachmentState> color_blend_attachment_states);
    VkPipelineDepthStencilStateCreateInfo vk_pipeline_depth_stencil_state_create_info(bool depth_test_enable = VK_TRUE, bool depth_write_enable = VK_TRUE, VkCompareOp depth_compare_op = VK_COMPARE_OP_LESS);
    VkPipelineDynamicStateCreateInfo vk_pipeline_dynamic_state_create_info(std::span<VkDynamicState> dynamic_states);
    VkPipelineInputAssemblyStateCreateInfo vk_pipeline_input_assembly_state_create_info(VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, bool primitive_restart_enable = VK_FALSE);
    VkPipelineLayoutCreateInfo vk_pipeline_layout_create_info(std::span<VkDescriptorSetLayout> set_layouts, std::span<VkPushConstantRange> push_constant_ranges);
    VkPipelineMultisampleStateCreateInfo vk_pipeline_multisample_state_create_info(VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT);
    VkPipelineRasterizationStateCreateInfo vk_pipeline_rasterization_state_create_info(VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL, VkCullModeFlags cull_mode = VK_CULL_MODE_BACK_BIT, VkFrontFace front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE);
    VkPipelineShaderStageCreateInfo vk_pipeline_shader_stage_create_info(const VkShaderModule& shader_module, VkShaderStageFlagBits stage, const char* name = "main", const VkSpecializationInfo* p_specialization_info = nullptr);
    VkPipelineVertexInputStateCreateInfo vk_pipeline_vertex_input_state_create_info(std::span<VkVertexInputBindingDescription> binding_descriptions, std::span<const VkVertexInputAttributeDescription> attribute_descriptions);
    VkPipelineViewportStateCreateInfo vk_pipeline_viewport_state_create_info();
    VkSamplerCreateInfo vk_sampler_create_info(VkPhysicalDeviceProperties properties);
    VkSemaphoreCreateInfo vk_semaphore_create_info();
    VkSemaphoreTypeCreateInfo vk_semaphore_type_create_info(VkSemaphoreType semaphore_type, uint64_t initial_value = 0);
    VkShaderModuleCreateInfo vk_shader_module_create_info(std::span<const char> code);
    VkSwapchainCreateInfoKHR vk_swapchain_create_info(VkSurfaceKHR surface, uint32_t image_count, VkExtent2D extent, VkSurfaceFormatKHR surface_format, VkImageUsageFlagBits image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

    // Other
    VkCommandBufferAllocateInfo vk_command_buffer_allocate_info(VkCommandPool command_pool, VkCommandBufferLevel level, size_t command_buffer_count);
    VkDependencyInfo vk_dependency_info(std::span<const VkImageMemoryBarrier2> image_barriers);
    VkImageMemoryBarrier2 vk_image_memory_barrier2(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags2 src_stage_mask, VkAccessFlags2 src_access_mask, VkPipelineStageFlags2 dst_stage_mask, VkAccessFlags2 dst_access_mask, VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT);
    VkCommandBufferBeginInfo vk_command_buffer_begin_info(VkCommandBufferUsageFlags flags);
    VkCommandBufferSubmitInfo vk_command_buffer_submit_info(VkCommandBuffer command_buffer, uint32_t device_mask = 0);
    VkDescriptorSetLayoutBinding vk_descriptor_set_layout_binding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t descriptorCount = 1);
    VkPhysicalDeviceDynamicRenderingFeatures vk_physical_device_dynamic_rendering_features(bool dynamic_rendering = VK_TRUE);
    VkPhysicalDeviceFeatures2 vk_physical_device_features2(void* pNext = nullptr);
    VkPhysicalDeviceSynchronization2Features vk_physical_device_synchronization2_features(bool synchronization2 = VK_TRUE, void* pNext = nullptr);
    VkPipelineRenderingCreateInfo vk_pipeline_rendering_create_info(std::span<const VkFormat> color_formats, VkFormat depth_format = VK_FORMAT_UNDEFINED, VkFormat stencil_format = VK_FORMAT_UNDEFINED);
    VkPipelineColorBlendAttachmentState vk_pipeline_color_blend_attachment_state(VkColorComponentFlags color_write_mask, bool blend_enable);
    VkPresentInfoKHR vk_present_info(std::span<const VkSemaphore> semaphores, std::span<const VkSwapchainKHR> swapchains, std::span<uint32_t> image_indices);
    VkPushConstantRange vk_push_constant_range(VkShaderStageFlags stage_flags, uint32_t offset, uint32_t size);
    VkRenderingAttachmentInfo vk_rendering_attachment_info_color(VkImageView image_view, VkImageLayout image_layout, VkResolveModeFlagBits resolve_mode, VkImageView resolve_image_view, VkImageLayout resolve_image_layout, const VkClearValue& clear_value);
    VkRenderingAttachmentInfo vk_rendering_attachment_info_depth(VkImageView image_view, VkImageLayout image_layout, VkResolveModeFlagBits resolve_mode, VkImageView resolve_image_view, VkImageLayout resolve_image_layout, const VkClearValue& clear_value);
// image_layout and resolve_mode here are stencil specific ...
    VkRenderingAttachmentInfo vk_rendering_attachment_info_stencil(VkImageView image_view, VkImageLayout image_layout, VkResolveModeFlagBits resolve_mode, VkImageView resolve_image_view, VkImageLayout resolve_image_layout, const VkClearValue& clear_value);
    VkRenderingInfo vk_rendering_info(VkRect2D render_area, uint32_t layer_count, uint32_t view_mask, std::span<const VkRenderingAttachmentInfo> color_attachments, std::optional<std::reference_wrapper<const VkRenderingAttachmentInfo>> depth_attachment = std::nullopt, std::optional<std::reference_wrapper<const VkRenderingAttachmentInfo>> stencil_attachment = std::nullopt);
    VkStencilOpState vk_stencil_op_state(VkCompareOp compare_op = VK_COMPARE_OP_ALWAYS);
    VkSemaphoreSubmitInfo vk_semaphore_submit_info(VkSemaphore semaphore, VkPipelineStageFlags2 stage_mask, uint64_t value = 0);
    VkSubmitInfo vk_submit_info(std::span<const VkSemaphore> wait_semaphores, std::span<const VkPipelineStageFlags> wait_stage_flags, std::span<const VkCommandBuffer> command_buffers, std::span<const VkSemaphore> signal_semaphores);
    VkSubmitInfo vk_submit_info(std::span<const VkCommandBuffer> command_buffers);
    VkSubmitInfo2 vk_submit_info2(std::span<const VkSemaphoreSubmitInfo> wait_semaphores, std::span<const VkCommandBufferSubmitInfo> command_buffers, std::span<const VkSemaphoreSubmitInfo> signal_semaphores);

    //VkPipelineTessellationStateCreateInfo pipeline_tessellation_state_create_info
    //VkGraphicsPipelineCreateInfo graphics_pipeline_create_info
    //VkComputePipelineCreateInfo compute_pipeline_create_info

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_INITIALIZER_HPP
