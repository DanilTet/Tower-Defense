cmake_minimum_required(VERSION 4.3.2 FATAL_ERROR)

set(PROJECT_NAME TD)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/res/shaders/vertex_shader.vert
    ${CMAKE_CURRENT_BINARY_DIR}/res/shaders/vertex_shader.vert
    COPYONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/res/shaders/fragment_shader.frag
    ${CMAKE_CURRENT_BINARY_DIR}/res/shaders/fragment_shader.frag
    COPYONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/res/textures/test_sprite.png
    ${CMAKE_CURRENT_BINARY_DIR}/res/textures/test_sprite.png
    COPYONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/res/shaders/text_shader.vert
    ${CMAKE_CURRENT_BINARY_DIR}/res/shaders/text_shader.vert
    COPYONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/res/shaders/text_shader.frag
    ${CMAKE_CURRENT_BINARY_DIR}/res/shaders/text_shader.frag
    COPYONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/res/fonts/Roboto-Regular.ttf
    ${CMAKE_CURRENT_BINARY_DIR}/res/fonts/Roboto-Regular.ttf
    COPYONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/res/levels/level_1.json
    ${CMAKE_CURRENT_BINARY_DIR}/res/levels/level_1.json
    COPYONLY
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/res/sounds DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/res)

add_subdirectory(external/glfw)
add_subdirectory(external/glad)
add_subdirectory(external/freetype)

add_executable(${PROJECT_NAME} 
    src/main.cpp 
    src/stb_image.cpp
    src/shaders/ShaderProgram.h 
    src/shaders/ShaderProgram.cpp
    src/textures/Texture2D.h
    src/textures/Texture2D.cpp
    src/resources/ResourceManager.h
    src/resources/ResourceManager.cpp
    src/renderer/SpriteRenderer.h
    src/renderer/SpriteRenderer.cpp
    src/game/Grid.h
    src/game/Grid.cpp
    src/game/Game.h
    src/game/Game.cpp
    src/game/Enemy.h
    src/game/Enemy.cpp
    src/game/Tower.h
    src/game/Tower.cpp
    src/renderer/TextRenderer.h
    src/renderer/TextRenderer.cpp
    src/game/WaveManager.h
    src/game/WaveManager.cpp
    src/audio/AudioManager.h
    src/audio/AudioManager.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/external
    ${CMAKE_CURRENT_SOURCE_DIR}/external/freetype/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/freetype/include/freetype2
    ${CMAKE_CURRENT_SOURCE_DIR}/external/miniaudio
)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad freetype)