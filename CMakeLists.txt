cmake_minimum_required(VERSION 3.21)
project(NovaEngine LANGUAGES CXX)


# Ensure volk is used as the Vulkan loader across all TUs
add_compile_definitions(VK_NO_PROTOTYPES)
option(NOVA_BUILD_EDITOR "Build editor" ON)
option(NOVA_FETCH_DEPS "Fetch third-party deps" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (NOVA_FETCH_DEPS)
  include(FetchContent)

  # glfw
  FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4)
  # imgui docking
  FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG v1.91.0-docking)
  # volk
  FetchContent_Declare(volk GIT_REPOSITORY https://github.com/zeux/volk.git GIT_TAG 1.3.270)
  # glm
  FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.1)
  # lua
  FetchContent_Declare(lua GIT_REPOSITORY https://github.com/lua/lua.git GIT_TAG v5.4.7)
  # Vulkan-Headers
  FetchContent_Declare(vulkanheaders GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git GIT_TAG v1.3.283)
  # glad
  FetchContent_Declare(glad GIT_REPOSITORY https://github.com/Dav1dde/glad.git GIT_TAG v0.1.36)

  FetchContent_MakeAvailable(glfw imgui glm lua glad)
endif()


# Populate volk (sources only) and Vulkan-Headers (headers only)
FetchContent_GetProperties(volk)
if (NOT volk_POPULATED)
  FetchContent_Populate(volk)
endif()
FetchContent_GetProperties(vulkanheaders)
if (NOT vulkanheaders_POPULATED)
  FetchContent_Populate(vulkanheaders)
endif()

# ---- Shaders build (requires Vulkan SDK: glslc) ----
set(SHADER_SRC_DIR ${CMAKE_SOURCE_DIR}/assets/shaders)
set(SHADER_OUT_DIR ${CMAKE_SOURCE_DIR}/assets/shaders) # keep in-source for simplicity

find_program(GLSLC glslc HINTS ENV VULKAN_SDK PATH_SUFFIXES Bin)
if (NOT GLSLC)
  message(WARNING "glslc not found. Make sure the Vulkan SDK is installed and VULKAN_SDK is set. "
                  "Shaders will not be compiled automatically.")
else()
  add_custom_command(
    OUTPUT ${SHADER_OUT_DIR}/pbr.vert.spv
    COMMAND ${GLSLC} -fshader-stage=vert -O -o ${SHADER_OUT_DIR}/pbr.vert.spv ${SHADER_SRC_DIR}/pbr.vert.glsl
    DEPENDS ${SHADER_SRC_DIR}/pbr.vert.glsl
    COMMENT "Compiling pbr.vert.glsl -> pbr.vert.spv"
  )
  add_custom_command(
    OUTPUT ${SHADER_OUT_DIR}/pbr.frag.spv
    COMMAND ${GLSLC} -fshader-stage=frag -O -o ${SHADER_OUT_DIR}/pbr.frag.spv ${SHADER_SRC_DIR}/pbr.frag.glsl
    DEPENDS ${SHADER_SRC_DIR}/pbr.frag.glsl
    COMMENT "Compiling pbr.frag.glsl -> pbr.frag.spv"
  )
  add_custom_target(Shaders ALL DEPENDS ${SHADER_OUT_DIR}/pbr.vert.spv ${SHADER_OUT_DIR}/pbr.frag.spv)
endif()

add_library(NovaEngine STATIC
  ${volk_SOURCE_DIR}/volk.c
  src/engine/renderer/glad.c
  src/engine/core/Log.cpp
  src/engine/core/Time.cpp
  src/engine/ecs/ECS.h
  src/engine/ecs/Components.h
  src/engine/input/Input.cpp
  src/engine/scripting/LuaVM.cpp
  src/engine/renderer/OpenGLRenderer.cpp
  src/engine/renderer/vk/VulkanRenderer.cpp
  src/engine/renderer/vk/VulkanHelpers.cpp
  src/engine/editor/Editor.cpp
  src/engine/editor/AICommandPalette.cpp
  src/engine/ai/Memory.cpp
  src/engine/ai/Proposals.cpp
)
target_include_directories(NovaEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/engine)

target_include_directories(NovaEngine PUBLIC src
  PRIVATE ${volk_SOURCE_DIR}
  PRIVATE ${vulkanheaders_SOURCE_DIR}/include
  PRIVATE ${imgui_SOURCE_DIR}
  PRIVATE ${imgui_SOURCE_DIR}/backends
  PRIVATE ${glfw_SOURCE_DIR}/include
  PRIVATE ${lua_SOURCE_DIR}
)
target_compile_definitions(NovaEngine PRIVATE IMGUI_DEFINE_MATH_OPERATORS)

target_link_libraries(NovaEngine
  PRIVATE glfw
  PUBLIC glm
)
# imgui sources
target_sources(NovaEngine PRIVATE
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)

# Lua
target_sources(NovaEngine PRIVATE
  ${lua_SOURCE_DIR}/lapi.c ${lua_SOURCE_DIR}/lcode.c ${lua_SOURCE_DIR}/lctype.c
  ${lua_SOURCE_DIR}/ldebug.c ${lua_SOURCE_DIR}/ldo.c ${lua_SOURCE_DIR}/ldump.c
  ${lua_SOURCE_DIR}/lfunc.c ${lua_SOURCE_DIR}/lgc.c ${lua_SOURCE_DIR}/llex.c
  ${lua_SOURCE_DIR}/lmem.c ${lua_SOURCE_DIR}/lobject.c ${lua_SOURCE_DIR}/lopcodes.c
  ${lua_SOURCE_DIR}/lparser.c ${lua_SOURCE_DIR}/lstate.c ${lua_SOURCE_DIR}/lstring.c
  ${lua_SOURCE_DIR}/ltable.c ${lua_SOURCE_DIR}/ltm.c ${lua_SOURCE_DIR}/lundump.c
  ${lua_SOURCE_DIR}/lvm.c ${lua_SOURCE_DIR}/lzio.c ${lua_SOURCE_DIR}/lauxlib.c
  ${lua_SOURCE_DIR}/lbaselib.c ${lua_SOURCE_DIR}/lcorolib.c ${lua_SOURCE_DIR}/ldblib.c
  ${lua_SOURCE_DIR}/liolib.c ${lua_SOURCE_DIR}/lmathlib.c ${lua_SOURCE_DIR}/loslib.c ${lua_SOURCE_DIR}/lstrlib.c
  ${lua_SOURCE_DIR}/ltablib.c ${lua_SOURCE_DIR}/lutf8lib.c ${lua_SOURCE_DIR}/loadlib.c
  ${lua_SOURCE_DIR}/linit.c
)

if (WIN32)
  target_compile_definitions(NovaEngine PRIVATE NOMINMAX VK_USE_PLATFORM_WIN32_KHR)
endif()

if (NOVA_BUILD_EDITOR)
  add_executable(NovaEditor src/app/EditorMain.cpp)
  target_link_libraries(NovaEditor PRIVATE NovaEngine)
  add_custom_command(TARGET NovaEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:NovaEditor>/assets)
  add_custom_command(TARGET NovaEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${CMAKE_SOURCE_DIR}/Scripts $<TARGET_FILE_DIR:NovaEditor>/Scripts)
  add_custom_command(TARGET NovaEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${CMAKE_SOURCE_DIR}/Samples $<TARGET_FILE_DIR:NovaEditor>/Samples)
  if (TARGET Shaders)
    add_dependencies(NovaEditor Shaders)
  endif()
endif()

# Added by apply_best_fix.ps1 (20250808021042)
target_include_directories(NovaEngine
    PRIVATE
        /src/engine
)

