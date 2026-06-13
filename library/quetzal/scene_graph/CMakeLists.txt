add_library(quetzal_scene_graph STATIC
    Scene.cpp
    TextureManager.cpp
    TinyobjReader.cpp
    AssetManager.hpp
    GeometryNode.hpp
    IMeshReader.hpp
    LightNode.hpp
    MaterialNode.hpp
    MeshData.hpp
    MeshLoader.hpp
    Node.hpp
    NodeVisitor.hpp
    RenderVisitor.hpp
    Scene.hpp
    TextureManager.hpp
    TinyobjReader.hpp
    TransformNode.hpp
)

set_property(TARGET quetzal_scene_graph PROPERTY FOLDER "library")

target_include_directories(quetzal_scene_graph PRIVATE
    ${DEV_ROOT}/library
    ${LIBRARY3_DIR}/stb
    ${LIBRARY3_DIR}/tinyobjloader
    ${Vulkan_INCLUDE_DIRS}
)
