cmake_minimum_required(VERSION 3.10)
project(OpenGLStarter)

add_subdirectory(external/glfw-3.5.1)
add_subdirectory(external/glm)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)

# every .cpp under src/ is compiled automatically.
# After adding a NEW file, run "CMake: Configure" again.
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_executable(OpenGLStarter ${GAME_SOURCES})

target_include_directories(OpenGLStarter PRIVATE external/glad/include src)
target_compile_definitions(OpenGLStarter PRIVATE GLFW_INCLUDE_NONE)
target_link_libraries(OpenGLStarter PRIVATE glfw OpenGL::GL glm::glm)