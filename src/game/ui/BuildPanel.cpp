#include "BuildPanel.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "renderer/SpriteRenderer.h"
#include "renderer/TextRenderer.h"
#include "textures/Texture2D.h"
#include "core/ConfigManager.h"
#include "gameplay/PlayerStats.h"

glm::vec2 Buildpanel::getUIPanelPos(int windowWidth, int windowHeight) const {
	return glm::vec2(windowWidth - UI_PANEL_WIDTH, windowHeight - UI_PANEL_HEIGHT);
}

glm::vec2 Buildpanel::getTowerIconPos(int index, int windowWidth, int windowHeight) const {
    glm::vec2 panelPos = getUIPanelPos(windowWidth, windowHeight);
    return glm::vec2(
        panelPos.x + UI_OFFSET_X + (index * UI_ICON_PADDING),
        panelPos.y + UI_OFFSET_Y
    );
}

void Buildpanel::BuildRenderUI(
    const PlayerStats& playerStats,
    SpriteRenderer* renderer,
    TextRenderer* textRenderer,
    std::shared_ptr<Texture2D> cellTexture,
    int windowWidth,
    int windowHeight,
    TowerType selectedTower)
{
    glm::vec2 panelPos = getUIPanelPos(windowWidth, windowHeight); // позиция менюшки

    // рисуем фон панели
    renderer->drawSprite(cellTexture, panelPos, glm::vec2(UI_PANEL_WIDTH, UI_PANEL_HEIGHT), 0.0f, glm::vec3(0.1f, 0.1f, 0.1f));
    // список башен для отрисовки
    std::vector<TowerType> availableTowers = { TowerType::Basic, TowerType::Sniper, TowerType::Cannon };

    // рисуем каждую башню
    for (size_t i = 0; i < availableTowers.size(); ++i) {
        TowerType currentType = availableTowers[i];
        TowerStats towerstats = Tower::getStatsfromTowerType(currentType);

        // позиция иконок
        glm::vec2 iconPos = getTowerIconPos(i, windowWidth, windowHeight);

        bool canAfford = (playerStats.money >= towerstats.cost); // определяем хватает ли денег

        // цвет башни
        glm::vec3 towerColor(1.0f);
        if (currentType == TowerType::Basic) towerColor = glm::vec3(0.5f, 0.5f, 0.5f);
        else if (currentType == TowerType::Sniper) towerColor = glm::vec3(0.8f, 0.2f, 0.2f);
        else if (currentType == TowerType::Cannon) towerColor = glm::vec3(0.2f, 0.2f, 0.8f);

        // если денег не достаточно то делаем башню серой
        glm::vec3 drawColor;

        if (canAfford) {
            drawColor = towerColor;
        }
        else {
            drawColor = glm::vec3(0.2f, 0.2f, 0.2f);
        }

        // выделяем желтой рамкой выбраную башню
        if (selectedTower == currentType) {
            renderer->drawSprite(cellTexture, iconPos - glm::vec2(4.0f), glm::vec2(UI_ICON_SIZE + 8.0f), 0.0f, glm::vec3(1.0f, 1.0f, 0.0f));
        }

        // рисуем иконку башни
        renderer->drawSprite(cellTexture, iconPos, glm::vec2(UI_ICON_SIZE), 0.0f, drawColor);

        // текст цены
        std::string towerName;

        if (currentType == TowerType::Basic) {
            towerName = "Basic";
        }
        else if (currentType == TowerType::Sniper) {
            towerName = "Sniper";
        }
        else if (currentType == TowerType::Cannon) {
            towerName = "Cannon";
        }

        // если нету денег текст красный и зеленый если есть
        glm::vec3 textColor;

        if (canAfford) {
            textColor = glm::vec3(0.4f, 1.0f, 0.4f);
        }
        else {
            textColor = glm::vec3(1.0f, 0.3f, 0.3f);
        }

        textRenderer->RenderText(towerName + ": $" + std::to_string(towerstats.cost),
            iconPos.x - 5.0f, iconPos.y + UI_ICON_SIZE + 10.0f, 0.5f, textColor);

    }
}


bool Buildpanel::checkClick(float mouseX, float mouseY, int windowWidth, int windowHeight, TowerType& selectedTower) {
    
    //проверка клика по Ui
    glm::vec2 panelPos = getUIPanelPos(windowWidth, windowHeight);


    // если мышка вне прямоугольника панели
    if (mouseX < panelPos.x || mouseY < panelPos.y){
        return false;
    }

    // проверяем, по какой именно башне кликнули
    for (int i = 0; i < 3; ++i) {
        glm::vec2 iconPos = getTowerIconPos(i, windowWidth, windowHeight);

        if (mouseX >= iconPos.x && mouseX <= iconPos.x + Buildpanel::UI_ICON_SIZE &&
            mouseY >= iconPos.y && mouseY <= iconPos.y + Buildpanel::UI_ICON_SIZE) {

            // меняем выбранную башню
            if (i == 0) selectedTower = TowerType::Basic;
            else if (i == 1) selectedTower = TowerType::Sniper;
            else if (i == 2) selectedTower = TowerType::Cannon;

            //AudioManager::playSound("res/sounds/build.wav", 0.5f); // Звук клика по кнопке
        }
    }

    return true;
}