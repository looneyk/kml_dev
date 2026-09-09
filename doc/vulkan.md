# Vulkan

The Vulkan based 3D rendering engine in `library/quetzal/vulkan`.

## Build System

This library is built with Visual Studio via `library/quetzal/vulkan/vulkan.vcxproj`. Configurations: Debug/Release × Win32/x64 (plus DLL variants). C++26 standard (`/std:c++latest`). No standalone build command — open the solution in Visual Studio or use MSBuild.

## Dependencies

Located in `library3/`:
- `glfw/include` — windowing
- `glm` — math types
- `stb` — image loading
- `tinyobjloader` — OBJ model loading
- `VulkanMemoryAllocator/include` — GPU memory allocation (`Allocator`)
- `KTX-Software/include` — KTX texture format
- Vulkan SDK via `%VULKAN_SDK%/Include`

The quetzal library root (`library/`) must also be on the include path, providing `quetzal/common/Config.hpp` and `quetzal/game/types.hpp`.

## Architecture

All code lives in namespace `quetzal::vulkan`. The library is a thin C++ RAII wrapper over raw Vulkan, using `vulkan.hpp` (C++ Vulkan bindings), not just `vulkan.h`.

### Object Hierarchy

Initialization flows top-down through ownership:

```
Context
  Instance          (VkInstance, app info, extensions, layers)
  DebugMessenger    (validation layer callback, only in debug builds)
  PhysicalDevices   (enumerates available GPUs)

Device
  Surface           (VkSurfaceKHR, platform window surface)
  PhysicalDevice    (selected GPU, queue family queries, feature checks)
  LogicalDevice     (VkDevice)
  Allocator         (VMA allocator)
  Queues            (graphics + present queues)

Renderer            (top-level class; owns everything below)
  Context
  Device
  ResourceManager   (caches PipelineLayout, RenderPass by hash)
  PipelineManager   (creates/caches GraphicsPipeline, uses VkPipelineCache)
  Swapchain
  Framebuffers
  RenderPass
  DescriptorAllocator + DescriptorLayoutCache   (Descriptors.hpp)
  CommandPool + CommandBuffers
  ModelCache        (loads/caches Model by path)
  TextureCache      (loads/caches Texture by path)
  UniformBuffers<T> (one buffer per swap frame)
  vector<Node>      (scene graph entries)
```

### Key Patterns

- **RAII wrappers**: Each Vulkan object type has its own `.hpp` (and sometimes `.cpp`). Objects expose `create()`/`destroy()` separately from the constructor to allow deferred init; destructors do not automatically destroy unless the handle is valid.
- **`operator VkDevice()`**: `Device`, `LogicalDevice`, and similar wrappers implicitly convert to their underlying Vk handle, so they can be passed directly to raw Vulkan calls.
- **`initializer.hpp`**: All `vk_*` free functions that construct Vulkan `CreateInfo` structs live here. Use these instead of zero-initializing structs manually.
- **`vulkan_util.hpp`**: `to_uint32<T>()`, `to_span<T>()`, and a generic `std::formatter` specialization for enums.
- **Cache classes**: `ResourceCache<T>`, `ModelCache`, `TextureCache`, `PipelineCache`, `DescriptorAllocator`/`DescriptorLayoutCache` all follow the same pattern — request an object by key, return existing or create new.
- **Swap frame indexing**: `m_swap_frame_count` (from config key `swap_frame_count`, default 2) drives per-frame resource arrays (uniform buffers, command buffers, semaphores, fences). `m_current_frame` cycles mod `m_swap_frame_count`.
- **Validation layers**: `VK_LAYER_KHRONOS_validation` is enabled when `NDEBUG` is not defined. Debug builds also add `VK_EXT_DEBUG_UTILS_EXTENSION_NAME`.

### Descriptor System

`Descriptors.hpp` contains three cooperating classes:
- `DescriptorAllocator` — manages pools, grows as needed
- `DescriptorLayoutCache` — deduplicates `VkDescriptorSetLayout` by binding hash
- `DescriptorBuilder` — fluent builder: `begin() → bind() → build()`

### Scene Rendering

`Renderer::insert<V>()` is the entry point for adding geometry. `V` is a vertex type providing `binding_description()` and `attribute_descriptions()`. Each inserted object becomes a `Node` holding descriptor sets, pipeline layout, and pipeline. `Renderer::draw()` records and submits command buffers per frame.

### Vulkan 1.4 Work-in-Progress

`VULKAN_1_4_PROPOSAL.md`, `VULKAN_1_4_TECHNICAL_SPEC.md`, `VULKAN_1_4_IMPLEMENTATION_CHECKLIST.md`, and `VULKAN_1_4_SUMMARY.md` document planned modernization (dynamic rendering, improved synchronization, descriptor indexing). The current codebase targets Vulkan 1.3.
