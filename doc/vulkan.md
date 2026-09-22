# Vulkan

The Vulkan based 3D rendering engine in `library/quetzal/vulkan`.

## Build

Built by the CMake solution as the `quetzal_vulkan` target, with `/std:c++latest`.

## Dependencies

In `library3/`:
- `glfw/include` --- windowing
- `glm` --- math types
- `stb` --- image loading
- `tinyobjloader` --- OBJ model loading
- `VulkanMemoryAllocator/include` --- GPU memory allocation (`Allocator`)
- `KTX-Software/include` --- KTX texture format
- The Vulkan SDK, through `%VULKAN_SDK%/Include`

The quetzal library root (`library/`) is also on the include path, for `quetzal/common/Config.hpp` and
`quetzal/game/types.hpp`.

## Architecture

All code is in namespace `quetzal::vulkan`: RAII wrappers over the Vulkan C API.

### Object hierarchy

Ownership, from the top:

```
Renderer            (implements IRenderer; owns everything below)
  Context
    Instance          (VkInstance, application info, extensions, layers)
    DebugMessenger    (validation layer callback, debug builds only)
    PhysicalDevices   (the available GPUs)
  Device
    Surface           (VkSurfaceKHR, the window surface)
    PhysicalDevice    (the selected GPU, queue family queries, feature checks)
    LogicalDevice     (VkDevice)
    Allocator         (VMA allocator)
    Queues            (graphics and present queues)
  ResourceManager     (caches PipelineLayout by hash)
  PipelineManager     (creates and caches GraphicsPipeline, using VkPipelineCache)
  DescriptorAllocator and DescriptorLayoutCache   (Descriptors.hpp)
  UniformBuffers<SceneUBO>   (one buffer per swap frame)
  GeometryCache, TextureCache, MaterialCache, MaterialInstanceCache
  CommandResources    (CommandPool and CommandBuffers)
  SurfaceResources    (Swapchain and RenderingAttachments, recreated on resize)
```

### Patterns

- **RAII wrappers**: each Vulkan object type has its own `.hpp`, and sometimes a `.cpp`. `create()` and `destroy()`
  are separate from the constructor, for deferred initialization; a destructor destroys only a valid handle.
- **Handle conversion**: `Device`, `LogicalDevice` and similar wrappers convert implicitly to their Vulkan handle, so
  they can be passed directly to Vulkan calls.
- **`initializer.hpp`**: the `vk_*` free functions that build Vulkan `CreateInfo` structs.
- **`vulkan_util.hpp`**: `to_uint32<T>()`, `to_span<T>()`, and a `std::formatter` specialization for enums.
- **Caches**: `ResourceCache<T>`, `GeometryCache`, `TextureCache`, `MaterialCache`, `MaterialInstanceCache`,
  `PipelineCache` and `DescriptorAllocator`/`DescriptorLayoutCache` return the existing object for a key or create it.
- **Swap frames**: `m_swap_frame_count`, from the config key `swap_frame_count` (default 2), sizes the per-frame
  resources --- uniform buffers, command buffers, semaphores and fences --- and `m_current_frame` cycles through them.
- **Validation layers**: `VK_LAYER_KHRONOS_validation` is enabled when `NDEBUG` is not defined.

### Descriptors

`Descriptors.hpp` has three classes:
- `DescriptorAllocator` --- manages pools, adding pools as needed
- `DescriptorLayoutCache` --- deduplicates `VkDescriptorSetLayout` by binding hash
- `DescriptorBuilder` --- a fluent builder, `begin()`, `bind()`, `build()`

### Rendering

`Renderer` implements `IRenderer`: `upload_geometry`, `upload_texture`, `create_material`,
`create_material_instance` and `update_material_instance` create GPU resources and return handles; `begin_frame`,
`set_camera`, `set_lights`, `draw_geometry` and `end_frame` draw a frame. Drawing uses dynamic rendering
(`vkCmdBeginRendering`), with the model matrix as a push constant and the camera and lights in a per-frame scene
uniform buffer. The `scene_graph` library drives it.
