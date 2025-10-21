//------------------------------------------------------------------------------
// quetzal::vulkan
// initializer.hpp
//------------------------------------------------------------------------------
#if !defined(QUETZAL_VULKAN_INITIALIZER_HPP)
#define QUETZAL_VULKAN_INITIALIZER_HPP

#include <vulkan/vulkan.hpp>
//#include <vk_mem_alloc.h>
#include <span>

namespace quetzal::vulkan
{

    // CreateInfo
    VkBufferCreateInfo vk_buffer_create_info(VkDeviceSize size, VkBufferUsageFlags usage);
    VkCommandPoolCreateInfo vk_command_pool_create_info(uint32_t queue_family_index, VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VkDescriptorPoolCreateInfo vk_descriptor_pool_create_info(std::span<VkDescriptorPoolSize> pool_sizes, uint32_t max_sets, VkDescriptorPoolCreateFlags flags);
    VkDescriptorSetLayoutCreateInfo vk_descriptor_set_layout_create_info(std::span<VkDescriptorSetLayoutBinding> bindings);
    VkDeviceCreateInfo vk_device_create_info(std::span<VkDeviceQueueCreateInfo> queue_cis, const std::vector<const char*>& extensions, VkPhysicalDeviceFeatures* enabled_features); // vector ...
//    VkDeviceQueueCreateInfo vk_device_queue_create_info(); // used in arrays ...
    VkEventCreateInfo vk_event_create_info();
    VkFenceCreateInfo vk_fence_create_info(VkFenceCreateFlagBits flags = VK_FENCE_CREATE_SIGNALED_BIT);
    VkFramebufferCreateInfo vk_framebuffer_create_info(VkExtent2D extent, std::span<VkImageView> attachments, VkRenderPass render_pass);
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
    VkRenderPassCreateInfo vk_render_pass_create_info(std::span<VkAttachmentDescription> attachments, std::span<VkSubpassDescription> subpasses, std::span<VkSubpassDependency> dependencies);
    VkSamplerCreateInfo vk_sampler_create_info(VkPhysicalDeviceProperties properties);
    VkSemaphoreCreateInfo vk_semaphore_create_info();
    VkShaderModuleCreateInfo vk_shader_module_create_info(std::span<const char> code);
    VkSwapchainCreateInfoKHR vk_swapchain_create_info(VkSurfaceKHR surface, uint32_t image_count, VkExtent2D extent, VkSurfaceFormatKHR surface_format, VkImageUsageFlagBits image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

    // Other
    VkAttachmentDescription vk_attachment_description(VkFormat format, VkSampleCountFlagBits samples, VkAttachmentLoadOp load_op, VkAttachmentStoreOp store_op, VkAttachmentLoadOp stencil_load_op, VkAttachmentStoreOp stencil_store_op, VkImageLayout initial_layout, VkImageLayout final_layout, VkAttachmentDescriptionFlags flags = 0);
    VkCommandBufferAllocateInfo vk_command_buffer_allocate_info(VkCommandPool command_pool, VkCommandBufferLevel level, size_t command_buffer_count);
    VkCommandBufferBeginInfo vk_command_buffer_begin_info(VkCommandBufferUsageFlags flags);
    VkDescriptorSetLayoutBinding vk_descriptor_set_layout_binding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t descriptorCount = 1);
    VkPipelineColorBlendAttachmentState vk_pipeline_color_blend_attachment_state(VkColorComponentFlags color_write_mask, bool blend_enable);
    VkPresentInfoKHR vk_present_info(std::span<const VkSemaphore> semaphores, std::span<const VkSwapchainKHR> swapchains, std::span<uint32_t> image_indices);
    VkPushConstantRange vk_push_constant_range(VkShaderStageFlags stage_flags, uint32_t offset, uint32_t size);
    VkRenderPassBeginInfo vk_render_pass_begin_info(VkRenderPass render_pass, VkFramebuffer framebuffer, VkExtent2D extent, std::span<VkClearValue> clear_values);
    VkStencilOpState vk_stencil_op_state(VkCompareOp compare_op = VK_COMPARE_OP_ALWAYS);
    VkSubmitInfo vk_submit_info(std::span<const VkSemaphore> wait_semaphores, std::span<const VkPipelineStageFlags> wait_stage_flags, std::span<const VkCommandBuffer> command_buffers, std::span<const VkSemaphore> signal_semaphores);
    VkSubmitInfo vk_submit_info(std::span<const VkCommandBuffer> command_buffers);
    VkSubpassDependency vk_subpass_dependency(uint32_t source_subpass, uint32_t dest_subpass, VkPipelineStageFlags source_stage_mask, VkPipelineStageFlags dest_stage_mask, VkAccessFlags source_access_mask, VkAccessFlags dest_access_mask, VkDependencyFlags flags = 0);
    VkSubpassDescription vk_subpass_description(VkPipelineBindPoint pipeline_bind_point, std::span<VkAttachmentReference> input_attachments, std::span<VkAttachmentReference> color_attachments,
            const VkAttachmentReference& resolve_attachment, const VkAttachmentReference& depth_stencil_attachment, std::span<uint32_t> preserve_attachments);

    //VkPipelineTessellationStateCreateInfo pipeline_tessellation_state_create_info
    //VkGraphicsPipelineCreateInfo graphics_pipeline_create_info
    //VkComputePipelineCreateInfo compute_pipeline_create_info

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_INITIALIZER_HPP
