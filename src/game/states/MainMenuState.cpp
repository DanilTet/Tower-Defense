#include "MainMenuState.h"
#include "GameStateManager.h"
#include "LevelSelectState.h"
#include "GameplayState.h"
#include "../renderer/TextRenderer.h"
#include <GLFW/glfw3.h>
#include "../resources/ResourceManager.h"
#include <iostream>

MainMenuState::MainMenuState(GameStateManager& stateManager, int width, int height, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer)
    : m_stateManager(stateManager), m_width(width), m_height(height), m_renderer(renderer), m_textRenderer(textRenderer), m_mousePressedLastFrame(false) {
}

void MainMenuState::init() {
    std::cout << "Main Menu Initialized" << std::endl;
    ResourceManager::loadTexture("uiBaseTexture", "res/textures/ui_space.png");
}

void MainMenuState::cleanup() {}

bool MainMenuState::isButtonClicked(double mouseX, double mouseY, float btnX, float btnY, float btnW, float btnH) {
    return mouseX >= btnX && mouseX <= btnX + btnW && mouseY >= btnY && mouseY <= btnY + btnH;
}

void MainMenuState::processInput(GLFWwindow* window, float dt) {
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    int mouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    if (mouseState == GLFW_PRESS && !m_mousePressedLastFrame) {
        m_mousePressedLastFrame = true;

        // координаты кнопок ОЧЕНЬ ПРИМЕРНО
        float startBtnX = m_width / 2.0f - 100.0f;
        float startBtnY = m_height / 2.0f - 20.0f;
        float startBtnW = 200.0f, startBtnH = 40.0f;

        float loadBtnX = m_width / 2.0f - 100.0f;
        float loadBtnY = m_height / 2.0f + 40.0f;
        float loadBtnW = 200.0f, loadBtnH = 40.0f;

        float exitBtnX = m_width / 2.0f - 50.0f;
        float exitBtnY = m_height / 2.0f + 100.0f;
        float exitBtnW = 100.0f, exitBtnH = 40.0f;

        // если клик по старт гейм
        if (isButtonClicked(mouseX, mouseY, startBtnX, startBtnY, startBtnW, startBtnH)) {
            // перекоючаем стейт
            m_stateManager.setState(std::make_unique<LevelSelectState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer));
            return;
        }

        // Клик по Load Game
        if (isButtonClicked(mouseX, mouseY, loadBtnX, loadBtnY, loadBtnW, loadBtnH)) {
            auto loadState = std::make_unique<GameplayState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, "");
            loadState->setSaveToLoad("savegame");

            m_stateManager.setState(std::move(loadState));
            return;
        }

        // если выход
        if (isButtonClicked(mouseX, mouseY, exitBtnX, exitBtnY, exitBtnW, exitBtnH)) {
            // закрываем нахер
            glfwSetWindowShouldClose(window, true);
        }
    }
    else if (mouseState == GLFW_RELEASE) {
        m_mousePressedLastFrame = false;
    }
}

void MainMenuState::update(float dt) {}

void MainMenuState::render() {
    m_renderer->beginBatch(); // открываем пакет
    // рисуем заголовок
    m_textRenderer->RenderText("Donbasyata Tower Defense", m_width / 2.0f - 180.0f, m_height / 2.0f - 100.0f, 1.5f, glm::vec3(1.0f, 1.0f, 0.0f));

    // рисуем копки
    m_textRenderer->RenderText("> Start Game <", m_width / 2.0f - 100.0f, m_height / 2.0f - 20.0f, 1.2f, glm::vec3(1.0f, 1.0f, 1.0f));
    m_textRenderer->RenderText("> Load Game <", m_width / 2.0f - 100.0f, m_height / 2.0f + 40.0f, 1.2f, glm::vec3(0.2f, 0.8f, 1.0f)); // Подсветим бирюзовым
    m_textRenderer->RenderText("> Exit <", m_width / 2.0f - 50.0f, m_height / 2.0f + 100.0f, 1.2f, glm::vec3(1.0f, 0.3f, 0.3f));
    m_renderer->endBatch(); // закрываем пакет
}

void MainMenuState::resize(int width, int height) {
    m_width = width;
    m_height = height;
}