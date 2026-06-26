find_program(GLSLC_EXECUTABLE glslc REQUIRED)

set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)

file(GLOB SHADER_SOURCES
    ${SHADER_DIR}/*.vert
    ${SHADER_DIR}/*.frag
)

set(SPV_FILES)
foreach(SRC ${SHADER_SOURCES})
    get_filename_component(NAME ${SRC} NAME_WE)
    get_filename_component(EXT ${SRC} LAST_EXT)
    string(REPLACE "." "" EXT_CLEAN ${EXT})
    set(OUT ${SHADER_DIR}/${NAME}.${EXT_CLEAN}.spv)
    add_custom_command(
        OUTPUT ${OUT}
        COMMAND ${GLSLC_EXECUTABLE} ${SRC} -o ${OUT}
        DEPENDS ${SRC}
        COMMENT "Compiling shader ${NAME}${EXT}"
    )
    list(APPEND SPV_FILES ${OUT})
endforeach()

add_custom_target(quetzal_shaders ALL DEPENDS ${SPV_FILES})
set_property(TARGET quetzal_shaders PROPERTY FOLDER "library")

add_library(quetzal_scene_graph STATIC
    DdsLoader.cpp
    GltfLoader.cpp
    KtxLoader.cpp
    Scene.cpp
    stb_image_impl.cpp
    TextureManager.cpp
    TinyobjReader.cpp
    AssetManager.hpp
    DdsLoader.hpp
    ShaderManager.hpp
    GeometryNode.hpp
    GltfLoader.hpp
    KtxLoader.hpp
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

add_dependencies(quetzal_scene_graph quetzal_shaders)

target_compile_definitions(quetzal_scene_graph PRIVATE GLM_ENABLE_EXPERIMENTAL)

target_include_directories(quetzal_scene_graph PRIVATE
    ${DEV_ROOT}/library
    ${LIBRARY3_DIR}/stb
    ${LIBRARY3_DIR}/tinyobjloader
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(quetzal_scene_graph PRIVATE fastgltf ktx)
