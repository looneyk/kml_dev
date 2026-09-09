
  ---
  Current Problems

  Node conflates three separate concerns:
  - Scene graph identity — id, transform, hierarchy
  - Renderable reference — which geometry, which material
  - GPU rendering state — descriptor sets, pipeline layout, pipeline handle

  Renderer::insert<V>() does all of this inline (load geometry, load texture, build descriptors, create pipeline, construct node) with no separation.

  ---
  Proposed Layer Structure

  1. Node — base class only

  Owns identity, transform, and hierarchy. Nothing GPU-specific.
  id: string
  local_transform: matrix_type
  children: vector<Node*>
  parent: Node*
  World transform is computed top-down before draw. A dirty flag avoids recomputing unchanged branches.

  2. Transform

  A small separate struct worth extracting if you want TRS decomposition (translate/rotate/scale) rather than raw matrices. Optional but clean.

  3. Material

  The missing abstraction. Currently the contents of insert<V>(). Owns:
  - texture id (→ TextureCache)
  - vertex/fragment shader paths (currently hardcoded)
  - descriptor sets per swap frame
  - pipeline layout (from ResourceManager)
  - pipeline (from PipelineManager)

  4. MaterialCache

  Same pattern as ModelCache/TextureCache — key by material description, reuse where possible.

  5. GeometryNode : Node

  A concrete Node subclass that is renderable. Holds:
  - geometry id (→ ModelCache)
  - material id (→ MaterialCache)

  Other subclasses would follow: CameraNode, LightNode, etc.

  6. Scene

  Owns the node tree (currently a flat vector<Node> in Renderer). Provides:
  - hierarchical storage (root node + children)
  - lookup by id
  - traversal (for transform propagation, for draw)

  Renderer owns a Scene and drives traversal during draw().

  ---
  Required Infrastructure (new files)

  ┌───────────────────┬──────────────────────────────────────────────────┐
  │       File        │                     Purpose                      │
  ├───────────────────┼──────────────────────────────────────────────────┤
  │ Transform.hpp     │ Wrap matrix, dirty flag, optional TRS            │
  ├───────────────────┼──────────────────────────────────────────────────┤
  │ Material.hpp      │ Group texture + shaders + pipeline + descriptors │
  ├───────────────────┼──────────────────────────────────────────────────┤
  │ MaterialCache.hpp │ Cache/reuse materials                            │
  ├───────────────────┼──────────────────────────────────────────────────┤
  │ Scene.hpp         │ Node tree ownership + traversal                  │
  ├───────────────────┼──────────────────────────────────────────────────┤
  │ GeometryNode.hpp  │ Concrete renderable node                         │
  └───────────────────┴──────────────────────────────────────────────────┘

  ---
  Key Changes to Existing Files

  - Node.hpp — strip to base (id + transform + hierarchy only)
  - Renderer.hpp — replace m_nodes flat vector with Scene; split insert<V>() into material creation step and node insertion step; move UniformBufferObject camera data toward a CameraNode eventually
  - ModelCache/TextureCache — no structural changes needed

  ---
  Immediate vs. Deferred

  The minimum needed to get a proper scene graph working:

  Phase 1 — foundation:
  Transform, Material, GeometryNode, Scene, strip Node to base

  Phase 2 — deferred:
  MaterialCache (can start with ad-hoc material creation), CameraNode, LightNode, dirty-flag propagation optimization

  Does this match where you want to go, or do you want to adjust scope before we start on any of it?
