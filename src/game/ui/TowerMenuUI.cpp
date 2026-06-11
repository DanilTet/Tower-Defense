#include "TowerMenuUI.h"
#include "../entities/Tower.h"
#include "../renderer/SpriteRenderer.h"
#include "../renderer/TextRenderer.h"
#include "../world/Grid.h"
#include "../gameplay/BuildManager.h"
#include "../gameplay/PlayerStats.h"
#include <iostream>
#include <string>

bool TowerMenuUI::processClick(
    double mouseX, double mouseY,
    Tower* selectedTower,
    BuildManager* buildManager,
    PlayerStats& stats,
    EntityManager& entityManager,
    Grid& gameGrid,
    Pathfinder& pathfinder,
    const std::vector<glm::ivec2>& spawners,
    const std::vector<glm::ivec2>& bases,
    std::vector<std::vector<glm::ivec2>>& paths,
    std::vector<glm::ivec2>& levelPath,
    Tower*& outSelectedTower)
{
    if (!selectedTower) return false;

    float cellSize = gameGrid.getCellSize();
    glm::vec2 towerPos = gameGrid.gridToPixel(selectedTower->getGridX(), selectedTower->getGridY());

    // размеры и позиции кнопок
    glm::vec2 btnSize(cellSize * 0.5f, cellSize * 0.5f);
    glm::vec2 upgPos = towerPos + glm::vec2(0.0f, -btnSize.y - 5.0f);
    glm::vec2 sellPos = towerPos + glm::vec2(cellSize * 0.5f, -btnSize.y - 5.0f);

    // клик по кнопке UPGRADE?
    if (mouseX >= upgPos.x && mouseX <= upgPos.x + btnSize.x && mouseY >= upgPos.y && mouseY <= upgPos.y + btnSize.y) {
        int cost = selectedTower->getUpgradeCost();
        if (selectedTower->upgrade(stats.money)) {
            std::cout << "Tower upgraded!" << std::endl;
        }
        else {
            std::cout << "Not enough money for upgrade. Need: $" << cost << std::endl;
        }
        return true; // клик обработан
    }

    // клик по кнопке SELL?
    if (mouseX >= sellPos.x && mouseX <= sellPos.x + btnSize.x && mouseY >= sellPos.y && mouseY <= sellPos.y + btnSize.y) {
        buildManager->sellTower(selectedTower, stats, entityManager, gameGrid, pathfinder, spawners, bases, paths, levelPath);
        outSelectedTower = nullptr; // снимаем выделение после продажи
        return true; // клик обработан
    }

    return false; // клик был мимо кнопок меню
}

void TowerMenuUI::render(
    Tower* selectedTower,
    SpriteRenderer* renderer,
    TextRenderer* textRenderer,
    std::shared_ptr<Texture2D> cellTexture,
    const Grid& gameGrid)
{
    if (!selectedTower) return;

    float cellSize = gameGrid.getCellSize();
    glm::vec2 towerPos = gameGrid.gridToPixel(selectedTower->getGridX(), selectedTower->getGridY());

    glm::vec2 btnSize(cellSize * 0.5f, cellSize * 0.5f);
    glm::vec2 upgPos = towerPos + glm::vec2(0.0f, -btnSize.y - 5.0f);
    glm::vec2 sellPos = towerPos + glm::vec2(cellSize * 0.5f, -btnSize.y - 5.0f);

    // келеная заглушка
    renderer->drawSprite(cellTexture, upgPos, btnSize, 0.0f, glm::vec3(0.2f, 0.8f, 0.2f));

    int cost = selectedTower->getUpgradeCost();
    if (cost == 0) {
        textRenderer->RenderText("MAX", upgPos.x + 5.0f, upgPos.y + 10.0f, 0.5f, glm::vec3(1.0f));
    }
    else {
        textRenderer->RenderText("$" + std::to_string(cost), upgPos.x + 2.0f, upgPos.y + 10.0f, 0.5f, glm::vec3(1.0f));
    }

    // красная заглушкак
    renderer->drawSprite(cellTexture, sellPos, btnSize, 0.0f, glm::vec3(0.8f, 0.2f, 0.2f));
    textRenderer->RenderText("SELL", sellPos.x + 2.0f, sellPos.y + 10.0f, 0.5f, glm::vec3(1.0f));
}