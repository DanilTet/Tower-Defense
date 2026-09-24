cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

set(PROJECT_NAME TD)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/res DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

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
    src/game/entities/TowerTargetingSystem.h
    src/game/entities/TowerTargetingSystem.cpp
    src/game/entities/Projectile.h
    src/game/entities/Projectile.cpp
    src/game/entities/CircleCollider.h

    # World (Мир и навигация)
    src/game/world/Grid.h
    src/game/world/Grid.cpp
    src/game/world/Pathfinder.h
    src/game/world/Pathfinder.cpp
    src/game/world/PathService.h
    src/game/world/PathService.cpp
    src/game/world/GameWorld.h
    src/game/world/GameWorld.cpp

    # === СИСТЕМНЫЕ МОДУЛИ ДВИЖКА ===

    # Renderer (Отрисовка)
    src/renderer/SpriteRenderer.h
    src/renderer/SpriteRenderer.cpp
    src/renderer/TextRenderer.h
    src/renderer/TextRenderer.cpp
    src/renderer/GameplayRenderer.h
    src/renderer/GameplayRenderer.cpp

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
    src/audio/AudioEventSystem.h
    src/audio/AudioEventSystem.cpp

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

    src/game/ui/HealthBarRenderer.h
    src/game/ui/HealthBarRenderer.cpp

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
    src/game/states/GameplayInputHandler.h
    src/game/states/GameplayInputHandler.cpp
    src/game/states/GameplayState.h
    src/game/states/GameplayState.cpp

    src/particles/ParticleSystem.cpp
    src/game/states/MainMenuState.cpp
    src/game/states/PauseState.cpp

    src/game/states/GameOverState.cpp
    src/game/states/VictoryState.cpp
    src/game/states/LevelSelectState.cpp
    src/game/core/SaveManager.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/external
    ${CMAKE_CURRENT_SOURCE_DIR}/external/freetype/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/freetype/include/freetype2
    ${CMAKE_CURRENT_SOURCE_DIR}/external/miniaudio
)

target_link_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/external/freetype/lib
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    glfw 
    glad 
    freetype
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/external/freetype/lib/freetype.dll"
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
)