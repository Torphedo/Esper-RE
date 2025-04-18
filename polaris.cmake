cmake_minimum_required(VERSION 3.13)
project(polaris
    VERSION 1.0.0
    LANGUAGES CXX C
)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

# Add ImGui as a library
add_library(imgui STATIC
    ext/imgui/imgui.cpp
    ext/imgui/imgui_demo.cpp
    ext/imgui/imgui_draw.cpp
    ext/imgui/imgui_tables.cpp
    ext/imgui/imgui_widgets.cpp
    ext/imgui/backends/imgui_impl_opengl3.cpp
    ext/imgui/backends/imgui_impl_glfw.cpp
)

# Internal build tool to generate headers for GLSL shaders
add_executable(txt2h "ext/txt2h.c")

# Autogenerate string constant headers for each GLSL source file using txt2h.c
set(glsl_headers)
foreach(glsl_src ${GLSL_SOURCES})
    # Replace the file extension with ".h" and put the string in ${file_h}
    set(file_h "${glsl_src}.h")
    message("txt2h ${CMAKE_CURRENT_SOURCE_DIR}/${file_h} ${glsl_src}")
    list(APPEND glsl_headers ${file_h}) # Add to list of generated headers

    add_custom_command(
        OUTPUT  ${CMAKE_CURRENT_SOURCE_DIR}/${file_h}
        COMMAND txt2h ${glsl_src} ${file_h}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endforeach()

add_executable(polaris
    src/polaris/main.cxx
    src/polaris/gui_loop.cxx

    src/polaris/polaris.cxx
    src/polaris/editor_alr.cxx
    src/polaris/alr_texture.cxx
    src/polaris/mapdata.cxx
    src/polaris/pd_mesh.cxx
    src/formats/pd_common.c

    src/polaris/viewport.cxx
    src/polaris/mesh_view.cxx
    src/polaris/camera.cxx
    src/polaris/imgui_utils.cxx

    ext/glad/src/glad.c

    # Adding the GLSL headers here auto-generates them during the build
    ${glsl_headers}
)

target_link_libraries(polaris PRIVATE glfw imgui bobtail nfd)
