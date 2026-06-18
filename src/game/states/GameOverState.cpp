#include "GameOverState.h"
#include "GameStateManager.h"
#include "GameplayState.h"
#include "LevelSelectState.h"
#include "../resources/ResourceManager.h"
#include "../renderer/SpriteRenderer.h"
#include "../renderer/TextRenderer.h"
#include <GLFW/glfw3.h>

GameOverState::GameOverState(GameStateManager& stateManager, int width, int height, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer, std::string levelPath)
    : m_stateManager(stateManager), m_width(width), m_height(height), m_renderer(renderer), m_textRenderer(textRenderer), m_levelPath(levelPath)
{
    // четенькая текстурка
    m_uiTexture = std::shared_ptr<Texture2D>(ResourceManager::getTexture("uiBaseTexture"), [](Texture2D*) {});

    m_mousePressedLastFrame = true;
}

void GameOverState::init() {
    m_windowSize = glm::vec2(400.0f, 300.0f); // размер окошка
    // бахаем по центру
    m_windowPos = glm::vec2((m_width - m_windowSize.x) / 2.0f, (m_height - m_windowSize.y) / 2.0f);
    // кнопки
    m_btnRetry = { glm::vec2(m_windowPos.x + 50.0f, m_windowPos.y + 120.0f), glm::vec2(300.0f, 50.0f), "TRY AGAIN", 0 };
    m_btnMenu = { glm::vec2(m_windowPos.x + 50.0f, m_windowPos.y + 200.0f), glm::vec2(300.0f, 50.0f), "EXIT TO MENU", 0 };
}

void GameOverState::cleanup() {}

bool GameOverState::isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) {
    return (point.x >= rectPos.x && point.x <= rectPos.x + rectSize.x &&
        point.y >= rectPos.y && point.y <= rectPos.y + rectSize.y);
}

void GameOverState::processInput(GLFWwindow* window, float dt) {
    double mouseX, mouseY; // корды мыши
    glfwGetCursorPos(window, &mouseX, &mouseY);
    glm::vec2 mousePos(mouseX, mouseY);

    // егор
    auto updateButtonState = [&](UIButton& btn) {
        if (isPointInRect(mousePos, btn.pos, btn.size)) {
            // чекаем нажали ли?
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                btn.state = 2;
            }
            else {
                btn.state = 1;
            }
        }
        else {
            btn.state = 0;
        }
    };

    updateButtonState(m_btnRetry);
    updateButtonState(m_btnMenu);

    // обработка клика
    int mouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    if (mouseState == GLFW_PRESS && !m_mousePressedLastFrame) {
        m_mousePressedLastFrame = true; // блок

        if (m_btnRetry.state == 2) {
            // рестартим
            m_stateManager.setState(std::make_unique<GameplayState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, m_levelPath));
        }
        else if (m_btnMenu.state == 2) {
            // выход в меню выбора уровней
            m_stateManager.setState(std::make_unique<LevelSelectState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer));
        }
    }
    else if (mouseState == GLFW_RELEASE) {
        m_mousePressedLastFrame = false;// !блок
    }
}

void GameOverState::update(float dt) {}

void GameOverState::render() {
    // фон поражения
    m_renderer->drawSpriteRGBA(m_uiTexture, glm::vec2(0.0f), glm::vec2(m_width, m_height), 0.0f, glm::vec4(0.1f, 0.0f, 0.0f, 0.8f));
    // окошник
    m_renderer->drawSprite(m_uiTexture, m_windowPos, m_windowSize, 0.0f, glm::vec3(0.15f, 0.12f, 0.12f));
    // бам бам бам
    m_textRenderer->RenderText("GAME OVER!", m_windowPos.x + 100.0f, m_windowPos.y + 40.0f, 1.2f, glm::vec3(1.0f, 0.2f, 0.2f));

    // егор
    auto drawButton = [&](UIButton& btn, glm::vec3 color, float textXOffset) {
        m_renderer->drawSprite(m_uiTexture, btn.pos, btn.size, 0.0f, color);
        m_textRenderer->RenderText(btn.text, btn.pos.x + textXOffset, btn.pos.y + 15.0f, 1.0f, glm::vec3(0.95f));
    };

    // определяем цвет
    glm::vec3 retryColor;
    if (m_btnRetry.state == 0) {
        retryColor = glm::vec3(0.3f);
    }
    else if (m_btnRetry.state == 1) {
        retryColor = glm::vec3(0.5f);
    }
    else {
        retryColor = glm::vec3(0.2f);
    }

    glm::vec3 menuColor;
    if (m_btnMenu.state == 0) {
        menuColor = glm::vec3(0.3f);
    }
    else if (m_btnMenu.state == 1) {
        menuColor = glm::vec3(0.5f);
    }
    else {
        menuColor = glm::vec3(0.2f);
    }

    drawButton(m_btnRetry, retryColor, 80.0f);
    drawButton(m_btnMenu, menuColor, 70.0f);
}

void GameOverState::resize(int width, int height) {
    m_width = width;
    m_height = height;
    init();
}