#include "PauseState.h"
#include "GameStateManager.h"
#include "MainMenuState.h"
#include "../renderer/TextRenderer.h"
#include "../renderer/SpriteRenderer.h"
#include "../resources/ResourceManager.h"
#include <GLFW/glfw3.h>

PauseState::PauseState(GameStateManager& stateManager, int width, int height, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer)
    : m_stateManager(stateManager), m_width(width), m_height(height), m_renderer(renderer), m_textRenderer(textRenderer), m_mousePressedLastFrame(false) {
    
    
    m_uiTexture = std::shared_ptr<Texture2D>(ResourceManager::getTexture("uiBaseTexture"), [](Texture2D*) {});
}

void PauseState::init() {
    // настройка окна
    m_windowSize = glm::vec2(400.0f, 300.0f);
    m_windowPos = glm::vec2((m_width - m_windowSize.x) / 2.0f, (m_height - m_windowSize.y) / 2.0f);
    m_headerHeight = 40.0f;
    m_isDragging = false;
    m_dragOffset = glm::vec2(0.0f);

    // настраиваем кнопки относительно окна
    m_btnResume.size = glm::vec2(200.0f, 50.0f);
    m_btnResume.text = "Resume";
    m_btnResume.state = 0;

    m_btnExit.size = glm::vec2(250.0f, 50.0f);
    m_btnExit.text = "Exit to Menu";
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

    m_btnResume.pos = m_windowPos + glm::vec2((m_windowSize.x - m_btnResume.size.x) / 2.0f, 100.0f);
    m_btnExit.pos = m_windowPos + glm::vec2((m_windowSize.x - m_btnExit.size.x) / 2.0f, 180.0f);

    int mouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

    // Drag & Drop
    if (mouseState == GLFW_PRESS) {
        if (!m_mousePressedLastFrame) {
            // кликнули в этом кадре
            m_mousePressedLastFrame = true;

            // проверяем клик по шапке окна
            glm::vec2 headerSize(m_windowSize.x, m_headerHeight);
            if (isPointInRect(m_currentMousePos, m_windowPos, headerSize)) {
                m_isDragging = true;
                // запоминаем разницу между мышью и углом окна
                m_dragOffset = m_currentMousePos - m_windowPos;
            }

            // проверяем клик по кнопкам
            if (isPointInRect(m_currentMousePos, m_btnResume.pos, m_btnResume.size)) m_btnResume.state = 2; // Pressed
            if (isPointInRect(m_currentMousePos, m_btnExit.pos, m_btnExit.size)) m_btnExit.state = 2; // Pressed
        }
        else if (m_isDragging) {
            // если мышь зажата и мы тащим окно
            m_windowPos = m_currentMousePos - m_dragOffset;
        }
    }
    else if (mouseState == GLFW_RELEASE) {
        // отпустили кнопку мыши
        m_isDragging = false;

        // если отпустили кнопку над Resume
        if (m_btnResume.state == 2 && isPointInRect(m_currentMousePos, m_btnResume.pos, m_btnResume.size)) {
            m_stateManager.popState();
            return;
        }

        // если отпустили кнопку над Exit
        if (m_btnExit.state == 2 && isPointInRect(m_currentMousePos, m_btnExit.pos, m_btnExit.size)) {
            m_stateManager.setState(std::make_unique<MainMenuState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer));
            return;
        }

        m_mousePressedLastFrame = false;
    }

    // обработка Hover
    if (mouseState == GLFW_RELEASE) {
        m_btnResume.state = isPointInRect(m_currentMousePos, m_btnResume.pos, m_btnResume.size) ? 1 : 0;
        m_btnExit.state = isPointInRect(m_currentMousePos, m_btnExit.pos, m_btnExit.size) ? 1 : 0;
    }
}

void PauseState::update(float dt) {}

void PauseState::render() {
    // затемнение игры
    m_renderer->drawSpriteRGBA(m_uiTexture, glm::vec2(0.0f), glm::vec2(m_width, m_height), 0.0f, glm::vec4(0.05f, 0.05f, 0.07f, 0.65f));
    // рисуем окно
    m_renderer->drawSprite(m_uiTexture, m_windowPos, m_windowSize, 0.0f, glm::vec3(0.12f, 0.12f, 0.15f));
    // рисуем шапку
    m_renderer->drawSprite(m_uiTexture, m_windowPos, glm::vec2(m_windowSize.x, m_headerHeight), 0.0f, glm::vec3(0.18f, 0.18f, 0.22f));
    // текст заголовочный
    m_textRenderer->RenderText("PAUSE MENU", m_windowPos.x + 135.0f, m_windowPos.y + 10.0f, 1.0f, glm::vec3(1.0f, 0.75f, 0.0f));

    // кнопки рисуем зависимо от стейта
    auto drawButton = [&](UIButton& btn, glm::vec3 color, float textXOffset) {
        // gодложка кнопки
        m_renderer->drawSprite(m_uiTexture, btn.pos, btn.size, 0.0f, color);

        // текст кнопки
        float textX = btn.pos.x + textXOffset;
        float textY = btn.pos.y + 12.0f;
        m_textRenderer->RenderText(btn.text, textX, textY, 1.0f, glm::vec3(0.95f, 0.95f, 0.95f));
    };

    glm::vec3 resumeColor = glm::vec3(0.15f, 0.35f, 0.15f); // Idle
    if (m_btnResume.state == 1) resumeColor = glm::vec3(0.22f, 0.55f, 0.22f); // Hover
    if (m_btnResume.state == 2) resumeColor = glm::vec3(0.10f, 0.25f, 0.10f); // Pressed


    glm::vec3 exitColor = glm::vec3(0.45f, 0.15f, 0.15f); // Idle
    if (m_btnExit.state == 1) exitColor = glm::vec3(0.65f, 0.20f, 0.20f); // Hover
    if (m_btnExit.state == 2) exitColor = glm::vec3(0.30f, 0.10f, 0.10f); // Pressed

    drawButton(m_btnResume, resumeColor, 65.0f);
    drawButton(m_btnExit, exitColor, 45.0f);
}

void PauseState::resize(int width, int height) {
    m_width = width;
    m_height = height;
    m_windowPos = glm::vec2((m_width - m_windowSize.x) / 2.0f, (m_height - m_windowSize.y) / 2.0f);
}