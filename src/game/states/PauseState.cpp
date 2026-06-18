#include "PauseState.h"
#include "GameStateManager.h"
#include "MainMenuState.h"
#include "../renderer/TextRenderer.h"
#include <GLFW/glfw3.h>

PauseState::PauseState(GameStateManager& stateManager, int width, int height, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer)
    : m_stateManager(stateManager), m_width(width), m_height(height), m_renderer(renderer), m_textRenderer(textRenderer), m_mousePressedLastFrame(false) {
}

void PauseState::init() {}
void PauseState::cleanup() {}

bool PauseState::isButtonClicked(double mouseX, double mouseY, float btnX, float btnY, float btnW, float btnH) {
    return mouseX >= btnX && mouseX <= btnX + btnW && mouseY >= btnY && mouseY <= btnY + btnH;
}

void PauseState::processInput(GLFWwindow* window, float dt) {
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    int mouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    if (mouseState == GLFW_PRESS && !m_mousePressedLastFrame) {
        m_mousePressedLastFrame = true;

        float resBtnX = m_width / 2.0f - 60.0f, resBtnY = m_height / 2.0f;
        float resBtnW = 120.0f, resBtnH = 40.0f;

        float exitBtnX = m_width / 2.0f - 140.0f, exitBtnY = m_height / 2.0f + 60.0f;
        float exitBtnW = 280.0f, exitBtnH = 40.0f;

        // если кликнули Resume
        if (isButtonClicked(mouseX, mouseY, resBtnX, resBtnY, resBtnW, resBtnH)) {
            m_stateManager.popState(); // удаляем паузу
            return;
        }

        // Если кликнули Exit to Main Menu
        if (isButtonClicked(mouseX, mouseY, exitBtnX, exitBtnY, exitBtnW, exitBtnH)) {
            m_stateManager.setState(std::make_unique<MainMenuState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer));
            return;
        }
    }
    else if (mouseState == GLFW_RELEASE) {
        m_mousePressedLastFrame = false;
    }
}

void PauseState::update(float dt) {}

void PauseState::render() {
    m_textRenderer->RenderText("GAME PAUSED", m_width / 2.0f - 120.0f, m_height / 2.0f - 80.0f, 1.5f, glm::vec3(1.0f, 0.8f, 0.0f));
    m_textRenderer->RenderText("> Resume <", m_width / 2.0f - 60.0f, m_height / 2.0f, 1.0f, glm::vec3(1.0f));
    m_textRenderer->RenderText("> Exit to Main Menu <", m_width / 2.0f - 140.0f, m_height / 2.0f + 60.0f, 1.0f, glm::vec3(1.0f, 0.3f, 0.3f));
}

void PauseState::resize(int width, int height) {
    m_width = width;
    m_height = height;
}