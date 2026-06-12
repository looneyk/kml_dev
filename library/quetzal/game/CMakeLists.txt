add_library(quetzal_game STATIC
    Application.cpp
    Camera.cpp
    Window.cpp
    Application.hpp
    Camera.hpp
    types.hpp
    Window.hpp
)

set_property(TARGET quetzal_game PROPERTY FOLDER "library")

target_include_directories(quetzal_game PRIVATE
    ${DEV_ROOT}/library
    ${LIBRARY3_DIR}
    ${LIBRARY3_DIR}/glfw/include
    ${LIBRARY3_DIR}/glm
)
