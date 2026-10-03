#include "PauseState.h"
#include "GameStateManager.h"
#include "MainMenuState.h"
#include "GameplayState.h"
#include "../renderer/TextRenderer.h"
#include "../renderer/SpriteRenderer.h"
#include "../resources/ResourceManager.h"
#include "../core/SettingsManager.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>

PauseState::PauseState(GameStateManager& stateManager, int width, int height, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer, GameplayState* gameplayState)
    : m_stateManager(stateManager), m_width(width), m_height(height), m_renderer(renderer), m_textRenderer(textRenderer), m_gameplayState(gameplayState), m_mousePressedLastFrame(false) {

    m_uiTexture = std::shared_ptr<Texture2D>(ResourceManager::getTexture("uiBaseTexture"), [](Texture2D*) {});
    m_whiteTexture = std::shared_ptr<Texture2D>(ResourceManager::getWhiteTexture(), [](Texture2D*) {});
}

void PauseState::init() {
    float s = SettingsManager::getUIScaleMultiplier();

    float winW = std::clamp(420.0f * s, 280.0f, 580.0f);
    float winH = std::clamp(335.0f * s, 240.0f, 470.0f);
    m_windowSize = glm::vec2(winW, winH);
    m_windowPos = glm::vec2((m_width - winW) / 2.0f, (m_height - winH) / 2.0f);
    m_headerHeight = std::clamp(40.0f * s, 30.0f, 56.0f);
    m_isDragging = false;
    m_dragOffset = glm::vec2(0.0f);

    float btnW = std::clamp(260.0f * s, 180.0f, 360.0f);
    float btnH = std::clamp(42.0f  * s, 30.0f, 58.0f);

    // настройка Resume
    m_btnResume.size = glm::vec2(btnW, btnH);
    m_btnResume.text = "Resume";
    m_btnResume.state = 0;

    // настройка Save
    m_btnSave.size = glm::vec2(btnW, btnH);
    m_btnSave.text = "Save Game";
    m_btnSave.state = 0;

    // настройка ползунка громкости
    m_volumeWidget = VolumeSliderWidget(m_windowPos + glm::vec2(30.0f * s, 170.0f * s), winW - 60.0f * s, true, "Громкость звука:");

    // настройка Exit
    m_btnExit.size = glm::vec2(btnW, btnH);
    if (m_gameplayState && m_gameplayState->isEditorTest()) {
        m_btnExit.text = "Назад в редактор";
    } else {
        m_btnExit.text = "Exit to Menu";
    }
    m_btnExit.state = 0;
}
void PauseState::cleanup() {}

bool PauseState::isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) {
    return point.x >= rectPos.x && point.x <= rectPos.x + rectSize.x &&
        point.y >= rectPos.y && point.y <= rectPos.y + rectSize.y;
}

void PauseState::processInput(GLFWwindow* window, float dt) {
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    m_currentMousePos = glm::vec2(mouseX, mouseY);

    float s = SettingsManager::getUIScaleMultiplier();
    float offsetY1 = std::clamp(55.0f * s, 40.0f, 76.0f);
    float offsetY2 = std::clamp(108.0f * s, 78.0f, 150.0f);
    float volumeY  = std::clamp(165.0f * s, 118.0f, 228.0f);
    float offsetY3 = std::clamp(255.0f * s, 180.0f, 352.0f);

    m_btnResume.pos = m_windowPos + glm::vec2((m_windowSize.x - m_btnResume.size.x) / 2.0f, offsetY1);
    m_btnSave.pos   = m_windowPos + glm::vec2((m_windowSize.x - m_btnSave.size.x)   / 2.0f, offsetY2);
    m_volumeWidget.setPosition(m_windowPos + glm::vec2(std::clamp(30.0f * s, 20.0f, 44.0f), volumeY));
    m_btnExit.pos   = m_windowPos + glm::vec2((m_windowSize.x - m_btnExit.size.x)   / 2.0f, offsetY3);

    int mouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    bool isPressed = (mouseState == GLFW_PRESS);
    bool justPressed = (isPressed && !m_mousePressedLastFrame);

    if (justPressed) {
        m_mousePressedLastFrame = true;

        if (m_volumeWidget.handleInput(m_currentMousePos, isPressed, justPressed)) {
            return;
        }

        // проверяем клик по шапке окна
        glm::vec2 headerSize(m_windowSize.x, m_headerHeight);
        if (isPointInRect(m_currentMousePos, m_windowPos, headerSize)) {
            m_isDragging = true;
            m_dragOffset = m_currentMousePos - m_windowPos;
            return;
        }

        // проверяем клик по кнопкам
        if (isPointInRect(m_currentMousePos, m_btnResume.pos, m_btnResume.size)) m_btnResume.state = 2;
        if (isPointInRect(m_currentMousePos, m_btnSave.pos, m_btnSave.size)) m_btnSave.state = 2;
        if (isPointInRect(m_currentMousePos, m_btnExit.pos, m_btnExit.size)) m_btnExit.state = 2;
    }
    else if (isPressed) {
        if (m_volumeWidget.handleInput(m_currentMousePos, isPressed, false)) {
            return;
        }
        if (m_isDragging) {
            m_windowPos = m_currentMousePos - m_dragOffset;
        }
    }
    else if (mouseState == GLFW_RELEASE) {
        m_volumeWidget.handleInput(m_currentMousePos, false, false);
        m_isDragging = false;

        // если отпустили кнопку над Resume
        if (m_btnResume.state == 2 && isPointInRect(m_currentMousePos, m_btnResume.pos, m_btnResume.size)) {
            m_stateManager.popState();
            return;
        }
        // если отпустили кнопку над Save
        if (m_btnSave.state == 2 && isPointInRect(m_currentMousePos, m_btnSave.pos, m_btnSave.size)) {
            if (m_gameplayState) {
                m_gameplayState->saveGame("savegame");
            }
            m_btnSave.state = 0;
            m_mousePressedLastFrame = false;
            return;
        }
        // если отпустили кнопку над Exit
        if (m_btnExit.state == 2 && isPointInRect(m_currentMousePos, m_btnExit.pos, m_btnExit.size)) {
            if (m_gameplayState && m_gameplayState->isEditorTest()) {
                std::cout << "[PauseState] Returning to MapEditor..." << std::endl;
                std::string lvlPath = m_gameplayState ? m_gameplayState->getCurrentLevelPath() : "";
                m_stateManager.returnToMapEditor(lvlPath, m_width, m_height, m_renderer, m_textRenderer);
                return;
            } else {
                m_stateManager.setState(std::make_unique<MainMenuState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer));
                return;
            }
        }

        m_btnResume.state = isPointInRect(m_currentMousePos, m_btnResume.pos, m_btnResume.size) ? 1 : 0;
        m_btnSave.state   = isPointInRect(m_currentMousePos, m_btnSave.pos, m_btnSave.size)   ? 1 : 0;
        m_btnExit.state   = isPointInRect(m_currentMousePos, m_btnExit.pos, m_btnExit.size)   ? 1 : 0;
        m_mousePressedLastFrame = false;
    }
}

