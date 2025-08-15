cmake_minimum_required(VERSION 3.21)
project(NovaEngineSimple LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Fetch dependencies
include(FetchContent)

# glfw
FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4)
# glm
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.1)

FetchContent_MakeAvailable(glfw glm)

# Create executable
add_executable(NovaSimple src/app/SimpleMain.cpp)

# Link libraries
target_link_libraries(NovaSimple PRIVATE glfw glm)

# Include directories
target_include_directories(NovaSimple PRIVATE 
    ${glfw_SOURCE_DIR}/include
    ${glm_SOURCE_DIR}
)

# Set GLFW to not build Wayland
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)