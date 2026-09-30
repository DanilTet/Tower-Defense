#include "SettingsState.h"
#include "GameStateManager.h"
#include "../../renderer/SpriteRenderer.h"
#include "../../renderer/TextRenderer.h"
#include "../../resources/ResourceManager.h"
#include <GLFW/glfw3.h>

SettingsState::SettingsState(GameStateManager& stateManager, int width, int height, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer)
    : m_stateManager(stateManager), m_width(width), m_height(height), m_renderer(renderer), m_textRenderer(textRenderer)
{
    m_whiteTexture = std::shared_ptr<Texture2D>(ResourceManager::getWhiteTexture(), [](Texture2D*) {});
    m_uiTexture = std::shared_ptr<Texture2D>(ResourceManager::getTexture("uiBaseTexture"), [](Texture2D*) {});
}

void SettingsState::init() {
    m_windowSize = glm::vec2(440.0f, 260.0f);
    m_windowPos = glm::vec2((m_width - m_windowSize.x) * 0.5f, (m_height - m_windowSize.y) * 0.5f);
    m_headerHeight = 40.0f;
    m_isDragging = false;
    m_dragOffset = glm::vec2(0.0f);

    m_volumeWidget = VolumeSliderWidget(m_windowPos + glm::vec2(30.0f, 75.0f), 380.0f, true, "Громкость звука:");
    m_closeBtnSize = glm::vec2(180.0f, 40.0f);
}

void SettingsState::cleanup() {}

bool SettingsState::isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) {
    return point.x >= rectPos.x && point.x <= rectPos.x + rectSize.x &&
        point.y >= rectPos.y && point.y <= rectPos.y + rectSize.y;
}

void SettingsState::processInput(GLFWwindow* window, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        m_stateManager.popState();
        return;
    }

    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    m_currentMousePos = glm::vec2(mouseX, mouseY);

    m_volumeWidget.setPosition(m_windowPos + glm::vec2(30.0f, 75.0f));
    m_closeBtnPos = m_windowPos + glm::vec2((m_windowSize.x - m_closeBtnSize.x) * 0.5f, 185.0f);

    int mouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    bool isPressed = (mouseState == GLFW_PRESS);
    bool justPressed = (isPressed && !m_mousePressedLastFrame);

    if (justPressed) {
        m_mousePressedLastFrame = true;

        // 1. Проверяем взаимодействие с виджетом громкости
        if (m_volumeWidget.handleInput(m_currentMousePos, isPressed, justPressed)) {
            return;
        }

        // 2. Проверяем клик по шапке окна для перетаскивания
        glm::vec2 headerSize(m_windowSize.x, m_headerHeight);
        if (isPointInRect(m_currentMousePos, m_windowPos, headerSize)) {
            m_isDragging = true;
            m_dragOffset = m_currentMousePos - m_windowPos;
            return;
        }

        // 3. Проверяем клик по кнопке Закрыть
        if (isPointInRect(m_currentMousePos, m_closeBtnPos, m_closeBtnSize)) {
            m_closeBtnState = 2;
            return;
        }
    }
    else if (isPressed) {
        // Мышь зажата
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

        if (m_closeBtnState == 2 && isPointInRect(m_currentMousePos, m_closeBtnPos, m_closeBtnSize)) {
            m_stateManager.popState();
            return;
        }

        m_closeBtnState = isPointInRect(m_currentMousePos, m_closeBtnPos, m_closeBtnSize) ? 1 : 0;
        m_mousePressedLastFrame = false;
    }
}

void SettingsState::update(float dt) {}

