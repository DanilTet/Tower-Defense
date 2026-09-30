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
#include "../game/ui/UICommon.h"
#include "../game/entities/Tower.h"

static std::string getPistonDirectionName(float angle) {
    if (std::abs(angle - 270.0f) < 5.0f || std::abs(angle - (-90.0f)) < 5.0f) return "ВВЕРХ";
    if (std::abs(angle - 0.0f) < 5.0f) return "ВПРАВО";
    if (std::abs(angle - 90.0f) < 5.0f) return "ВНИЗ";
    if (std::abs(angle - 180.0f) < 5.0f) return "ВЛЕВО";
    return "ВВЕРХ";
}

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
    m_whiteTexture = std::shared_ptr<Texture2D>(ResourceManager::getWhiteTexture(), [](Texture2D*) {});
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
    int windowHeight,
    float pistonPlacementAngle)
{
    if (!m_renderer) return;

    m_renderer->beginBatch();

    // 1. Отрисовка сплошного минималистичного фона и нижней панели (Dock)
    if (m_whiteTexture) {
        // Основной фон
        m_renderer->drawSprite(m_whiteTexture, glm::vec2(0.0f, 0.0f), glm::vec2(windowWidth, windowHeight), 0.0f, glm::vec3(0.12f, 0.13f, 0.16f));

        // Полоса нижней панели управления (Dock)
        float bottomBarHeight = Buildpanel::getBottomBarHeight(windowWidth, windowHeight);
        float barY = static_cast<float>(windowHeight) - bottomBarHeight;

        // Разделительная линия сверху нижней панели
        m_renderer->drawSprite(m_whiteTexture, glm::vec2(0.0f, barY), glm::vec2(static_cast<float>(windowWidth), 2.0f), 0.0f, glm::vec3(0.22f, 0.24f, 0.29f));
        // Темная подложка дока
        m_renderer->drawSprite(m_whiteTexture, glm::vec2(0.0f, barY + 2.0f), glm::vec2(static_cast<float>(windowWidth), bottomBarHeight - 2.0f), 0.0f, glm::vec3(0.08f, 0.09f, 0.11f));
    }

    // 2. Отрисовка игровой сетки
    if (world.grid) {
        world.grid->draw(m_renderer.get(), m_whiteTexture, { 1.0f, 1.0f, 1.0f });
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
            world.entityManager ? &world.entityManager->getEnemies() : nullptr,
            pistonPlacementAngle
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

    // 6. Подсказка управления поворотом для поршня (клавиша R)
    if (m_textRenderer && m_whiteTexture) {
        std::string hintText = "";
        if (selectedTowerType == "Piston") {
            hintText = "[R] - Повернуть поршень (Направление: " + getPistonDirectionName(pistonPlacementAngle) + ")";
        }
        else if (selectedTowerOnMap && selectedTowerOnMap->getType() == "Piston") {
            hintText = "[R] - Повернуть поршень (Направление: " + getPistonDirectionName(selectedTowerOnMap->getAngle()) + ")";
        }

        if (!hintText.empty()) {
            float scale = GetUIScale(windowWidth, windowHeight);
            float bottomBarHeight = Buildpanel::getBottomBarHeight(windowWidth, windowHeight);
            float fontScale = 0.52f * scale;
            float textWidth = m_textRenderer->CalculateTextWidth(hintText, fontScale);
            float hintX = (static_cast<float>(windowWidth) - textWidth) * 0.5f;
            float hintY = static_cast<float>(windowHeight) - bottomBarHeight - (28.0f * scale);

            // Подложка бейджа
            m_renderer->drawSprite(m_whiteTexture, glm::vec2(hintX - 16.0f * scale, hintY - 6.0f * scale), glm::vec2(textWidth + 32.0f * scale, 28.0f * scale), 0.0f, glm::vec3(0.08f, 0.09f, 0.12f));
            // Золотистая акцентная черта слева
            m_renderer->drawSprite(m_whiteTexture, glm::vec2(hintX - 16.0f * scale, hintY - 6.0f * scale), glm::vec2(4.0f * scale, 28.0f * scale), 0.0f, glm::vec3(1.0f, 0.85f, 0.2f));
            m_renderer->flush();

            m_textRenderer->RenderText(hintText, hintX, hintY + 1.0f * scale, fontScale, glm::vec3(1.0f, 0.92f, 0.4f));
        }
    }

    m_renderer->endBatch();
}
