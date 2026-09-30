#include "MainMenuState.h"
#include "GameStateManager.h"
#include "LevelSelectState.h"
#include "GameplayState.h"
#include "MapEditorState.h"
#include "SettingsState.h"
#include "../core/LocalizationManager.h"
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

        float btnW = 240.0f;
        float btnH = 38.0f;
        float btnX = (m_width - btnW) * 0.5f;

        float startBtnY = m_height / 2.0f - 60.0f;
        float loadBtnY = m_height / 2.0f - 5.0f;
        float editorBtnY = m_height / 2.0f + 50.0f;
        float settingsBtnY = m_height / 2.0f + 105.0f;
        float exitBtnY = m_height / 2.0f + 160.0f;

        // если клик по старт гейм
        if (isButtonClicked(mouseX, mouseY, btnX, startBtnY, btnW, btnH)) {
            m_stateManager.setState(std::make_unique<LevelSelectState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer));
            return;
        }

        // Клик по Load Game
        if (isButtonClicked(mouseX, mouseY, btnX, loadBtnY, btnW, btnH)) {
            auto loadState = std::make_unique<GameplayState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, "");
            loadState->setSaveToLoad("savegame");

            m_stateManager.setState(std::move(loadState));
            return;
        }

        // Клик по Map Editor
        if (isButtonClicked(mouseX, mouseY, btnX, editorBtnY, btnW, btnH)) {
            m_stateManager.setState(std::make_unique<MapEditorState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer));
            return;
        }

        // Клик по Settings
        if (isButtonClicked(mouseX, mouseY, btnX, settingsBtnY, btnW, btnH)) {
            m_stateManager.pushState(std::make_unique<SettingsState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer));
            return;
        }

        // если выход
        if (isButtonClicked(mouseX, mouseY, btnX, exitBtnY, btnW, btnH)) {
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
    float titleW = m_textRenderer->CalculateTextWidth("Donbasyata Tower Defense", 1.5f);
    m_textRenderer->RenderText("Donbasyata Tower Defense", (m_width - titleW) * 0.5f, m_height / 2.0f - 130.0f, 1.5f, glm::vec3(1.0f, 1.0f, 0.0f));

    // рисуем кнопки с автоцентрированием текста
    std::string startStr = "> " + LOC("BTN_START_GAME") + " <";
    float startW = m_textRenderer->CalculateTextWidth(startStr, 1.2f);
    m_textRenderer->RenderText(startStr, (m_width - startW) * 0.5f, m_height / 2.0f - 60.0f, 1.2f, glm::vec3(1.0f, 1.0f, 1.0f));

    std::string loadStr = "> " + LOC("BTN_LOAD_GAME") + " <";
    float loadW = m_textRenderer->CalculateTextWidth(loadStr, 1.2f);
    m_textRenderer->RenderText(loadStr, (m_width - loadW) * 0.5f, m_height / 2.0f - 5.0f, 1.2f, glm::vec3(0.2f, 0.8f, 1.0f));

    std::string editorStr = "> " + LOC("BTN_MAP_EDITOR") + " <";
    float editorW = m_textRenderer->CalculateTextWidth(editorStr, 1.2f);
    m_textRenderer->RenderText(editorStr, (m_width - editorW) * 0.5f, m_height / 2.0f + 50.0f, 1.2f, glm::vec3(0.9f, 0.8f, 0.2f)); // Золотистый

    std::string settingsStr = "> " + LOC("BTN_SETTINGS") + " <";
    float settingsW = m_textRenderer->CalculateTextWidth(settingsStr, 1.2f);
    m_textRenderer->RenderText(settingsStr, (m_width - settingsW) * 0.5f, m_height / 2.0f + 105.0f, 1.2f, glm::vec3(0.75f, 0.88f, 1.0f)); // Светло-голубой

    std::string exitStr = "> " + LOC("BTN_EXIT") + " <";
    float exitW = m_textRenderer->CalculateTextWidth(exitStr, 1.2f);
    m_textRenderer->RenderText(exitStr, (m_width - exitW) * 0.5f, m_height / 2.0f + 160.0f, 1.2f, glm::vec3(1.0f, 0.3f, 0.3f));

    m_renderer->endBatch(); // закрываем пакет
}

void MainMenuState::resize(int width, int height) {
    m_width = width;
    m_height = height;
}