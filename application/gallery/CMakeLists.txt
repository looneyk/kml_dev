add_executable(gallery
    Application.cpp
    Application.hpp
    main.cpp
)

set_property(TARGET gallery PROPERTY FOLDER "application")

set_target_properties(gallery PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(gallery PRIVATE
    ${DEV_ROOT}/library
    ${Vulkan_INCLUDE_DIRS}
    ${LIBRARY3_DIR}/glfw/include
    ${LIBRARY3_DIR}/glm
    ${LIBRARY3_DIR}/stb
    ${LIBRARY3_DIR}/tinyobjloader
    ${LIBRARY3_DIR}/VulkanMemoryAllocator/include

)

target_link_options(gallery PRIVATE $<$<CONFIG:Debug>:/NODEFAULTLIB:LIBCMT>)

target_link_libraries(gallery PRIVATE
    quetzal_common
    quetzal_effects
    quetzal_game
    quetzal_vulkan
    quetzal_scene_graph
    Vulkan::Vulkan
    glfw3_mt
)
