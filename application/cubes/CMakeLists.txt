find_program(GLSLC_EXECUTABLE glslc REQUIRED)

set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)

set(SHADER_PAIRS
    shader_pnt.frag:frag_pnt.spv
    shader_pnt.vert:vert_pnt.spv
    shader_emissive.frag:frag_emissive.spv
    shader_emissive.vert:vert_emissive.spv
)

set(SPV_FILES)
foreach(PAIR ${SHADER_PAIRS})
    string(REPLACE ":" ";" PAIR_LIST ${PAIR})
    list(GET PAIR_LIST 0 SRC)
    list(GET PAIR_LIST 1 OUT)
    add_custom_command(
        OUTPUT ${SHADER_DIR}/${OUT}
        COMMAND ${GLSLC_EXECUTABLE} ${SHADER_DIR}/${SRC} -o ${SHADER_DIR}/${OUT}
        DEPENDS ${SHADER_DIR}/${SRC}
        COMMENT "Compiling ${SRC}"
    )
    list(APPEND SPV_FILES ${SHADER_DIR}/${OUT})
endforeach()

add_custom_target(cubes_shaders DEPENDS ${SPV_FILES})

add_executable(cubes
    Application.cpp
    Application.hpp
    main.cpp
)

set_property(TARGET cubes PROPERTY FOLDER "application")

set_target_properties(cubes PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(cubes PRIVATE
    ${DEV_ROOT}/library
    ${Vulkan_INCLUDE_DIRS}
    ${LIBRARY3_DIR}/glfw/include
    ${LIBRARY3_DIR}/glm
    ${LIBRARY3_DIR}/json/single_include
    ${LIBRARY3_DIR}/stb
    ${LIBRARY3_DIR}/tinyobjloader
    ${LIBRARY3_DIR}/VulkanMemoryAllocator/include
    ${LIBRARY3_DIR}/KTX-Software/include
)

target_link_options(cubes PRIVATE $<$<CONFIG:Debug>:/NODEFAULTLIB:LIBCMT>)

target_link_libraries(cubes PRIVATE
    quetzal_common
    quetzal_game
    quetzal_vulkan
    quetzal_scene_graph
    Vulkan::Vulkan
    glfw3_mt
)

add_dependencies(cubes cubes_shaders)
