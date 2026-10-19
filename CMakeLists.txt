
cmake_minimum_required(VERSION 3.10)
project(OpenGLStarter)

add_subdirectory(external/glfw-3.5.1)
add_subdirectory(external/glm)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)

add_executable(OpenGLStarter
    main.cpp
)


target_include_directories(OpenGLStarter PRIVATE external/glad/include)
target_link_libraries(OpenGLStarter PRIVATE glfw OpenGL::GL glm::glm)

set(GAME_SOURCES
    src/Renderer2D.cpp
    src/DefensePlayer.cpp
    src/LongRangePlayer.cpp
    src/MeleePlayer.cpp
)

add_executable(CharacterTest
    tests/character_test.cpp
    ${GAME_SOURCES}
)
target_include_directories(CharacterTest PRIVATE external/glad/include include)
target_link_libraries(CharacterTest PRIVATE glfw OpenGL::GL glm::glm)

add_executable(LongRangeTest
    tests/long_range_test.cpp
    ${GAME_SOURCES}
)
target_include_directories(LongRangeTest PRIVATE external/glad/include include)
target_link_libraries(LongRangeTest PRIVATE glfw OpenGL::GL glm::glm)

add_executable(MeleeTest
    tests/melee_test.cpp
    ${GAME_SOURCES}
)
target_include_directories(MeleeTest PRIVATE external/glad/include include)
target_link_libraries(MeleeTest PRIVATE glfw OpenGL::GL glm::glm)
