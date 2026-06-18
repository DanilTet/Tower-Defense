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

include_directories(src/game)

add_executable(${PROJECT_NAME} 
    # Главные файлы
    src/main.cpp 
    src/stb_image.cpp

    # === ИГРОВАЯ ЛОГИКА (Новая структура) ===

    # Core (Ядро и менеджеры)
    src/game/core/Game.h
    src/game/core/Game.cpp
    src/game/core/ConfigManager.h
    src/game/core/ConfigManager.cpp
    src/game/core/WaveManager.h
    src/game/core/WaveManager.cpp

    # Entities (Игровые сущности)
    src/game/entities/Enemy.h
    src/game/entities/Enemy.cpp
    src/game/entities/Tower.h
    src/game/entities/Tower.cpp
    src/game/entities/Projectile.h
    src/game/entities/Projectile.cpp
    src/game/entities/CircleCollider.h

    # World (Мир и навигация)
    src/game/world/Grid.h
    src/game/world/Grid.cpp
    src/game/world/Pathfinder.h
    src/game/world/Pathfinder.cpp

    # === СИСТЕМНЫЕ МОДУЛИ ДВИЖКА ===

    # Renderer (Отрисовка)
    src/renderer/SpriteRenderer.h
    src/renderer/SpriteRenderer.cpp
    src/renderer/TextRenderer.h
    src/renderer/TextRenderer.cpp

    # Resources (Ресурсы)
    src/resources/ResourceManager.h
    src/resources/ResourceManager.cpp

    # Shaders (Шейдеры)
    src/shaders/ShaderProgram.h
    src/shaders/ShaderProgram.cpp

    # Textures (Текстуры)
    src/textures/Texture2D.h
    src/textures/Texture2D.cpp

    # Audio (Аудио)
    src/audio/AudioManager.h
    src/audio/AudioManager.cpp

    # панелька для строительства
    src/game/ui/BuildPanel.h
    src/game/ui/BuildPanel.cpp

    src/game/ui/PlacementUI.h
    src/game/ui/PlacementUI.cpp

    src/game/ui/PathRenderer.h
    src/game/ui/PathRenderer.cpp

    src/game/ui/statsPanel.h
    src/game/ui/statsPanel.cpp

    src/game/ui/TowerMenuUI.h
    src/game/ui/TowerMenuUI.cpp

    src/game/gameplay/BuildManager.h
    src/game/gameplay/BuildManager.cpp

    src/game/gameplay/EntityManager.h
    src/game/gameplay/EntityManager.cpp

    src/game/gameplay/PlayerStats.h

    src/game/core/LevelManager.h
    src/game/core/LevelManager.cpp

    src/game/core/EventBus.cpp
    src/game/core/EventBus.h

    src/game/states/IGameState.h
    src/game/states/GameStateManager.h
    src/game/states/GameStateManager.cpp
    src/game/states/GameplayState.cpp

    src/particles/ParticleSystem.cpp
    src/game/states/MainMenuState.cpp
    src/game/states/PauseState.cpp

    src/game/states/GameOverState.cpp
    src/game/states/VictoryState.cpp
    src/game/states/LevelSelectState.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/external
    ${CMAKE_CURRENT_SOURCE_DIR}/external/freetype/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/freetype/include/freetype2
    ${CMAKE_CURRENT_SOURCE_DIR}/external/miniaudio
)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad debug freetype optimized freetyped)