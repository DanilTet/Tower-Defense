#include "VictoryState.h"
#include "GameStateManager.h"
#include "LevelSelectState.h"
#include "../resources/ResourceManager.h"
#include "../renderer/SpriteRenderer.h"
#include "../renderer/TextRenderer.h"
#include <GLFW/glfw3.h>

VictoryState::VictoryState(GameStateManager& stateManager, int width, int height, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer)
    : m_stateManager(stateManager), m_width(width), m_height(height), m_renderer(renderer), m_textRenderer(textRenderer)
{
    m_uiTexture = std::shared_ptr<Texture2D>(ResourceManager::getTexture("uiBaseTexture"), [](Texture2D*) {});
    m_mousePressedLastFrame = true;
}

void VictoryState::init() {
    m_windowSize = glm::vec2(400.0f, 300.0f);
    m_windowPos = glm::vec2((m_width - m_windowSize.x) / 2.0f, (m_height - m_windowSize.y) / 2.0f);

    m_btnNext = { glm::vec2(m_windowPos.x + 50.0f, m_windowPos.y + 120.0f), glm::vec2(300.0f, 50.0f), "SELECT LEVEL", 0 };
    m_btnMenu = { glm::vec2(m_windowPos.x + 50.0f, m_windowPos.y + 200.0f), glm::vec2(300.0f, 50.0f), "MAIN MENU", 0 };
}

void VictoryState::cleanup() {}

bool VictoryState::isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) {
    return (point.x >= rectPos.x && point.x <= rectPos.x + rectSize.x &&
        point.y >= rectPos.y && point.y <= rectPos.y + rectSize.y);
}

void VictoryState::processInput(GLFWwindow* window, float dt) {
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    glm::vec2 mousePos(mouseX, mouseY);

    auto updateButtonState = [&](UIButton& btn) {
        if (isPointInRect(mousePos, btn.pos, btn.size)) {
            btn.state = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) ? 2 : 1;
        }
        else {
            btn.state = 0;
        }
        };

    updateButtonState(m_btnNext);
    updateButtonState(m_btnMenu);

    int mouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    if (mouseState == GLFW_PRESS && !m_mousePressedLastFrame) {
        m_mousePressedLastFrame = true;

        if (m_btnNext.state == 2) {
            // Переход к выбору уровней
            m_stateManager.setState(std::make_unique<LevelSelectState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer));
        }
        else if (m_btnMenu.state == 2) {
            // Выход в главное меню (пока тоже перекинем в LevelSelect, чтобы не усложнять, потом поправишь на MainMenuState)
            m_stateManager.setState(std::make_unique<LevelSelectState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer));
        }
    }
    else if (mouseState == GLFW_RELEASE) {
        m_mousePressedLastFrame = false;
    }
}

void VictoryState::update(float dt) {}

void VictoryState::render() {
    m_renderer->beginBatch(); // открыли пакеть

    // фон победы
    m_renderer->drawSpriteRGBA(m_uiTexture, glm::vec2(0.0f), glm::vec2(m_width, m_height), 0.0f, glm::vec4(0.0f, 0.1f, 0.2f, 0.8f));

    // окошник
    m_renderer->drawSprite(m_uiTexture, m_windowPos, m_windowSize, 0.0f, glm::vec3(0.12f, 0.15f, 0.2f));

    // цвет подложки
    glm::vec3 nextColor;
    if (m_btnNext.state == 1) {
        nextColor = glm::vec3(0.3f, 0.5f, 0.3f);
    }
    else if (m_btnNext.state == 2) {
        nextColor = glm::vec3(0.1f, 0.3f, 0.1f);
    }
    else {
        nextColor = glm::vec3(0.2f, 0.4f, 0.2f);
    }

    glm::vec3 menuColor;
    if (m_btnMenu.state == 1) {
        menuColor = glm::vec3(0.5f);
    }
    else if (m_btnMenu.state == 2) {
        menuColor = glm::vec3(0.2f);
    }
    else {
        menuColor = glm::vec3(0.3f);
    }

    // кидаем подложки кнопок в батч
    m_renderer->drawSprite(m_uiTexture, m_btnNext.pos, m_btnNext.size, 0.0f, nextColor);
    m_renderer->drawSprite(m_uiTexture, m_btnMenu.pos, m_btnMenu.size, 0.0f, menuColor);

    m_renderer->endBatch(); // закрываем пакет

    // заголовок победы
    m_textRenderer->RenderText("VICTORY!", m_windowPos.x + 130.0f, m_windowPos.y + 40.0f, 1.2f, glm::vec3(1.0f, 0.8f, 0.2f));

    // текст на кнопках
    m_textRenderer->RenderText(m_btnNext.text, m_btnNext.pos.x + 75.0f, m_btnNext.pos.y + 15.0f, 1.0f, glm::vec3(0.95f));
    m_textRenderer->RenderText(m_btnMenu.text, m_btnMenu.pos.x + 90.0f, m_btnMenu.pos.y + 15.0f, 1.0f, glm::vec3(0.95f));
}

void VictoryState::resize(int width, int height) {
    m_width = width;
    m_height = height;
    init();
}