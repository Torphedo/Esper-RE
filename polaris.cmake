cmake_minimum_required(VERSION 3.13)
project(polaris
    VERSION 1.0.0
    LANGUAGES CXX C
)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

# Add ImGui as a library
add_library(imgui STATIC
    lib/imgui/imgui.cpp
    lib/imgui/imgui_demo.cpp
    lib/imgui/imgui_draw.cpp
    lib/imgui/imgui_tables.cpp
    lib/imgui/imgui_widgets.cpp
    lib/imgui/backends/imgui_impl_opengl3.cpp
    lib/imgui/backends/imgui_impl_glfw.cpp
)
set(COREGUI_USE_IMGUI ON)
add_subdirectory(lib/coregui)

# Internal build tool to generate headers for GLSL shaders
add_executable(txt2h "lib/txt2h.c")

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
    src/polaris/validation.cxx
    src/polaris/validation.cxx
    src/polaris/gui/selector_ray.cxx

    src/polaris/alr/alr_dump.cxx
    src/polaris/alr/mkak.cxx
    src/polaris/alr/alr_file.cxx

    src/polaris/gui/polaris.cxx
    src/polaris/gui/alr_imgui.cxx
    src/polaris/gui/alr_assets.cxx
    src/polaris/gui/tool_audio.cxx
    src/polaris/gui/tool_quest.cxx

    src/polaris/gui/alr_editor.cxx
    src/polaris/gui/mapdata_editor.cxx
    src/polaris/gui/mesh_view.cxx
    src/polaris/gui/framebuffer.cxx
    src/polaris/gui/viewport.cxx
    src/polaris/gui/camera.cxx

    src/polaris/util/fileclass.cxx
    src/polaris/util/scope_timer.cxx
    src/polaris/util/imgui_utils.cxx
    src/polaris/util/nfde_wrapper.cxx
    src/polaris/util/utils.cxx

    lib/glad/src/glad.c

    # Adding the GLSL headers here auto-generates them during the build
    ${glsl_headers}
    src/polaris/ma_stx_player.c
    src/polaris/mapdata.cxx
)

target_link_libraries(polaris PRIVATE pd_common coregui imgui nfd)
target_include_directories(polaris PRIVATE ${Esper-RE_SOURCE_DIR}/src/polaris)
