add_executable(model_pattern
    main.cpp
    Page.cpp
    Pages.cpp
    Pane.cpp
    Panes.cpp
    Page.hpp
    Pages.hpp
    Pane.hpp
    Panes.hpp
)

set_property(TARGET model_pattern PROPERTY FOLDER "model")

set_target_properties(model_pattern PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(model_pattern PRIVATE
    ${DEV_ROOT}/library
)

target_link_libraries(model_pattern PRIVATE
    quetzal_common
    quetzal_math
    quetzal_pdf
    quetzal_svg
    quetzal_triangulation
    quetzal_wavefront_obj
    quetzal_xml
)
