#pragma once
#include "IGameState.h"
#include <memory>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "../world/Grid.h"
#include "../entities/Enemy.h"
#include "../entities/Tower.h"
#include "../core/WaveManager.h"
#include "../ui/BuildPanel.h"
#include "../ui/PlacementUI.h"
#include "../ui/StatsPanel.h"
#include "../ui/TowerMenuUI.h"
#include "../gameplay/BuildManager.h"
#include "../gameplay/EntityManager.h"
#include "../world/Pathfinder.h"
#include "../renderer/TextRenderer.h"
#include "../gameplay/PlayerStats.h"
#include "../ui/PathRenderer.h"

class SpriteRenderer;
class Texture2D;
class GameStateManager;

class GameplayState : public IGameState {
private:
    GameStateManager& m_stateManager; // ссылка для смены экранов

    int width, height; // ширина высота экрана
    bool m_mousePressedLastFrame; // фиксик
    glm::vec2 m_currentMousePos; // позиция мыши
    std::string m_selectedTowerType; // башня в руке
    Tower* m_selectedTowerOnMap = nullptr; // выбраная башня на карте

    PlayerStats m_playerStats; // статы игрока

    std::string m_currentLevelPath; // путь на левел который щас играет

    /// все указатели мира
    std::unique_ptr<Grid> m_gameGrid;
    std::unique_ptr<Pathfinder> m_pathfinder;
    std::unique_ptr<WaveManager> m_waveManager;
    std::unique_ptr<Buildpanel> m_buildPanel;
    std::unique_ptr<PlacementUI> m_placementUI;
    std::unique_ptr<PathVisualizer> m_pathVisualizer;
    std::unique_ptr<StatsPanel> m_statsPanel;
    std::unique_ptr<TowerMenuUI> m_towerMenuUI;
    std::unique_ptr<BuildManager> m_buildManager;
    std::unique_ptr<EntityManager> m_entityManager;

    // указатели на базы, спавнера, пути, общий путь
    std::vector<SpawnerData> m_spawners;
    std::vector<glm::ivec2> m_bases;
    std::vector<std::vector<glm::ivec2>> m_paths;
    std::vector<glm::ivec2> m_levelPath;

    // текстуры
    std::shared_ptr<Texture2D> m_cellTexture;
    std::shared_ptr<Texture2D> m_grassTexture;
    std::shared_ptr<Texture2D> m_radiusTexture;
    std::shared_ptr<Texture2D> m_particleTexture;

    // указатель на рендеры
    std::shared_ptr<SpriteRenderer> m_renderer;
    TextRenderer* m_textRenderer;

    // Вспомогательные методы, которые относятся только к бою
    bool isKeyJustPressed(GLFWwindow* window, int key);
    bool m_keysProcessed[1024] = { false };

public:
    // конструктор
    GameplayState(GameStateManager& stateManager, int windowWidth, int windowHeight, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer, std::string levelPath);


    // база всего мать его
    void init() override;
    void cleanup() override;
    void processInput(GLFWwindow* window, float dt) override;
    void update(float dt) override;
    void render() override;
    void resize(int width, int height) override;

    // методы для геймплея
    void spawnEnemy(const std::string& type, int spawnerIndex = 0);
    void startNextWave();
    void restartGame();

    int getPathCount() const { return m_paths.size(); }
};