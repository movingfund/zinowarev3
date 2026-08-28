cmake_minimum_required(VERSION 3.20)
project(rbx-external LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.91.5
)
FetchContent_MakeAvailable(imgui)

# Collect all source files from src/
file(GLOB_RECURSE SOURCES
  src/*.cpp
  src/*.hpp
)

# Add the executable
add_executable(rbx-external WIN32 ${SOURCES})

target_include_directories(rbx-external PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)

# Link ImGui library and other dependencies
target_link_libraries(rbx-external PRIVATE
  imgui
  d3d11
  dxgi
  d3dcompiler
  winmm
  gdi32
  user32
  psapi
)

# Add ImGui backend sources (they are not part of the imgui library)
target_sources(rbx-external PRIVATE
  ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)

set_target_properties(rbx-external PROPERTIES
  LINK_FLAGS "/SUBSYSTEM:CONSOLE"
)
