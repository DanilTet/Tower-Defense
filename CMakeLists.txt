cmake_minimum_required(VERSION 4.3.2 FATAL_ERROR)

set(PROJECT_NAME TD)
project(${PROJECT_NAME})

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

add_subdirectory(external/glfw)
add_subdirectory(external/glad)

add_executable(${PROJECT_NAME} 
    src/main.cpp 
    src/shaders/ShaderProgram.h 
    src/shaders/ShaderProgram.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad)