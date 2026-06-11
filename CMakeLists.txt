cmake_minimum_required(VERSION 3.20)
project(RapidDesk VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# 1. BAIXAR O GLFW AUTOMATICAMENTE
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.3.8
)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# 2. BAIXAR O DEAR IMGUI AUTOMATICAMENTE
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.89.9 # Versão estável do ImGui
)
FetchContent_MakeAvailable(imgui)

find_package(OpenGL REQUIRED)

# Inclui os caminhos do nosso projeto e do ImGui baixado para o compilador achar os .h
include_directories(
    src
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

# Lista de arquivos do nosso projeto + arquivos necessários do ImGui
set(SOURCES
    src/ui/main.cpp
    src/ui/main_window.cpp
    
    # Arquivos base do Dear ImGui (Pegando direto da pasta onde o CMake baixou)
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    
    # Extensões do ImGui para conversar com o GLFW e OpenGL3
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(RapidDesk ${SOURCES})

# Vincula as bibliotecas ao executável final
target_link_libraries(RapidDesk PRIVATE glfw OpenGL::GL)