void PauseState::update(float dt) {}

void PauseState::render() {
    m_renderer->beginBatch(); // открываем пакет
    // затемнение игры
    m_renderer->drawSpriteRGBA(m_uiTexture, glm::vec2(0.0f), glm::vec2(m_width, m_height), 0.0f, glm::vec4(0.05f, 0.05f, 0.07f, 0.65f));
    // рисуем окно
    m_renderer->drawSprite(m_uiTexture, m_windowPos, m_windowSize, 0.0f, glm::vec3(0.12f, 0.12f, 0.15f));
    // рисуем шапку
    m_renderer->drawSprite(m_uiTexture, m_windowPos, glm::vec2(m_windowSize.x, m_headerHeight), 0.0f, glm::vec3(0.18f, 0.18f, 0.22f));

    // кнопки рисуем зависимо от стейта
    auto drawButton = [&](UIButton& btn, glm::vec3 color) {
        m_renderer->drawSprite(m_uiTexture, btn.pos, btn.size, 0.0f, color);
    };

    glm::vec3 resumeColor = glm::vec3(0.15f, 0.35f, 0.15f); // Idle
    if (m_btnResume.state == 1) resumeColor = glm::vec3(0.22f, 0.55f, 0.22f); // Hover
    if (m_btnResume.state == 2) resumeColor = glm::vec3(0.10f, 0.25f, 0.10f); // Pressed

    glm::vec3 saveColor = glm::vec3(0.15f, 0.30f, 0.45f);
    if (m_btnSave.state == 1) saveColor = glm::vec3(0.20f, 0.40f, 0.60f);
    if (m_btnSave.state == 2) saveColor = glm::vec3(0.10f, 0.20f, 0.30f);

    glm::vec3 exitColor = glm::vec3(0.45f, 0.15f, 0.15f); // Idle
    if (m_btnExit.state == 1) exitColor = glm::vec3(0.65f, 0.20f, 0.20f); // Hover
    if (m_btnExit.state == 2) exitColor = glm::vec3(0.30f, 0.10f, 0.10f); // Pressed

    drawButton(m_btnResume, resumeColor);
    drawButton(m_btnSave, saveColor);
    drawButton(m_btnExit, exitColor);

    m_renderer->endBatch(); // рисуем

    float s = SettingsManager::getUIScaleMultiplier();
    float fTitle = std::clamp(0.90f * s, 0.62f, 1.26f);
    float fBtn   = std::clamp(0.55f * s, 0.40f, 0.76f);

    // текст заголовочный
    float titleW = m_textRenderer->CalculateTextWidth("PAUSE MENU", fTitle);
    m_textRenderer->RenderText("PAUSE MENU", m_windowPos.x + (m_windowSize.x - titleW) * 0.5f, m_windowPos.y + 10.0f, fTitle, glm::vec3(1.0f, 0.75f, 0.0f));

    auto drawBtnText = [&](const UIButton& btn, const std::string& text) {
        float tw = m_textRenderer->CalculateTextWidth(text, fBtn);
        m_textRenderer->RenderText(text, btn.pos.x + (btn.size.x - tw) * 0.5f, btn.pos.y + (btn.size.y - fBtn * 28.0f) * 0.5f + 2.0f, fBtn, glm::vec3(0.95f, 0.95f, 0.95f));
    };

    drawBtnText(m_btnResume, m_btnResume.text);
    drawBtnText(m_btnSave, m_btnSave.text);
    drawBtnText(m_btnExit, m_btnExit.text);

    // Отрисовка виджета громкости
    m_volumeWidget.render(m_renderer.get(), m_textRenderer, m_whiteTexture);
}

void PauseState::resize(int width, int height) {
    m_width = width;
    m_height = height;
    m_windowPos = glm::vec2((m_width - m_windowSize.x) / 2.0f, (m_height - m_windowSize.y) / 2.0f);
}