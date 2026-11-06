
cmake_minimum_required(VERSION 3.10)
project(OpenGLStarter)

add_subdirectory(external/glfw-3.5.1)
add_subdirectory(external/glm)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(OpenGL REQUIRED)

# PRESERVED: compile every source file under src/ into the normal game.
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_executable(OpenGLStarter ${GAME_SOURCES})
target_include_directories(OpenGLStarter PRIVATE external/glad/include src)
target_compile_definitions(OpenGLStarter PRIVATE GLFW_INCLUDE_NONE)
target_link_libraries(OpenGLStarter PRIVATE glfw OpenGL::GL glm::glm)

# NEW: focused combat tests use the same game implementation, but a test main().
set(COMBAT_SOURCES ${GAME_SOURCES})
list(FILTER COMBAT_SOURCES EXCLUDE REGEX "[/\\\\]main[.]cpp$")
add_executable(CombatLogicTests tests/combat_logic_test.cpp ${COMBAT_SOURCES})
target_include_directories(CombatLogicTests PRIVATE external/glad/include src)
target_compile_definitions(CombatLogicTests PRIVATE GLFW_INCLUDE_NONE TEMPLE_COMBAT_TESTS)
target_link_libraries(CombatLogicTests PRIVATE glfw OpenGL::GL glm::glm)

enable_testing()
add_test(NAME CombatLogicTests COMMAND CombatLogicTests)

# Windows' built-in WAV playback (PlaySound); no extra downloads or libraries.
if(WIN32)
    target_link_libraries(OpenGLStarter PRIVATE winmm)
    target_link_libraries(CombatLogicTests PRIVATE winmm)
endif()

# Copy the original suspense soundtrack next to the built .exe so it works
# both from CMD and when the .exe is opened directly in Explorer.
if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    add_custom_command(TARGET OpenGLStarter POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/assets"
                "$<TARGET_FILE_DIR:OpenGLStarter>/assets"
        COMMENT "Copying temple intro audio")
endif()
