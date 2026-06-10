add_library(scene_graph STATIC
    Scene.cpp
    stb_image_impl.cpp
    tinyobj_impl.cpp
    CameraNode.hpp
    GeometryNode.hpp
    MaterialNode.hpp
    Node.hpp
    NodeVisitor.hpp
    RenderVisitor.hpp
    Scene.hpp
    TransformNode.hpp
)

set_property(TARGET scene_graph PROPERTY FOLDER "library")

target_include_directories(scene_graph PRIVATE
    ${DEV_ROOT}/library
    ${LIBRARY3_DIR}/stb
    ${LIBRARY3_DIR}/tinyobjloader
    ${Vulkan_INCLUDE_DIRS}
)
