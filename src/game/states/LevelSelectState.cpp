#include "LevelSelectState.h"
#include "GameStateManager.h"
#include "GameplayState.h"
#include "MainMenuState.h"
#include "../resources/ResourceManager.h"
#include "../renderer/SpriteRenderer.h"
#include "../renderer/TextRenderer.h"
#include <GLFW/glfw3.h>
#include "MainMenuState.h"

LevelSelectState::LevelSelectState(GameStateManager& stateManager, int width, int height, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer)
    : m_stateManager(stateManager), m_width(width), m_height(height), m_renderer(renderer), m_textRenderer(textRenderer)
{
    m_uiTexture = std::shared_ptr<Texture2D>(ResourceManager::getTexture("uiBaseTexture"), [](Texture2D*) {});
    m_mousePressedLastFrame = true;
}

void LevelSelectState::init() {
    m_levelButtons.clear();

    // сетка кнопок выбора уровней
    float startX = m_width / 2.0f - 250.0f;
    float startY = 150.0f;

    // кнопка Уровня 1
    LevelButton lvl1;
    lvl1.btn = { glm::vec2(startX, startY), glm::vec2(150.0f, 150.0f), "LEVEL 1", 0 };
    lvl1.levelPath = "res/levels/level_1.json";
    m_levelButtons.push_back(lvl1);

    // кнопка Уровня 2
    LevelButton lvl2;
    lvl2.btn = { glm::vec2(startX + 180.0f, startY), glm::vec2(150.0f, 150.0f), "LEVEL 2", 0 };
    lvl2.levelPath = "res/levels/level_2.json";
    m_levelButtons.push_back(lvl2);

    // кнопка возврата в меню
    m_btnBack = { glm::vec2(m_width / 2.0f - 100.0f, m_height - 100.0f), glm::vec2(200.0f, 50.0f), "BACK TO MENU", 0 };
}

void LevelSelectState::cleanup() {}

bool LevelSelectState::isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) {
    return (point.x >= rectPos.x && point.x <= rectPos.x + rectSize.x &&
        point.y >= rectPos.y && point.y <= rectPos.y + rectSize.y);
}

void LevelSelectState::processInput(GLFWwindow* window, float dt) {
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    glm::vec2 mousePos(mouseX, mouseY);

    // обновляем состояния кнопок
    for (auto& lvlBtn : m_levelButtons) {
        if (isPointInRect(mousePos, lvlBtn.btn.pos, lvlBtn.btn.size)) {
            lvlBtn.btn.state = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) ? 2 : 1;
        }
        else {
            lvlBtn.btn.state = 0;
        }
    }

    if (isPointInRect(mousePos, m_btnBack.pos, m_btnBack.size)) {
        m_btnBack.state = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) ? 2 : 1;
    }
    else {
        m_btnBack.state = 0;
    }

    // обработка клика
    int mouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    if (mouseState == GLFW_PRESS && !m_mousePressedLastFrame) {
        m_mousePressedLastFrame = true;

        for (const auto& lvlBtn : m_levelButtons) {
            if (lvlBtn.btn.state == 2) {
                // ЗАПУСКАЕМ ВЫБРАННЫЙ УРОВЕНЬ
                m_stateManager.setState(std::make_unique<GameplayState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, lvlBtn.levelPath));
                return;
            }
        }

        if (m_btnBack.state == 2) {
            m_stateManager.setState(std::make_unique<MainMenuState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer));
            return;
        }
    }
    else if (mouseState == GLFW_RELEASE) {
        m_mousePressedLastFrame = false;
    }
}

void LevelSelectState::update(float dt) {}

void LevelSelectState::render() {
    // фон
    m_renderer->drawSprite(m_uiTexture, glm::vec2(0.0f), glm::vec2(m_width, m_height), 0.0f, glm::vec3(0.08f, 0.08f, 0.1f));

    m_textRenderer->RenderText("SELECT LEVEL", m_width / 2.0f - 120.0f, 50.0f, 1.5f, glm::vec3(1.0f, 0.8f, 0.2f));

    // отрисовка кнопок уровней
    for (const auto& lvlBtn : m_levelButtons) {
        glm::vec3 color = (lvlBtn.btn.state == 0) ? glm::vec3(0.2f, 0.3f, 0.4f) : (lvlBtn.btn.state == 1) ? glm::vec3(0.3f, 0.4f, 0.5f) : glm::vec3(0.1f, 0.2f, 0.3f);
        m_renderer->drawSprite(m_uiTexture, lvlBtn.btn.pos, lvlBtn.btn.size, 0.0f, color);
        m_textRenderer->RenderText(lvlBtn.btn.text, lvlBtn.btn.pos.x + 35.0f, lvlBtn.btn.pos.y + 65.0f, 1.0f, glm::vec3(1.0f));
    }

    // отрисовка кнопки Назад
    glm::vec3 backColor = (m_btnBack.state == 0) ? glm::vec3(0.3f) : (m_btnBack.state == 1) ? glm::vec3(0.5f) : glm::vec3(0.2f);
    m_renderer->drawSprite(m_uiTexture, m_btnBack.pos, m_btnBack.size, 0.0f, backColor);
    m_textRenderer->RenderText(m_btnBack.text, m_btnBack.pos.x + 30.0f, m_btnBack.pos.y + 15.0f, 1.0f, glm::vec3(0.9f));
}

void LevelSelectState::resize(int width, int height) {
    m_width = width;
    m_height = height;
    init();
}