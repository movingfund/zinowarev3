cmake_minimum_required(VERSION 3.20)
project(rbx-external LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── ImGui (fetched automatically) ──────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.91.5
)
FetchContent_MakeAvailable(imgui)

# ── Sources ────────────────────────────────────────────────────────────────
file(GLOB_RECURSE SOURCES
  src/*.cpp
  src/*.hpp
)

# ImGui back-end files we need
set(IMGUI_BACKEND
  ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)

# ── Target ─────────────────────────────────────────────────────────────────
add_executable(rbx-external WIN32 ${SOURCES} ${IMGUI_BACKEND})

target_include_directories(rbx-external PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(rbx-external PRIVATE
  d3d11
  dxgi
  d3dcompiler
  winmm
  gdi32
  user32
  psapi
)

# Keep SUBSYSTEM:CONSOLE so stdout/cerr work
set_target_properties(rbx-external PROPERTIES
  LINK_FLAGS "/SUBSYSTEM:CONSOLE"
)
