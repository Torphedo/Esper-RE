cmake_minimum_required(VERSION 3.13)
project(polaris
    VERSION 0.0.1
    LANGUAGES CXX C
)
set(CMAKE_CXX_STANDARD 11)

# Even though its 1 file, this keeps the main source file list shorter
add_library(glad STATIC
    ../ext/glad/src/glad.c
)

# Add ImGui as a library
add_library(imgui STATIC
    ../ext/imgui/imgui.cpp
    ../ext/imgui/imgui_demo.cpp
    ../ext/imgui/imgui_draw.cpp
    ../ext/imgui/imgui_tables.cpp
    ../ext/imgui/imgui_widgets.cpp
    ../ext/imgui/backends/imgui_impl_opengl3.cpp
    ../ext/imgui/backends/imgui_impl_glfw.cpp
)

add_executable(polaris
    src/main.cxx
    src/gui_loop.cxx
    src/polaris_gui.cxx
)

target_link_libraries(polaris PRIVATE glfw glad imgui bobtail)
