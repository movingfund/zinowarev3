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

# Explicitly list all source files – NO GLOB to avoid duplicates
set(SOURCES
  src/main.cpp
  src/driver/syscall.cpp
  src/features/esp.cpp
  src/features/aimbot.cpp
  src/features/triggerbot.cpp
  src/sdk/math.cpp
  src/sdk/roblox.cpp
  src/overlay/overlay.cpp
)

add_executable(rbx-external WIN32 ${SOURCES})

target_include_directories(rbx-external PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)

# ImGui core sources (these are NOT part of the imgui library target)
set(IMGUI_CORE
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
)

# ImGui backend sources
set(IMGUI_BACKEND
  ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)

# Add all ImGui sources to the executable
target_sources(rbx-external PRIVATE ${IMGUI_CORE} ${IMGUI_BACKEND})

target_link_libraries(rbx-external PRIVATE
  d3d11
  dxgi
  d3dcompiler
  winmm
  gdi32
  user32
  psapi
)

set_target_properties(rbx-external PROPERTIES
  LINK_FLAGS "/SUBSYSTEM:CONSOLE"
)
