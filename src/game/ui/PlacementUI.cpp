#include "PlacementUI.h"
#include "renderer/SpriteRenderer.h"
#include "textures/Texture2D.h"
#include "world/Grid.h"
#include "core/ConfigManager.h"
#include "gameplay/PlayerStats.h"

void PlacementUI::renderHologram(
    SpriteRenderer* renderer,
    std::shared_ptr<Texture2D> mainAtlas,
    std::shared_ptr<Texture2D> radiusTexture,
    const Grid& gameGrid,
    glm::vec2 currentMousePos,
    const std::string& selectedTower,
    const PlayerStats& stats,
    glm::vec2 panelPos,
    bool hasValidPath) {

    // если рука пустая то выходим
    if (selectedTower.empty()) {
        return;
    }
    // если нету пути то не рисуем голограму
    if (!hasValidPath) return;

    // не рисуем башню поверх меню
    if (currentMousePos.x >= panelPos.x && currentMousePos.y >= panelPos.y) {
        return;
    }

    // переводим пиксели мыши в координаты сетки
    glm::ivec2 gridPos = gameGrid.pixelToGrid(currentMousePos);

    // получаем данные выбранной башни
    TowerStats towerstats = ConfigManager::getTowerStats(selectedTower);

    // проверяем, можно ли тут строить
    bool hasMoney = (stats.money >= towerstats.cost);
    bool canBuildHere = gameGrid.canBuildAt(gridPos.x, gridPos.y);

    // задаем цвет голограмы
    glm::vec3 holoColor;

    if (hasMoney && canBuildHere) {
        holoColor = glm::vec3(0.2f, 1.0f, 0.2f); // Зелёная голограмма все ок
    }
    else {
        holoColor = glm::vec3(1.0f, 0.2f, 0.2f); // Красная голограмма проблема
    }

    // вычисляем пиксельные координаты для центрирования
    float cellSize = gameGrid.getCellSize();
    glm::vec2 cellPixelPos = gameGrid.gridToPixel(gridPos.x, gridPos.y);
    glm::vec2 cellCenter = cellPixelPos + glm::vec2(cellSize / 2.0f);

    // включаем режим свечения
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // отрисовка радиуса атаки
    float currentPixelRange = towerstats.range * cellSize;
    glm::vec2 radiusSize(currentPixelRange * 2.0f, currentPixelRange * 2.0f);
    glm::vec2 radiusPos = cellCenter - glm::vec2(currentPixelRange);

    // рисуем радиус цвета голограммы
    renderer->drawSprite(radiusTexture, radiusPos, radiusSize, 0.0f, holoColor);

    // вырезаем башню из атласа
    SpriteUV towerUV = ConfigManager::getUV("main_atlas", "tower_basic");

    // отрисовка самой башни
    renderer->drawSprite(mainAtlas, cellPixelPos, glm::vec2(cellSize), 0.0f, holoColor, towerUV);

    // выключаем свечение
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}