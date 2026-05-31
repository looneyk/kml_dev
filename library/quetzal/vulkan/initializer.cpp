//------------------------------------------------------------------------------
// quetzal::vulkan
// initializer.cpp
//------------------------------------------------------------------------------

#include "initializer.hpp"
#include "vulkan_util.hpp"
#include <cassert>

//------------------------------------------------------------------------------
VkBufferCreateInfo quetzal::vulkan::vk_buffer_create_info(VkDeviceSize size, VkBufferUsageFlags usage)
{
    VkBufferCreateInfo ci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    ci.size = size;
    ci.usage = usage;
    //ci.sharingMode
    //ci.queueFamilyIndexCount
    //ci.pQueueFamilyIndices
    return ci;
}

//------------------------------------------------------------------------------
VkCommandPoolCreateInfo quetzal::vulkan::vk_command_pool_create_info(uint32_t queue_family_index, VkCommandPoolCreateFlags flags)
{
    VkCommandPoolCreateInfo ci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    ci.flags = flags;
    ci.queueFamilyIndex = queue_family_index;
    return ci;
}

//------------------------------------------------------------------------------
VkDescriptorPoolCreateInfo quetzal::vulkan::vk_descriptor_pool_create_info(std::span<VkDescriptorPoolSize> pool_sizes, uint32_t max_sets, VkDescriptorPoolCreateFlags flags)
{
    VkDescriptorPoolCreateInfo ci{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    ci.flags = flags;
    ci.maxSets = max_sets;
    ci.poolSizeCount = to_uint32(pool_sizes.size());
    ci.pPoolSizes = pool_sizes.data();
    return ci;
}

//------------------------------------------------------------------------------
VkDescriptorSetLayoutCreateInfo quetzal::vulkan::vk_descriptor_set_layout_create_info(std::span<VkDescriptorSetLayoutBinding> bindings)
{
    VkDescriptorSetLayoutCreateInfo ci{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    ci.bindingCount = to_uint32(bindings.size());
    ci.pBindings = bindings.data();
    return ci;
}

//------------------------------------------------------------------------------
VkDeviceCreateInfo quetzal::vulkan::vk_device_create_info(std::span<VkDeviceQueueCreateInfo> queue_cis, const std::vector<const char*>& extensions, VkPhysicalDeviceFeatures* enabled_features)
{
    VkDeviceCreateInfo ci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    ci.queueCreateInfoCount = to_uint32(queue_cis.size());
    ci.pQueueCreateInfos = queue_cis.data();
    //ci.enabledLayerCount
    //ci.ppEnabledLayerNames
    ci.enabledExtensionCount = to_uint32(extensions.size());
    ci.ppEnabledExtensionNames = extensions.data();
    ci.pEnabledFeatures = enabled_features;
    return ci;
}

//------------------------------------------------------------------------------
/*
VkDeviceQueueCreateInfo quetzal::vulkan::vk_device_queue_create_info()
{
    VkDeviceQueueCreateInfo ci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    return ci;
}
*/

//------------------------------------------------------------------------------
VkEventCreateInfo quetzal::vulkan::vk_event_create_info()
{
    VkEventCreateInfo ci{VK_STRUCTURE_TYPE_EVENT_CREATE_INFO};
    return ci;
}

//------------------------------------------------------------------------------
VkFenceCreateInfo quetzal::vulkan::vk_fence_create_info(VkFenceCreateFlagBits flags)
{
    VkFenceCreateInfo ci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    ci.flags = flags;
    return ci;
}

//------------------------------------------------------------------------------
VkFenceCreateInfo quetzal::vulkan::vk_fence_create_info_v2(bool timeline, uint64_t initial_value)
{
    VkFenceCreateInfo ci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    return ci;
}

//------------------------------------------------------------------------------
VkFramebufferCreateInfo quetzal::vulkan::vk_framebuffer_create_info(VkExtent2D extent, std::span<VkImageView> attachments, VkRenderPass render_pass)
{
    VkFramebufferCreateInfo ci{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    ci.renderPass = render_pass;
    ci.attachmentCount = to_uint32(attachments.size());
    ci.pAttachments = attachments.data();
    ci.width = extent.width;
    ci.height = extent.height;
    ci.layers = 1;
    return ci;
}

//------------------------------------------------------------------------------
VkImageCreateInfo quetzal::vulkan::vk_image_create_info(VkExtent2D extent, VkFormat format, VkImageUsageFlags usage, VkSampleCountFlagBits sampleCount, uint32_t mipLevels, VkImageTiling tiling)
{
    VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ci.imageType = VK_IMAGE_TYPE_2D;
    ci.format = format;
    ci.extent = {extent.width, extent.height, 1};
    ci.mipLevels = mipLevels;
    ci.arrayLayers = 1; // ...
    ci.samples = sampleCount;
    ci.tiling = tiling;
    ci.usage = usage;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // ...
    ci.queueFamilyIndexCount = 0;
    ci.pQueueFamilyIndices = nullptr;
    ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    return ci;
}

//------------------------------------------------------------------------------
VkImageViewCreateInfo quetzal::vulkan::vk_image_view_create_info(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
    VkImageViewCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    ci.image = image;
    ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ci.format = format;
    ci.subresourceRange.aspectMask = aspectFlags;
    ci.subresourceRange.baseMipLevel = 0;
    ci.subresourceRange.levelCount = mipLevels;
    ci.subresourceRange.baseArrayLayer = 0;
    ci.subresourceRange.layerCount = 1;
    return ci;
}

//------------------------------------------------------------------------------
VkInstanceCreateInfo quetzal::vulkan::vk_instance_create_info(const VkApplicationInfo& application_info, const std::vector<const char*>& extensions, const std::vector<const char*>& layers)
{
    VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ci.pApplicationInfo = &application_info;
    ci.enabledLayerCount = to_uint32(layers.size());
    ci.ppEnabledLayerNames = layers.data();
    ci.enabledExtensionCount = to_uint32(extensions.size());
    ci.ppEnabledExtensionNames = extensions.data();
    return ci;
}

//------------------------------------------------------------------------------
VkPipelineCacheCreateInfo quetzal::vulkan::vk_pipeline_cache_create_info(std::span<uint8_t> buffer)
{
    VkPipelineCacheCreateInfo ci{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
    ci.initialDataSize = buffer.size();
    ci.pInitialData = buffer.data();
    return ci;
}

//------------------------------------------------------------------------------
VkPipelineColorBlendStateCreateInfo quetzal::vulkan::vk_pipeline_color_blend_state_create_info(std::span<VkPipelineColorBlendAttachmentState> color_blend_attachment_states)
{
    VkPipelineColorBlendStateCreateInfo ci{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    ci.logicOpEnable = VK_FALSE;
    ci.logicOp = VK_LOGIC_OP_COPY;
    ci.attachmentCount = to_uint32(color_blend_attachment_states.size());
    ci.pAttachments = color_blend_attachment_states.data();
    //ci.blendConstants
    return ci;
}

//------------------------------------------------------------------------------
VkPipelineDepthStencilStateCreateInfo quetzal::vulkan::vk_pipeline_depth_stencil_state_create_info(bool depth_test_enable, bool depth_write_enable, VkCompareOp depth_compare_op)
{
    VkPipelineDepthStencilStateCreateInfo ci{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    ci.depthTestEnable = depth_test_enable;
    ci.depthWriteEnable = depth_write_enable;
    ci.depthCompareOp = depth_compare_op;
    ci.depthBoundsTestEnable = VK_FALSE;
    ci.stencilTestEnable = VK_FALSE;
    //ci.front
    //ci.back
    //ci.minDepthBounds
    //ci.maxDepthBounds
    return ci;
}

//------------------------------------------------------------------------------
VkPipelineDynamicStateCreateInfo quetzal::vulkan::vk_pipeline_dynamic_state_create_info(std::span<VkDynamicState> dynamic_states)
{
    VkPipelineDynamicStateCreateInfo ci{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    ci.dynamicStateCount = to_uint32(dynamic_states.size());
    ci.pDynamicStates = dynamic_states.data();
    return ci;
}

//------------------------------------------------------------------------------
VkPipelineInputAssemblyStateCreateInfo quetzal::vulkan::vk_pipeline_input_assembly_state_create_info(VkPrimitiveTopology topology, bool primitive_restart_enable)
{
    VkPipelineInputAssemblyStateCreateInfo ci{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    ci.topology = topology;
    ci.primitiveRestartEnable = primitive_restart_enable;
    return ci;
}

//------------------------------------------------------------------------------
VkPipelineMultisampleStateCreateInfo quetzal::vulkan::vk_pipeline_multisample_state_create_info(VkSampleCountFlagBits sample_count)
{
    VkPipelineMultisampleStateCreateInfo ci{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    ci.rasterizationSamples = sample_count;
    ci.sampleShadingEnable = VK_FALSE;
    ci.minSampleShading = 0.0f;
    ci.pSampleMask = 0;
    ci.alphaToCoverageEnable = VK_FALSE;
    ci.alphaToOneEnable = VK_FALSE;
    return ci;
}

//------------------------------------------------------------------------------
VkPipelineLayoutCreateInfo quetzal::vulkan::vk_pipeline_layout_create_info(std::span<VkDescriptorSetLayout> set_layouts, std::span<VkPushConstantRange> push_constant_ranges)
{
    VkPipelineLayoutCreateInfo ci{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    ci.setLayoutCount = to_uint32(set_layouts.size());
    ci.pSetLayouts = set_layouts.data();
    ci.pushConstantRangeCount = to_uint32(push_constant_ranges.size());
    ci.pPushConstantRanges = push_constant_ranges.data();
    return ci;
}

//------------------------------------------------------------------------------
VkPipelineRasterizationStateCreateInfo quetzal::vulkan::vk_pipeline_rasterization_state_create_info(VkPolygonMode polygon_mode, VkCullModeFlags cull_mode, VkFrontFace front_face)
{
    VkPipelineRasterizationStateCreateInfo ci{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    ci.depthClampEnable = VK_FALSE;
    ci.rasterizerDiscardEnable = VK_FALSE;
    ci.polygonMode = polygon_mode;
    ci.cullMode = cull_mode;
    ci.frontFace = front_face;
    ci.depthBiasEnable = VK_FALSE;
    //ci.depthBiasConstantFactor
    //ci.depthBiasClamp
    //ci.depthBiasSlopeFactor
    ci.lineWidth = 1.0f;
    return ci;
}

//------------------------------------------------------------------------------
VkPipelineShaderStageCreateInfo quetzal::vulkan::vk_pipeline_shader_stage_create_info(const VkShaderModule& shader_module, VkShaderStageFlagBits stage, const char* name, const VkSpecializationInfo* p_specialization_info)
{
    VkPipelineShaderStageCreateInfo ci{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    ci.stage = stage;
    ci.module = shader_module;
    ci.pName = name;
    ci.pSpecializationInfo = p_specialization_info;
    return ci;
}

//------------------------------------------------------------------------------
VkPipelineVertexInputStateCreateInfo quetzal::vulkan::vk_pipeline_vertex_input_state_create_info(std::span<VkVertexInputBindingDescription> binding_descriptions, std::span<const VkVertexInputAttributeDescription> attribute_descriptions)
{
    VkPipelineVertexInputStateCreateInfo ci{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    ci.vertexBindingDescriptionCount = to_uint32(binding_descriptions.size());
    ci.pVertexBindingDescriptions = binding_descriptions.data();
    ci.vertexAttributeDescriptionCount = to_uint32(attribute_descriptions.size());
    ci.pVertexAttributeDescriptions = attribute_descriptions.data();
    return ci;
}

//------------------------------------------------------------------------------
VkPipelineViewportStateCreateInfo quetzal::vulkan::vk_pipeline_viewport_state_create_info()
{
    // Initialized for dynamic usage
    VkPipelineViewportStateCreateInfo ci{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    ci.viewportCount = 1;
    //ci.pViewports
    ci.scissorCount = 1;
    //ci.pScissors
    return ci;
}

//------------------------------------------------------------------------------
VkRenderPassCreateInfo quetzal::vulkan::vk_render_pass_create_info(std::span<VkAttachmentDescription> attachments, std::span<VkSubpassDescription> subpasses, std::span<VkSubpassDependency> dependencies)
{
    VkRenderPassCreateInfo ci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    ci.attachmentCount = to_uint32(attachments.size());
    ci.pAttachments = attachments.data();
    ci.subpassCount = to_uint32(subpasses.size());
    ci.pSubpasses = subpasses.data();
    ci.dependencyCount = to_uint32(dependencies.size());
    ci.pDependencies = dependencies.data();
    return ci;
}

//------------------------------------------------------------------------------
VkSamplerCreateInfo quetzal::vulkan::vk_sampler_create_info(VkPhysicalDeviceProperties properties)
{
    VkSamplerCreateInfo ci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    ci.magFilter = VK_FILTER_LINEAR;
    ci.minFilter = VK_FILTER_LINEAR;
    ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    ci.mipLodBias = 0.0f;
    ci.anisotropyEnable = VK_TRUE;
    ci.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    ci.compareEnable = VK_FALSE;
    ci.compareOp = VK_COMPARE_OP_ALWAYS;
    ci.minLod = 0.0f;
    ci.maxLod = VK_LOD_CLAMP_NONE;
    ci.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    ci.unnormalizedCoordinates = VK_FALSE;
    return ci;
}

//------------------------------------------------------------------------------
VkSemaphoreCreateInfo quetzal::vulkan::vk_semaphore_create_info()
{
    VkSemaphoreCreateInfo ci{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    return ci;
}

//------------------------------------------------------------------------------
VkSemaphoreTypeCreateInfo quetzal::vulkan::vk_semaphore_type_create_info(VkSemaphoreType semaphore_type, uint64_t initial_value)
{
    VkSemaphoreTypeCreateInfo ci{VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO};
    ci.semaphoreType = semaphore_type;
    ci.initialValue = initial_value;
    return ci;
}

//------------------------------------------------------------------------------
VkShaderModuleCreateInfo quetzal::vulkan::vk_shader_module_create_info(std::span<const char> code)
{
    VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    ci.codeSize = code.size();
    ci.pCode = reinterpret_cast<const uint32_t*>(code.data());
    return ci;
}

//------------------------------------------------------------------------------
VkSwapchainCreateInfoKHR quetzal::vulkan::vk_swapchain_create_info(VkSurfaceKHR surface, uint32_t image_count, VkExtent2D extent, VkSurfaceFormatKHR surface_format, VkImageUsageFlagBits image_usage)
{
    VkSwapchainCreateInfoKHR ci{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    ci.surface = surface;
    ci.minImageCount = image_count;
    ci.imageFormat = surface_format.format;
    ci.imageColorSpace = surface_format.colorSpace;
    ci.imageExtent = extent;
    ci.imageArrayLayers = 1;
    ci.imageUsage = image_usage;
    //ci.imageSharingMode
    //ci.queueFamilyIndexCount
    //ci.pQueueFamilyIndices
    //ci.preTransform
    //ci.compositeAlpha
    //ci.presentMode
    //ci.clipped
    //ci.oldSwapchain
    return ci;
}

//------------------------------------------------------------------------------
VkAttachmentDescription quetzal::vulkan::vk_attachment_description(VkFormat format, VkSampleCountFlagBits samples, VkAttachmentLoadOp load_op, VkAttachmentStoreOp store_op, VkAttachmentLoadOp stencil_load_op, VkAttachmentStoreOp stencil_store_op, VkImageLayout initial_layout, VkImageLayout final_layout, VkAttachmentDescriptionFlags flags)
{
    VkAttachmentDescription attachment_description{};
    attachment_description.format = format;
    attachment_description.samples = samples;
    attachment_description.loadOp = load_op;
    attachment_description.storeOp = store_op;
    attachment_description.stencilLoadOp = stencil_load_op;
    attachment_description.stencilStoreOp = stencil_store_op;
    attachment_description.initialLayout = initial_layout;
    attachment_description.finalLayout = final_layout;
    attachment_description.flags = flags;
    return attachment_description;
}

//------------------------------------------------------------------------------
VkCommandBufferAllocateInfo quetzal::vulkan::vk_command_buffer_allocate_info(VkCommandPool command_pool, VkCommandBufferLevel level, size_t command_buffer_count)
{
    VkCommandBufferAllocateInfo allocate_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocate_info.commandPool = command_pool;
    allocate_info.level = level;
    allocate_info.commandBufferCount = to_uint32(command_buffer_count);
    return allocate_info;
}

//------------------------------------------------------------------------------
VkCommandBufferBeginInfo quetzal::vulkan::vk_command_buffer_begin_info(VkCommandBufferUsageFlags flags)
{
    VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = flags;
    return begin_info;
}

//------------------------------------------------------------------------------
VkDescriptorSetLayoutBinding quetzal::vulkan::vk_descriptor_set_layout_binding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage_flags, uint32_t descriptor_count)
{
    VkDescriptorSetLayoutBinding layout_binding{};
    layout_binding.binding = binding;
    layout_binding.descriptorType = type;
    layout_binding.descriptorCount = descriptor_count;
    layout_binding.stageFlags = stage_flags;
    return layout_binding;
}

//------------------------------------------------------------------------------
VkPhysicalDeviceDynamicRenderingFeatures quetzal::vulkan::vk_physical_device_dynamic_rendering_features(bool dynamic_rendering)
{
    VkPhysicalDeviceDynamicRenderingFeatures features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES};
    features.dynamicRendering = dynamic_rendering;
    return features;
}

//------------------------------------------------------------------------------
VkPhysicalDeviceSynchronization2Features quetzal::vulkan::vk_physical_device_synchronization2_features(bool synchronization2)
{
    VkPhysicalDeviceSynchronization2Features features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES};
    features.synchronization2 = synchronization2;
    return features;
}

//------------------------------------------------------------------------------
VkPipelineColorBlendAttachmentState quetzal::vulkan::vk_pipeline_color_blend_attachment_state(VkColorComponentFlags color_write_mask, bool blend_enable)
{
    VkPipelineColorBlendAttachmentState state{};
    state.blendEnable = blend_enable;
    state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    state.colorBlendOp = VK_BLEND_OP_ADD;
    state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    state.alphaBlendOp = VK_BLEND_OP_ADD;
    state.colorWriteMask = color_write_mask;
    return state;
}

//------------------------------------------------------------------------------
VkPresentInfoKHR quetzal::vulkan::vk_present_info(std::span<const VkSemaphore> semaphores, std::span<const VkSwapchainKHR> swapchains, std::span<uint32_t> image_indices)
{
    assert(swapchains.size() == image_indices.size());

    VkPresentInfoKHR info{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    info.waitSemaphoreCount = to_uint32(semaphores.size());
    info.pWaitSemaphores = semaphores.data();
    info.swapchainCount = to_uint32(swapchains.size());
    info.pSwapchains = swapchains.data();
    info.pImageIndices = image_indices.data();
    //info.pResults
    return info;
}

//------------------------------------------------------------------------------
VkPushConstantRange quetzal::vulkan::vk_push_constant_range(VkShaderStageFlags stage_flags, uint32_t offset, uint32_t size)
{
    VkPushConstantRange range{};
    range.stageFlags = stage_flags;
    range.offset = offset;
    range.size = size;
    return range;
}

//------------------------------------------------------------------------------
VkRenderPassBeginInfo quetzal::vulkan::vk_render_pass_begin_info(VkRenderPass render_pass, VkFramebuffer framebuffer, VkExtent2D extent, std::span<VkClearValue> clear_values)
{
    VkRenderPassBeginInfo info{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    info.renderPass = render_pass;
    info.framebuffer = framebuffer;
    info.renderArea.extent = extent;
    info.clearValueCount = to_uint32(clear_values.size());
    info.pClearValues = clear_values.data();
    return info;
}

//------------------------------------------------------------------------------
VkRenderingAttachmentInfo quetzal::vulkan::vk_rendering_attachment_info_color(VkImageView image_view, VkImageLayout image_layout, VkResolveModeFlagBits resolve_mode, VkImageView resolve_image_view, VkImageLayout resolve_image_layout, const VkClearValue& clear_value)
{
    VkRenderingAttachmentInfo info{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    info.imageView = image_view;
    info.imageLayout = image_layout;
    info.resolveMode = resolve_mode;
    info.resolveImageView = resolve_image_view;
    info.resolveImageLayout = resolve_image_layout;
    info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    info.clearValue = clear_value;
    return info;
}

//------------------------------------------------------------------------------
VkRenderingAttachmentInfo quetzal::vulkan::vk_rendering_attachment_info_depth(VkImageView image_view, VkImageLayout image_layout, VkResolveModeFlagBits resolve_mode, VkImageView resolve_image_view, VkImageLayout resolve_image_layout, const VkClearValue& clear_value)
{
    VkRenderingAttachmentInfo info{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    info.imageView = image_view;
    info.imageLayout = image_layout;
    info.resolveMode = resolve_mode;
    info.resolveImageView = resolve_image_view;
    info.resolveImageLayout = resolve_image_layout;
    info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    info.clearValue = clear_value;
    return info;
}

//------------------------------------------------------------------------------
VkRenderingAttachmentInfo quetzal::vulkan::vk_rendering_attachment_info_stencil(VkImageView image_view, VkImageLayout image_layout, VkResolveModeFlagBits resolve_mode, VkImageView resolve_image_view, VkImageLayout resolve_image_layout, const VkClearValue& clear_value)
{
    VkRenderingAttachmentInfo info = vk_rendering_attachment_info_depth(image_view, image_layout, resolve_mode, resolve_image_view, resolve_image_layout, clear_value);
    info.imageLayout = image_layout;
    info.resolveMode = resolve_mode;
    return info;
}

//------------------------------------------------------------------------------
VkRenderingInfo quetzal::vulkan::vk_rendering_info(VkRect2D render_area, uint32_t layer_count, uint32_t view_mask, std::span<const VkRenderingAttachmentInfo> color_attachments, std::optional<std::reference_wrapper<const VkRenderingAttachmentInfo>> depth_attachment, std::optional<std::reference_wrapper<const VkRenderingAttachmentInfo>> stencil_attachment)
{
    VkRenderingInfo info{VK_STRUCTURE_TYPE_RENDERING_INFO};
    info.renderArea = render_area;
    info.layerCount = layer_count;
    info.viewMask = view_mask;
    info.colorAttachmentCount = to_uint32(color_attachments.size());
    info.pColorAttachments = color_attachments.data();
    info.pDepthAttachment = depth_attachment ? &depth_attachment->get() : nullptr;
    info.pStencilAttachment = stencil_attachment ? &stencil_attachment->get() : nullptr;
    return info;
}

//------------------------------------------------------------------------------
VkStencilOpState quetzal::vulkan::vk_stencil_op_state(VkCompareOp compare_op)
{
    VkStencilOpState state{};
    //state.failOp
    //state.passOp
    //state.depthFailOp
    state.compareOp = compare_op;
    //state.compareMask
    //state.writeMask
    //state.reference
    return state;
}

//------------------------------------------------------------------------------
VkSemaphoreSubmitInfo quetzal::vulkan::vk_semaphore_submit_info(VkSemaphore semaphore, VkPipelineStageFlags2 stage_mask, uint64_t value)
{
    VkSemaphoreSubmitInfo info{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    info.semaphore = semaphore;
    info.stageMask = stage_mask;
    info.value = value;
    return info;
}

//------------------------------------------------------------------------------
VkSubmitInfo quetzal::vulkan::vk_submit_info(std::span<const VkSemaphore> wait_semaphores, std::span<const VkPipelineStageFlags> wait_stage_flags, std::span<const VkCommandBuffer> command_buffers, std::span<const VkSemaphore> signal_semaphores)
{
    assert(wait_semaphores.size() == wait_stage_flags.size());

    VkSubmitInfo info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    info.waitSemaphoreCount = to_uint32(wait_semaphores.size());
    info.pWaitSemaphores = wait_semaphores.data();
    info.pWaitDstStageMask = wait_stage_flags.data();
    info.commandBufferCount = to_uint32(command_buffers.size());
    info.pCommandBuffers = command_buffers.data();
    info.signalSemaphoreCount = to_uint32(signal_semaphores.size());
    info.pSignalSemaphores = signal_semaphores.data();
    return info;
}

//------------------------------------------------------------------------------
VkSubmitInfo quetzal::vulkan::vk_submit_info(std::span<const VkCommandBuffer> command_buffers)
{
    VkSubmitInfo info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    //info.waitSemaphoreCount = ;
    //info.pWaitSemaphores = ;
    //info.pWaitDstStageMask = ;
    info.commandBufferCount = to_uint32(command_buffers.size());
    info.pCommandBuffers = command_buffers.data();
    //info.signalSemaphoreCount = ;
    //info.pSignalSemaphores = ;
    return info;
}

//------------------------------------------------------------------------------
VkCommandBufferSubmitInfo quetzal::vulkan::vk_command_buffer_submit_info(VkCommandBuffer command_buffer, uint32_t device_mask)
{
    VkCommandBufferSubmitInfo info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    info.commandBuffer = command_buffer;
    info.deviceMask = device_mask;
    return info;
}

//------------------------------------------------------------------------------
VkSubmitInfo2 quetzal::vulkan::vk_submit_info2(std::span<const VkSemaphoreSubmitInfo> wait_semaphores, std::span<const VkCommandBufferSubmitInfo> command_buffers, std::span<const VkSemaphoreSubmitInfo> signal_semaphores)
{
    VkSubmitInfo2 info{VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    info.waitSemaphoreInfoCount = to_uint32(wait_semaphores.size());
    info.pWaitSemaphoreInfos = wait_semaphores.data();
    info.commandBufferInfoCount = to_uint32(command_buffers.size());
    info.pCommandBufferInfos = command_buffers.data();
    info.signalSemaphoreInfoCount = to_uint32(signal_semaphores.size());
    info.pSignalSemaphoreInfos = signal_semaphores.data();
    return info;
}

//------------------------------------------------------------------------------
VkSubpassDependency quetzal::vulkan::vk_subpass_dependency(uint32_t source_subpass, uint32_t dest_subpass, VkPipelineStageFlags source_stage_mask, VkPipelineStageFlags dest_stage_mask, VkAccessFlags source_access_mask, VkAccessFlags dest_access_mask, VkDependencyFlags flags)
{
    VkSubpassDependency dependency{};
    dependency.srcSubpass = source_subpass;
    dependency.dstSubpass = dest_subpass;
    dependency.srcStageMask = source_stage_mask;
    dependency.dstStageMask = dest_stage_mask;
    dependency.srcAccessMask = source_access_mask;
    dependency.dstAccessMask = dest_access_mask;
    dependency.dependencyFlags = flags;
    return dependency;
}

//------------------------------------------------------------------------------
VkSubpassDescription quetzal::vulkan::vk_subpass_description(VkPipelineBindPoint pipeline_bind_point, std::span<VkAttachmentReference> input_attachments, std::span<VkAttachmentReference> color_attachments,
        const VkAttachmentReference& resolve_attachment, const VkAttachmentReference& depth_stencil_attachment, std::span<uint32_t> preserve_attachments)
{
    VkSubpassDescription description{};
    description.flags = 0;
    description.pipelineBindPoint = pipeline_bind_point;
    description.inputAttachmentCount = to_uint32(input_attachments.size());
    description.pInputAttachments = input_attachments.data();
    description.colorAttachmentCount = to_uint32(color_attachments.size());
    description.pColorAttachments = color_attachments.data();
    description.pResolveAttachments = &resolve_attachment;
    description.pDepthStencilAttachment = &depth_stencil_attachment;
    description.preserveAttachmentCount = to_uint32(preserve_attachments.size());
    description.pPreserveAttachments = preserve_attachments.data();
    return description;
}
