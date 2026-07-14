#if !defined(QUETZAL_VULKAN_RENDERER_HPP)
#define QUETZAL_VULKAN_RENDERER_HPP
//------------------------------------------------------------------------------
// quetzal
// Renderer.hpp
//------------------------------------------------------------------------------

#include "IRenderer.hpp"
#include "Buffer.hpp"
#include "CommandBuffers.hpp"
#include "CommandPool.hpp"
#include "Context.hpp"
#include "Descriptors.hpp"
#include "Device.hpp"
#include "Fence.hpp"
#include "Geometry.hpp"
#include "GeometryCache.hpp"
#include "MaterialCache.hpp"
#include "MaterialInstance.hpp"
#include "MaterialInstanceCache.hpp"
#include "PhysicalDevice.hpp"
#include "PipelineManager.hpp"
#include "RenderingAttachments.hpp"
#include "ResourceManager.hpp"
#include "Semaphore.hpp"
#include "Surface.hpp"
#include "Swapchain.hpp"
#include "TextureCache.hpp"
#include "UniformBuffers.hpp"
#include "VulkanError.hpp"
#include "Window.hpp"
#include "handles.hpp"
#include "initializer.hpp"
#include "quetzal/common/Config.hpp"
#include "quetzal/game/types.hpp"
#include <glm/gtc/matrix_inverse.hpp>
#include <vulkan/vulkan.hpp>
#include <optional>
#include <span>
#include <vector>
#include <cstdint>
#include <cstdlib>

namespace quetzal::vulkan
{

    namespace detail
    {

#ifdef NDEBUG
        const std::vector<const char*> validation_layers = {};
#else
        const std::vector<const char*> validation_layers =
        {
            "VK_LAYER_KHRONOS_validation"
        };
#endif

        const std::vector<const char*> device_extensions =
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

    } // namespace detail

    struct SceneLight
    {
        glm::vec4 position;
        glm::vec4 color;
        float intensity;
        float _pad[3];
    };

    struct SceneUBO
    {
        alignas(16) game::matrix_type view_matrix;
        game::matrix_type proj_matrix;
        glm::vec4 camera_pos;
        SceneLight lights[8];
        int32_t light_count;
        int32_t _pad[3];
    };

    static constexpr uint32_t transform_push_offset = 0;
    static constexpr uint32_t transform_push_size = static_cast<uint32_t>(sizeof(game::matrix_type));
    static constexpr uint32_t material_push_offset = static_cast<uint32_t>(sizeof(game::matrix_type));
    static constexpr uint32_t material_push_size = 128;

    struct TransformPushConstants
    {
        alignas(16) game::matrix_type model_matrix;
    };

    class Renderer : public IRenderer
    {
    public:

        Renderer(const Config& config, const Window& window) :
            m_context(config, window, detail::validation_layers),
            m_device(m_context, window, detail::device_extensions, physical_device_selector),
            m_resource_manager(m_device),
            m_pipeline_manager(m_device, "."),
            m_swap_frame_count(swap_frame_count_(config, 2)),
            m_msaa_samples(VK_SAMPLE_COUNT_1_BIT),
            m_descriptor_allocator(m_device),
            m_descriptor_layout_cache(m_device),
            m_scene_uniform_buffers(m_device, m_swap_frame_count),
            m_set0_layout(VK_NULL_HANDLE),
            m_set1_layout(VK_NULL_HANDLE),
            m_scene_ubo{},
            m_current_frame(0),
            m_current_image_index(0),
            m_framebuffer_resized(false)
        {
        }

        ~Renderer()
        {
            m_device.wait_idle();

            if (m_material_instance_cache) m_material_instance_cache->clear();
            if (m_material_cache) m_material_cache->clear();
            m_descriptor_layout_cache.clear();
            m_descriptor_allocator.clear();
            if (m_texture_cache) m_texture_cache->clear();
            if (m_geometry_cache) m_geometry_cache->clear();
        }

