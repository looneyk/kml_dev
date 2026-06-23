add_executable(lightmatrix
    Animation.cpp
    Animation.hpp
    Application.cpp
    Application.hpp
    Arrangement.hpp
    Color.cpp
    Color.hpp
    Frame.cpp
    Frame.hpp
    Panel.cpp
    Panel.hpp
    Pattern.cpp
    Pattern.hpp
    Pixel.cpp
    Pixel.hpp
    Scene.cpp
    Scene.hpp
    Smoothing.hpp
    Transition.cpp
    Transition.hpp
    types.hpp
    Viewport.cpp
    Viewport.hpp
    main.cpp
)

set_property(TARGET lightmatrix PROPERTY FOLDER "application")

set_target_properties(lightmatrix PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(lightmatrix PRIVATE
    ${DEV_ROOT}/library
    ${Vulkan_INCLUDE_DIRS}
    ${LIBRARY3_DIR}/glfw/include
    ${LIBRARY3_DIR}/glm
    ${LIBRARY3_DIR}/stb
    ${LIBRARY3_DIR}/tinyobjloader
    ${LIBRARY3_DIR}/VulkanMemoryAllocator/include
)

target_link_options(lightmatrix PRIVATE $<$<CONFIG:Debug>:/NODEFAULTLIB:LIBCMT>)

target_link_libraries(lightmatrix PRIVATE
    quetzal_common
    quetzal_game
    quetzal_vulkan
    quetzal_scene_graph
    quetzal_xml
    Vulkan::Vulkan
    glfw3_mt
)
