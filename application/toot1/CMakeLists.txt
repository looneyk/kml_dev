add_executable(toot1
    Application.cpp
    Application.hpp
    main.cpp
)

set_property(TARGET toot1 PROPERTY FOLDER "application")

set_target_properties(toot1 PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(toot1 PRIVATE
    ${DEV_ROOT}/library
    ${Vulkan_INCLUDE_DIRS}
    ${LIBRARY3_DIR}/glfw/include
    ${LIBRARY3_DIR}/glm
    ${LIBRARY3_DIR}/stb
    ${LIBRARY3_DIR}/tinyobjloader
    ${LIBRARY3_DIR}/VulkanMemoryAllocator/include

)

target_link_options(toot1 PRIVATE $<$<CONFIG:Debug>:/NODEFAULTLIB:LIBCMT>)

target_link_libraries(toot1 PRIVATE
    quetzal_common
    quetzal_game
    quetzal_vulkan
    quetzal_scene_graph
    Vulkan::Vulkan
    glfw3_mt
)
