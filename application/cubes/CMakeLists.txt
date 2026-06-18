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
