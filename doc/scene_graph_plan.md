# Scene Graph --- Plan

Separate three concerns that `Node` combined:

- scene graph identity --- id, transform, hierarchy
- the renderable --- which geometry, which material
- GPU rendering state --- descriptor sets, pipeline layout, pipeline

`Renderer::insert<V>()` loaded the geometry and texture, built the descriptors, created the pipeline and constructed the
node in one function.

## Layers

1. **`Node`**, the base class: identity, transform and hierarchy, and nothing GPU-specific.

   ```
   id: string
   local_transform: matrix_type
   children: vector<Node*>
   parent: Node*
   ```

   The world transform is computed top down before drawing, with a dirty flag to skip unchanged branches.

2. **`Transform`**: a matrix with a dirty flag, optionally decomposed into translation, rotation and scale.

3. **`Material`**: the texture id (in `TextureCache`), the vertex and fragment shader paths, the descriptor sets per
   swap frame, the pipeline layout (from `ResourceManager`) and the pipeline (from `PipelineManager`).

4. **`MaterialCache`**: keyed by material description, as `ModelCache` and `TextureCache` are keyed.

5. **`GeometryNode : Node`**: a renderable node, with a geometry id (in `ModelCache`) and a material id (in
   `MaterialCache`). `CameraNode`, `LightNode` and others follow the same form.

6. **`Scene`**: the node tree, with a root node, lookup by id, and traversal for transform propagation and drawing.
   `Renderer` owns a `Scene` and traverses it to draw.

## New files

| File | Purpose |
| --- | --- |
| `Transform.hpp` | Matrix, dirty flag, optional decomposition |
| `Material.hpp` | Texture, shaders, pipeline and descriptors together |
| `MaterialCache.hpp` | Material reuse |
| `Scene.hpp` | Node tree ownership and traversal |
| `GeometryNode.hpp` | Renderable node |

## Changes to existing files

- `Node.hpp` --- reduced to id, transform and hierarchy.
- `Renderer.hpp` --- the flat `m_nodes` vector replaced with a `Scene`; `insert<V>()` split into material creation and
  node insertion; the camera data in `UniformBufferObject` moved toward a `CameraNode`.
- `ModelCache` and `TextureCache` --- unchanged.

## Phases

1. `Transform`, `Material`, `GeometryNode`, `Scene`, and `Node` reduced to its base.
2. `MaterialCache`, `CameraNode`, `LightNode`, and dirty-flag propagation.