        void create()
        {
            m_msaa_samples = m_device.physical_device().max_sample_count();

            m_surface_resources.emplace(m_device, m_swap_frame_count, m_msaa_samples);

            VkDescriptorSetLayoutBinding set0_binding{};
            set0_binding.binding = 0;
            set0_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            set0_binding.descriptorCount = 1;
            set0_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            set0_binding.pImmutableSamplers = nullptr;
            std::array<VkDescriptorSetLayoutBinding, 1> set0_bindings = {set0_binding};
            m_set0_layout = m_descriptor_layout_cache.create_descriptor_layout(set0_bindings);

            VkDescriptorSetLayoutBinding set1_binding{};
            set1_binding.binding = 0;
            set1_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            set1_binding.descriptorCount = 1;
            set1_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            set1_binding.pImmutableSamplers = nullptr;
            std::array<VkDescriptorSetLayoutBinding, 1> set1_bindings = {set1_binding};
            m_set1_layout = m_descriptor_layout_cache.create_descriptor_layout(set1_bindings);

            m_scene_descriptor_sets.resize(m_swap_frame_count);
            for (size_t i = 0; i < m_swap_frame_count; ++i)
            {
                VkDescriptorBufferInfo buffer_info = m_scene_uniform_buffers[i].descriptor();
                DescriptorBuilder::begin(&m_descriptor_allocator, &m_descriptor_layout_cache)
                    .bind(0, buffer_info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
                    .build(m_scene_descriptor_sets[i]);
            }

            VkPushConstantRange push_range = vk_push_constant_range(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, transform_push_size + material_push_size);
            m_material_cache.emplace(
                m_device,
                m_set0_layout,
                m_set1_layout,
                m_resource_manager,
                m_pipeline_manager,
                m_surface_resources->swapchain.image_format(),
                m_surface_resources->swapchain.depth_format(),
                m_msaa_samples,
                std::span<VkPushConstantRange>{&push_range, 1}
            );

            const Queue& graphics_queue = m_device.queues().get(VK_QUEUE_GRAPHICS_BIT, 0);
            m_command_resources.emplace(m_device, m_swap_frame_count, graphics_queue.family_index());
            m_geometry_cache.emplace(m_device, m_command_resources->pool);
            m_texture_cache.emplace(m_device, m_command_resources->pool, graphics_queue);

            m_material_instance_cache.emplace(
                m_device,
                m_descriptor_allocator,
                m_descriptor_layout_cache,
                *m_material_cache,
                *m_texture_cache,
                m_swap_frame_count
            );

            for (size_t i = 0; i < m_swap_frame_count; ++i)
            {
                m_image_available_semaphores.emplace_back(m_device);
                m_in_flight_fences.emplace_back(m_device);
            }
        }

        size_t swap_frame_count() const
        {
            return m_swap_frame_count;
        }

        VkExtent2D extent() const
        {
            return m_surface_resources->swapchain.extent();
        }

        void set_resized()
        {
            m_framebuffer_resized = true;
        }

        GeometryHandle upload_geometry(const std::string& id, const void* vertex_data, size_t vertex_count, const VertexBinding& layout, std::span<const uint32_t> indices) override
        {
            Geometry::buffer_type bytes{static_cast<const uint8_t*>(vertex_data), vertex_count * layout.stride};
            return m_geometry_cache->upload(id, bytes, indices);
        }

        TextureHandle upload_texture(const std::string& id, const TextureData& td) override
        {
            return m_texture_cache->upload(id, td);
        }

        MaterialHandle create_material(const std::string& vert_spv, const std::string& frag_spv, const VertexBinding& layout) override
        {
            VkVertexInputBindingDescription binding_desc{layout.binding, layout.stride, VK_VERTEX_INPUT_RATE_VERTEX};
            std::vector<VkVertexInputAttributeDescription> attr_descs;
            for (const auto& attr : layout.attributes)
                attr_descs.push_back({attr.location, layout.binding, static_cast<VkFormat>(attr.format), attr.offset});
            return m_material_cache->create(vert_spv, frag_spv, binding_desc, attr_descs);
        }

        MaterialInstanceHandle create_material_instance(const MaterialHandle& material, const MaterialParameters& params) override
        {
            return m_material_instance_cache->create(material, params);
        }

        void update_material_instance(const MaterialInstanceHandle& handle, const MaterialParameters& params) override
        {
            m_material_instance_cache->update(handle, params);
        }

        template<typename V>
        GeometryHandle upload_geometry(const std::string& id, std::span<const V> vertices, std::span<const Geometry::index_type> indices)
        {
            return m_geometry_cache->upload<V>(id, vertices, indices);
        }

        bool begin_frame() override
        {
            m_in_flight_fences[m_current_frame].wait();

            auto [result, image_index] = m_surface_resources->swapchain.acquire_next_image(m_image_available_semaphores[m_current_frame]);
            if (result == VK_ERROR_OUT_OF_DATE_KHR)
            {
                recreate_surface_resources();
                return false;
            }
            else if (result != VK_SUBOPTIMAL_KHR)
            {
                check_result(result, "vkAcquireNextImageKHR");
            }

            m_current_image_index = image_index;

            CommandBuffer& command_buffer = m_command_resources->buffers[m_current_frame];
            m_in_flight_fences[m_current_frame].reset();
            vkResetCommandBuffer(command_buffer, /*VkCommandBufferResetFlagBits*/ 0);
            command_buffer.begin();

            auto ext = m_surface_resources->swapchain.extent();
            VkImage swapchain_image = m_surface_resources->swapchain.images()[m_current_image_index];

            VkClearValue color_clear{};
            color_clear.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
            VkClearValue depth_clear{};
            depth_clear.depthStencil = {1.0f, 0};

            std::array<VkImageMemoryBarrier2, 3> pre_barriers = {
                vk_image_memory_barrier2(swapchain_image,
                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
                    VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT),
                vk_image_memory_barrier2(m_surface_resources->attachments.color_image(),
                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
                    VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT),
                vk_image_memory_barrier2(m_surface_resources->attachments.depth_image(),
                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
                    VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                    VK_IMAGE_ASPECT_DEPTH_BIT),
            };
            VkDependencyInfo pre_dep = vk_dependency_info(pre_barriers);
            vkCmdPipelineBarrier2(command_buffer, &pre_dep);

            VkRenderingAttachmentInfo color_attachment = vk_rendering_attachment_info_color(
                m_surface_resources->attachments.color_image_view(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_RESOLVE_MODE_AVERAGE_BIT,
                m_surface_resources->swapchain.image_views()[m_current_image_index], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                color_clear);

            VkRenderingAttachmentInfo depth_attachment = vk_rendering_attachment_info_depth(
                m_surface_resources->attachments.depth_image_view(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_RESOLVE_MODE_NONE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED,
                depth_clear);

            VkRect2D render_area{{0, 0}, ext};
            VkRenderingInfo rendering_info = vk_rendering_info(render_area, 1, 0, {&color_attachment, 1}, std::cref(depth_attachment));

            command_buffer.begin_rendering(rendering_info);
            command_buffer.set_viewport(ext);
            command_buffer.set_scissor(ext);

            return true;
        }

        void end_frame() override
        {
            CommandBuffer& command_buffer = m_command_resources->buffers[m_current_frame];

            command_buffer.end_rendering();

            VkImage swapchain_image = m_surface_resources->swapchain.images()[m_current_image_index];
            VkImageMemoryBarrier2 post_barrier = vk_image_memory_barrier2(swapchain_image,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0);
            VkDependencyInfo post_dep = vk_dependency_info({&post_barrier, 1});
            vkCmdPipelineBarrier2(command_buffer, &post_dep);

            command_buffer.end();

            VkSemaphoreSubmitInfo wait_semaphore = vk_semaphore_submit_info(m_image_available_semaphores[m_current_frame], VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
            VkCommandBufferSubmitInfo cmd_submit = vk_command_buffer_submit_info(command_buffer);
            VkSemaphoreSubmitInfo signal_semaphore = vk_semaphore_submit_info(m_surface_resources->rendering_finished_semaphores[m_current_image_index], VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);
            VkSubmitInfo2 submit_info2 = vk_submit_info2({&wait_semaphore, 1}, {&cmd_submit, 1}, {&signal_semaphore, 1});

            const Queue& graphics_queue = m_device.queues().get(VK_QUEUE_GRAPHICS_BIT, 0);
            graphics_queue.submit2({&submit_info2, 1}, m_in_flight_fences[m_current_frame]);

            std::array<const VkSwapchainKHR, 1> swapchains = {m_surface_resources->swapchain};
            VkSemaphore rendering_finished = m_surface_resources->rendering_finished_semaphores[m_current_image_index];
            VkPresentInfoKHR present_info = vk_present_info({&rendering_finished, 1}, swapchains, {&m_current_image_index, 1});
            const Queue& present_queue = m_device.queues().get_presentable_queue(0);
            VkResult result = present_queue.present(present_info);
            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebuffer_resized)
            {
                m_framebuffer_resized = false;
                recreate_surface_resources();
            }
            else
            {
                check_result(result, "vkQueuePresentKHR");
            }

            m_current_frame = (m_current_frame + 1) % m_swap_frame_count;
        }

        void set_camera(const game::matrix_type& view, const game::matrix_type& proj) override
        {
            m_scene_ubo.view_matrix = view;
            m_scene_ubo.proj_matrix = proj;
            game::matrix_type inv_view = glm::inverse(view);
            m_scene_ubo.camera_pos = glm::vec4(glm::vec3(inv_view[3]), 1.0f);
            m_scene_uniform_buffers.set(m_current_frame, m_scene_ubo);
        }

        void set_lights(std::span<const Light> lights) override
        {
            m_scene_ubo.light_count = static_cast<int32_t>(std::min(lights.size(), size_t(8)));
            for (int32_t i = 0; i < m_scene_ubo.light_count; ++i)
            {
                m_scene_ubo.lights[i].position = glm::vec4(lights[i].position, 1.0f);
                m_scene_ubo.lights[i].color = glm::vec4(lights[i].color, 1.0f);
                m_scene_ubo.lights[i].intensity = lights[i].intensity;
            }
            m_scene_uniform_buffers.set(m_current_frame, m_scene_ubo);
        }

        void draw_geometry(const GeometryHandle& geometry_handle, const MaterialInstanceHandle& material_instance_handle, const game::matrix_type& model_matrix) override
        {
            const Geometry& geometry = m_geometry_cache->get(geometry_handle);
            const MaterialInstance& material_instance = m_material_instance_cache->get(material_instance_handle);

            TransformPushConstants transform_pcs;
            transform_pcs.model_matrix = model_matrix;

            CommandBuffer& command_buffer = m_command_resources->buffers[m_current_frame];

            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material_instance.pipeline());
            material_instance.bind(command_buffer, m_scene_descriptor_sets[m_current_frame], m_current_frame);

            VkBuffer vertex_buffers[] = {geometry.vertex_buffer()};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
            vkCmdBindIndexBuffer(command_buffer, geometry.index_buffer(), 0, VK_INDEX_TYPE_UINT32);
            vkCmdPushConstants(command_buffer, material_instance.pipeline_layout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, transform_push_offset, transform_push_size, &transform_pcs);
            vkCmdDrawIndexed(command_buffer, to_uint32(geometry.index_count()), 1, 0, 0, 0);
        }

    private:

        struct SurfaceResources
        {
            SurfaceResources(const Device& device, size_t frame_count, VkSampleCountFlagBits msaa_samples) :
                swapchain(device, device.window()),
                attachments(device)
            {
                swapchain.create(device.window().extent(), to_uint32(frame_count));
                attachments.create(swapchain.extent(), swapchain.image_format(), swapchain.depth_format(), msaa_samples);
                for (size_t i = 0; i < swapchain.images().size(); ++i)
                    rendering_finished_semaphores.emplace_back(device);
            }

            SurfaceResources(const SurfaceResources&) = delete;
            SurfaceResources(SurfaceResources&&) = delete;

            ~SurfaceResources()
            {
                rendering_finished_semaphores.clear();
                swapchain.destroy();
            }

            Swapchain swapchain;
            RenderingAttachments attachments;
            std::vector<Semaphore> rendering_finished_semaphores;
        };

        struct CommandResources
        {
            CommandResources(const Device& device, size_t frame_count, uint32_t queue_family) :
                pool(device),
                buffers(device, pool)
            {
                pool.create(queue_family);
                buffers.create(frame_count);
            }

            CommandResources(const CommandResources&) = delete;
            CommandResources(CommandResources&&) = delete;

            ~CommandResources()
            {
                buffers.destroy();
                pool.destroy();
            }

            CommandPool pool;
            CommandBuffers buffers;
        };

        size_t swap_frame_count_(const Config& config, size_t n_default = 2)
        {
            size_t n = static_cast<size_t>(::strtoul(config["swap_frame_count"].c_str(), nullptr, 10));
            return n > 0 ? n : n_default;
        }

        void recreate_surface_resources()
        {
            VkExtent2D ext = m_device.window().extent();
            while (ext.width == 0 || ext.height == 0)
            {
                m_device.window().wait_events();
                ext = m_device.window().extent();
            }

            m_device.wait_idle();
            m_surface_resources.reset();
            m_surface_resources.emplace(m_device, m_swap_frame_count, m_msaa_samples);
        }

        static bool physical_device_selector(const PhysicalDevice& physical_device, const Surface& surface)
        {
            bool extensions_supported = physical_device.check_extension_support(detail::device_extensions);

            SurfaceSupport surface_support = physical_device.surface_support(surface);
            bool swapchain_adequate = !surface_support.formats.empty() && !surface_support.present_modes.empty();

            VkPhysicalDeviceFeatures supported_features;
            vkGetPhysicalDeviceFeatures(physical_device, &supported_features);

            return physical_device.queue_family_presentable(surface) && extensions_supported && swapchain_adequate && supported_features.samplerAnisotropy;
        }

        // Foundation (declared first, destroyed last)
        Context m_context;
        Device m_device;

        // Configuration
        size_t m_swap_frame_count;
        VkSampleCountFlagBits m_msaa_samples;

        // Permanent services
        ResourceManager m_resource_manager;
        PipelineManager m_pipeline_manager;
        DescriptorAllocator m_descriptor_allocator;
        DescriptorLayoutCache m_descriptor_layout_cache;
        UniformBuffers<SceneUBO> m_scene_uniform_buffers;
        std::vector<Semaphore> m_image_available_semaphores;
        std::vector<Fence> m_in_flight_fences;

        // Scene descriptor state (raw handles managed by m_descriptor_allocator / m_descriptor_layout_cache)
        VkDescriptorSetLayout m_set0_layout;
        VkDescriptorSetLayout m_set1_layout;
        std::vector<VkDescriptorSet> m_scene_descriptor_sets;
        SceneUBO m_scene_ubo;

        // Deferred resources (emplaced in create(), cleared in ~Renderer())
        std::optional<MaterialCache> m_material_cache;
        std::optional<MaterialInstanceCache> m_material_instance_cache;
        std::optional<GeometryCache> m_geometry_cache;
        std::optional<TextureCache> m_texture_cache;
        std::optional<CommandResources> m_command_resources;

        // Recreatable surface resources
        std::optional<SurfaceResources> m_surface_resources;

        // Frame state
        uint32_t m_current_frame;
        uint32_t m_current_image_index;
        bool m_framebuffer_resized;
    };

} // namespace quetzal::vulkan

#endif // QUETZAL_VULKAN_RENDERER_HPP
