
cmake_minimum_required(VERSION 3.10)
project(OpenGLStarter)

add_subdirectory(external/glfw-3.5.1)
add_subdirectory(external/glm)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(OpenGL REQUIRED)

# compile every source file under src/ into the normal game.
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_executable(OpenGLStarter ${GAME_SOURCES})
target_include_directories(OpenGLStarter PRIVATE external/glad/include external/miniaudio src)
target_compile_definitions(OpenGLStarter PRIVATE GLFW_INCLUDE_NONE)
target_link_libraries(OpenGLStarter PRIVATE glfw OpenGL::GL glm::glm)

# focused combat tests use the same game implementation, but a test main().
set(COMBAT_SOURCES ${GAME_SOURCES})
list(FILTER COMBAT_SOURCES EXCLUDE REGEX "[/\\\\]main[.]cpp$")
add_executable(CombatLogicTests tests/combat_logic_test.cpp ${COMBAT_SOURCES})
target_include_directories(CombatLogicTests PRIVATE external/glad/include external/miniaudio src)
target_compile_definitions(CombatLogicTests PRIVATE GLFW_INCLUDE_NONE TEMPLE_COMBAT_TESTS)
target_link_libraries(CombatLogicTests PRIVATE glfw OpenGL::GL glm::glm)

enable_testing()
add_test(NAME CombatLogicTests COMMAND CombatLogicTests)

# Audio: miniaudio (external/miniaudio/miniaudio.h) needs no extra libraries on Windows.
# On Linux/macOS it needs the dynamic loader, threads and math libraries.
if(NOT WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(OpenGLStarter PRIVATE Threads::Threads ${CMAKE_DL_LIBS} m)
    target_link_libraries(CombatLogicTests PRIVATE Threads::Threads ${CMAKE_DL_LIBS} m)
endif()

# Copy the music and sound effects next to the built .exe so they work
# both from the terminal and when the .exe is opened directly in Explorer.
if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    add_custom_command(TARGET OpenGLStarter POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/assets"
                "$<TARGET_FILE_DIR:OpenGLStarter>/assets"
        COMMENT "Copying game audio")
endif()
