#include "BuildPanel.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "renderer/SpriteRenderer.h"
#include "renderer/TextRenderer.h"
#include "textures/Texture2D.h"
#include "core/ConfigManager.h"
#include "gameplay/PlayerStats.h"
#include "UICommon.h"

void Buildpanel::initPanelData() {
    // получаем типы башен один раз и сохраняем в кеш
    m_cachedTowers = ConfigManager::getAllTowerTypes();
    // рассичивыем базовую ширину
    m_cachedPanelWidth = calculatePanelWidth(m_cachedTowers.size());
}

glm::vec2 Buildpanel::getUIPanelPos(int windowWidth, int windowHeight) const {
    float scale = GetUIScale(windowWidth, windowHeight);
    glm::vec2 panelSize(m_cachedPanelWidth * scale, UI_PANEL_HEIGHT * scale);
    glm::vec2 offset(20.0f * scale, 20.0f * scale);

	return CalculateAnchorPosition(UIAnchor::BottomRight, offset, panelSize, windowWidth, windowHeight);
}

glm::vec2 Buildpanel::getTowerIconPos(int index, int windowWidth, int windowHeight) const {
    float scale = GetUIScale(windowWidth, windowHeight);
    glm::vec2 panelPos = getUIPanelPos(windowWidth, windowHeight);
    return glm::vec2(
        panelPos.x + (UI_OFFSET_X * scale) + (index * UI_ICON_PADDING * scale),
        panelPos.y + (UI_OFFSET_Y * scale)
    );
}

void Buildpanel::BuildRenderUI(
    const PlayerStats& playerStats,
    SpriteRenderer* renderer,
    TextRenderer* textRenderer,
    std::shared_ptr<Texture2D> cellTexture,
    int windowWidth,
    int windowHeight,
    const std::string& selectedTower)
{
    float scale = GetUIScale(windowWidth, windowHeight);
    glm::vec2 panelPos = getUIPanelPos(windowWidth, windowHeight); // позиция менюшки

    // рисуем фон панели
    renderer->drawSprite(cellTexture, panelPos, glm::vec2(m_cachedPanelWidth * scale, UI_PANEL_HEIGHT * scale), 0.0f, glm::vec3(0.1f, 0.1f, 0.1f));

    // рисуем каждую башню
    for (size_t i = 0; i < m_cachedTowers.size(); ++i) {
        std::string currentType = m_cachedTowers[i];
        TowerStats towerstats = ConfigManager::getTowerStats(currentType);

        // позиция иконок
        glm::vec2 iconPos = getTowerIconPos(i, windowWidth, windowHeight);

        bool canAfford = (playerStats.money >= towerstats.cost); // определяем хватает ли денег

        // цвет башни
        glm::vec3 drawColor;

        if (canAfford) {
            drawColor = towerstats.color;
        }
        else {
            drawColor = glm::vec3(0.2f, 0.2f, 0.2f);
        }

        // выделяем желтой рамкой выбраную башню
        if (selectedTower == currentType) {
            renderer->drawSprite(cellTexture, iconPos - glm::vec2(4.0f * scale), glm::vec2((UI_ICON_SIZE + 8.0f) * scale), 0.0f, glm::vec3(1.0f, 1.0f, 0.0f));
        }

        // рисуем иконку башни
        renderer->drawSprite(cellTexture, iconPos, glm::vec2(UI_ICON_SIZE * scale), 0.0f, drawColor);
    }

    renderer->flush(); //рисуем батч на экран

    for (size_t i = 0; i < m_cachedTowers.size(); ++i) {
        std::string currentType = m_cachedTowers[i];
        TowerStats towerstats = ConfigManager::getTowerStats(currentType);
        glm::vec2 iconPos = getTowerIconPos(i, windowWidth, windowHeight);

        bool canAfford = (playerStats.money >= towerstats.cost);

        glm::vec3 textColor;

        if (canAfford) {
            textColor = glm::vec3(0.4f, 1.0f, 0.4f);
        }
        else {
            textColor = glm::vec3(1.0f, 0.3f, 0.3f);
        }

        textRenderer->RenderText(currentType + ": $" + std::to_string(towerstats.cost),
            iconPos.x - (5.0f * scale), iconPos.y + (UI_ICON_SIZE * scale) + (10.0f * scale), 0.5f * scale, textColor);
    }
}


bool Buildpanel::checkClick(float mouseX, float mouseY, int windowWidth, int windowHeight, std::string& selectedTower) {
    
    //проверка клика по Ui
    float scale = GetUIScale(windowWidth, windowHeight);
    glm::vec2 panelPos = getUIPanelPos(windowWidth, windowHeight);

    // если мышка вне прямоугольника панели
    if (mouseX < panelPos.x || mouseX > panelPos.x + (m_cachedPanelWidth * scale) ||
        mouseY < panelPos.y || mouseY > panelPos.y + (UI_PANEL_HEIGHT * scale)) {
        return false;
    }

    // проверяем, по какой именно башне кликнули
    for (int i = 0; i < m_cachedTowers.size(); ++i) {
        glm::vec2 iconPos = getTowerIconPos(i, windowWidth, windowHeight);
        float iconSizeScaled = UI_ICON_SIZE * scale;

        if (mouseX >= iconPos.x && mouseX <= iconPos.x + iconSizeScaled &&
            mouseY >= iconPos.y && mouseY <= iconPos.y + iconSizeScaled) {
            // записуем имя башни из списка
            selectedTower = m_cachedTowers[i];
            return true;
            //AudioManager::playSound("res/sounds/build.wav", 0.5f); // Звук клика по кнопке
        }
    }

    return true;
}