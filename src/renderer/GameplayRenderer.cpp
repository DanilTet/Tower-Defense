#include "GameplayRenderer.h"
#include "SpriteRenderer.h"
#include "TextRenderer.h"
#include "../textures/Texture2D.h"
#include "../resources/ResourceManager.h"
#include "../game/core/ConfigManager.h"
#include "../game/world/GameWorld.h"
#include "../game/ui/BuildPanel.h"
#include "../game/ui/PlacementUI.h"
#include "../game/ui/PathRenderer.h"
#include "../game/ui/statsPanel.h"
#include "../game/ui/TowerMenuUI.h"

GameplayRenderer::GameplayRenderer(std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer)
    : m_renderer(renderer), m_textRenderer(textRenderer)
{
    loadTextures();
}

void GameplayRenderer::loadTextures() {
    // Загрузка атласов и текстур в ResourceManager при необходимости
    ResourceManager::loadTexture("mainAtlas", "res/textures/mainAtlas.png");
    ResourceManager::loadTexture("enemyAtlas", "res/textures/enemyAtlas.png");
    ResourceManager::loadTexture("transitionsAtlas", "res/textures/Frame 1 (2).png");
    ResourceManager::loadTexture("towerTexture", "res/textures/test_sprite.png");
    ResourceManager::loadTexture("grassTexture", "res/textures/spr_grass_02.png");
    ResourceManager::loadTexture("radiusTexture", "res/textures/radius2.png");
    ResourceManager::loadTexture("arrowTexture", "res/textures/pathArrow.png");
    ResourceManager::loadTexture("particleTexture", "res/textures/particle.png");
    ResourceManager::loadTexture("uiBaseTexture", "res/textures/ui_space.png");

    auto wrapNoDelete = [](const std::string& name) {
        Texture2D* tex = ResourceManager::getTexture(name);
        return std::shared_ptr<Texture2D>(tex, [](Texture2D*) {});
    };

    m_mainAtlas = wrapNoDelete("mainAtlas");
    m_enemyAtlas = wrapNoDelete("enemyAtlas");
    m_transitionsAtlas = wrapNoDelete("transitionsAtlas");
    m_radiusTexture = wrapNoDelete("radiusTexture");
    m_particleTexture = wrapNoDelete("particleTexture");
    m_arrowTexture = wrapNoDelete("arrowTexture");
    m_uiBaseTexture = wrapNoDelete("uiBaseTexture");
}

void GameplayRenderer::renderFrame(
    GameWorld& world,
    Buildpanel& buildPanel,
    PlacementUI& placementUI,
    PathVisualizer& pathVisualizer,
    StatsPanel& statsPanel,
    TowerMenuUI& towerMenuUI,
    const std::string& selectedTowerType,
    Tower* selectedTowerOnMap,
    const glm::vec2& mousePos,
    int windowWidth,
    int windowHeight)
{
    if (!m_renderer) return;

    m_renderer->beginBatch();

    // 1. Отрисовка тайлового фона
    if (m_mainAtlas) {
        SpriteUV uvBg = ConfigManager::getUV("main_atlas", "background");
        int bgSize = 64;
        for (int y = 0; y < windowHeight + bgSize; y += bgSize) {
            for (int x = 0; x < windowWidth + bgSize; x += bgSize) {
                m_renderer->drawSprite(m_mainAtlas, glm::vec2(x, y), glm::vec2(bgSize, bgSize), 0.0f, glm::vec3(1.0f, 1.0f, 1.0f), uvBg);
            }
        }
    }

    // 2. Отрисовка игровой сетки
    if (world.grid) {
        world.grid->draw(m_renderer.get(), m_mainAtlas, m_transitionsAtlas, { 1.0f, 1.0f, 1.0f });
    }

    // 3. Стрелочки пути
    for (const auto& path : world.paths) {
        if (world.grid) {
            pathVisualizer.renderPathArrows(
                m_renderer.get(),
                m_arrowTexture,
                path,
                *world.grid
            );
        }
    }

    // 4. Отрисовка сущностей (башни, враги, пули, частицы)
    if (world.entityManager && world.grid) {
        world.entityManager->render(
            m_renderer.get(),
            m_mainAtlas,
            m_enemyAtlas,
            m_radiusTexture,
            m_arrowTexture,
            m_particleTexture,
            *world.grid,
            selectedTowerOnMap
        );
    }

    m_renderer->flush();

    // 5. Отрисовка интерфейса
    if (world.grid) {
        towerMenuUI.render(selectedTowerOnMap, m_renderer.get(), m_textRenderer, m_uiBaseTexture, *world.grid);
    }

    bool hasPath = !world.paths.empty();
    glm::vec2 currentPanelPos = buildPanel.getUIPanelPos(windowWidth, windowHeight);

    if (world.grid) {
        placementUI.renderHologram(
            m_renderer.get(),
            m_mainAtlas,
            m_radiusTexture,
            *world.grid,
            mousePos,
            selectedTowerType,
            world.playerStats,
            currentPanelPos,
            hasPath,
            world.entityManager ? &world.entityManager->getEnemies() : nullptr
        );
    }

    statsPanel.drawStatsPanel(world.playerStats, world.waveManager.get(), m_textRenderer, windowWidth, windowHeight);

    buildPanel.BuildRenderUI(
        world.playerStats,
        m_renderer.get(),
        m_textRenderer,
        m_mainAtlas,
        m_uiBaseTexture,
        windowWidth,
        windowHeight,
        selectedTowerType
    );

    m_renderer->endBatch();
}
