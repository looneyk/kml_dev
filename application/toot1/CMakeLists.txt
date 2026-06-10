add_executable(toot1
    Application.cpp
    Application.hpp
    main.cpp
)

set_property(TARGET toot1 PROPERTY FOLDER "application")

target_include_directories(toot1 PRIVATE
    ${DEV_ROOT}/library
    ${Vulkan_INCLUDE_DIRS}
    ${LIBRARY3_DIR}/glfw/include
    ${LIBRARY3_DIR}/glm
    ${LIBRARY3_DIR}/stb
    ${LIBRARY3_DIR}/tinyobjloader
    ${LIBRARY3_DIR}/VulkanMemoryAllocator/include
    ${LIBRARY3_DIR}/KTX-Software/include
)

target_link_options(toot1 PRIVATE $<$<CONFIG:Debug>:/NODEFAULTLIB:LIBCMT>)

target_link_libraries(toot1 PRIVATE
    common
    game
    quetzal_vulkan
    scene_graph
    Vulkan::Vulkan
    glfw3_mt
)