void SettingsState::render() {
    if (!m_renderer || !m_textRenderer) return;

    m_renderer->beginBatch();

    // 1. Затемнение фона
    if (m_whiteTexture) {
        m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(0.0f), glm::vec2(m_width, m_height), 0.0f, glm::vec4(0.04f, 0.05f, 0.07f, 0.70f));

        // 2. Окно настроек
        m_renderer->drawSprite(m_whiteTexture, m_windowPos, m_windowSize, 0.0f, glm::vec3(0.12f, 0.13f, 0.16f));

        // Рамка окна
        glm::vec3 borderColor(0.24f, 0.26f, 0.32f);
        m_renderer->drawSprite(m_whiteTexture, m_windowPos, glm::vec2(m_windowSize.x, 1.0f), 0.0f, borderColor);
        m_renderer->drawSprite(m_whiteTexture, m_windowPos + glm::vec2(0.0f, m_windowSize.y - 1.0f), glm::vec2(m_windowSize.x, 1.0f), 0.0f, borderColor);
        m_renderer->drawSprite(m_whiteTexture, m_windowPos, glm::vec2(1.0f, m_windowSize.y), 0.0f, borderColor);
        m_renderer->drawSprite(m_whiteTexture, m_windowPos + glm::vec2(m_windowSize.x - 1.0f, 0.0f), glm::vec2(1.0f, m_windowSize.y), 0.0f, borderColor);

        // Шапка окна
        m_renderer->drawSprite(m_whiteTexture, m_windowPos, glm::vec2(m_windowSize.x, m_headerHeight), 0.0f, glm::vec3(0.18f, 0.19f, 0.24f));
        m_renderer->drawSprite(m_whiteTexture, m_windowPos + glm::vec2(0.0f, m_headerHeight), glm::vec2(m_windowSize.x, 2.0f), 0.0f, glm::vec3(1.0f, 0.85f, 0.2f));

        // 3. Кнопка Закрыть
        glm::vec3 btnColor(0.20f, 0.22f, 0.28f);
        if (m_closeBtnState == 1) btnColor = glm::vec3(0.28f, 0.32f, 0.42f);
        if (m_closeBtnState == 2) btnColor = glm::vec3(0.14f, 0.16f, 0.20f);

        m_renderer->drawSprite(m_whiteTexture, m_closeBtnPos, m_closeBtnSize, 0.0f, btnColor);
        m_renderer->drawSprite(m_whiteTexture, m_closeBtnPos, glm::vec2(m_closeBtnSize.x, 1.0f), 0.0f, glm::vec3(0.35f, 0.38f, 0.48f));
        m_renderer->drawSprite(m_whiteTexture, m_closeBtnPos + glm::vec2(0.0f, m_closeBtnSize.y - 1.0f), glm::vec2(m_closeBtnSize.x, 1.0f), 0.0f, glm::vec3(0.35f, 0.38f, 0.48f));
        m_renderer->drawSprite(m_whiteTexture, m_closeBtnPos, glm::vec2(1.0f, m_closeBtnSize.y), 0.0f, glm::vec3(0.35f, 0.38f, 0.48f));
        m_renderer->drawSprite(m_whiteTexture, m_closeBtnPos + glm::vec2(m_closeBtnSize.x - 1.0f, 0.0f), glm::vec2(1.0f, m_closeBtnSize.y), 0.0f, glm::vec3(0.35f, 0.38f, 0.48f));
    }

    m_renderer->flush();

    // Заголовок окна
    float titleWidth = m_textRenderer->CalculateTextWidth("НАСТРОЙКИ", 0.65f);
    m_textRenderer->RenderText("НАСТРОЙКИ", m_windowPos.x + (m_windowSize.x - titleWidth) * 0.5f, m_windowPos.y + 10.0f, 0.65f, glm::vec3(1.0f, 0.85f, 0.2f));

    // Виджет ползунка громкости и кнопки Mute
    m_volumeWidget.render(m_renderer.get(), m_textRenderer, m_whiteTexture);

    // Текст на кнопке Закрыть
    float btnTextWidth = m_textRenderer->CalculateTextWidth("Закрыть", 0.55f);
    m_textRenderer->RenderText("Закрыть", m_closeBtnPos.x + (m_closeBtnSize.x - btnTextWidth) * 0.5f, m_closeBtnPos.y + 11.0f, 0.55f, glm::vec3(0.95f, 0.95f, 0.98f));

    m_renderer->endBatch();
}

void SettingsState::resize(int width, int height) {
    m_width = width;
    m_height = height;
    m_windowPos = glm::vec2((m_width - m_windowSize.x) * 0.5f, (m_height - m_windowSize.y) * 0.5f);
}

