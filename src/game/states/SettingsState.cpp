#include "SettingsState.h"
#include "GameStateManager.h"
#include "../core/LocalizationManager.h"
#include "../core/SettingsManager.h"
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
    m_windowSize = glm::vec2(440.0f, 310.0f);
    m_windowPos = glm::vec2((m_width - m_windowSize.x) * 0.5f, (m_height - m_windowSize.y) * 0.5f);
    m_headerHeight = 40.0f;
    m_isDragging = false;
    m_dragOffset = glm::vec2(0.0f);

    m_volumeWidget = VolumeSliderWidget(m_windowPos + glm::vec2(30.0f, 65.0f), 380.0f, true, LOC("SETTINGS_VOLUME"));
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

    m_volumeWidget.setPosition(m_windowPos + glm::vec2(30.0f, 65.0f));
    m_langBtnRuPos = m_windowPos + glm::vec2(160.0f, 150.0f);
    m_langBtnUaPos = m_windowPos + glm::vec2(250.0f, 150.0f);
    m_langBtnEnPos = m_windowPos + glm::vec2(340.0f, 150.0f);
    m_langBtnSize = glm::vec2(80.0f, 36.0f);
    m_closeBtnPos = m_windowPos + glm::vec2((m_windowSize.x - m_closeBtnSize.x) * 0.5f, 235.0f);

    int mouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    bool isPressed = (mouseState == GLFW_PRESS);
    bool justPressed = (isPressed && !m_mousePressedLastFrame);

    if (justPressed) {
        m_mousePressedLastFrame = true;

        // 1. Проверяем взаимодействие с виджетом громкости
        if (m_volumeWidget.handleInput(m_currentMousePos, isPressed, justPressed)) {
            return;
        }

        // 2. Кнопки смены языка
        if (isPointInRect(m_currentMousePos, m_langBtnRuPos, m_langBtnSize)) {
            LocalizationManager::setLanguageByCode("ru");
            m_volumeWidget.setLabel(LOC("SETTINGS_VOLUME"));
            return;
        }
        if (isPointInRect(m_currentMousePos, m_langBtnUaPos, m_langBtnSize)) {
            LocalizationManager::setLanguageByCode("ua");
            m_volumeWidget.setLabel(LOC("SETTINGS_VOLUME"));
            return;
        }
        if (isPointInRect(m_currentMousePos, m_langBtnEnPos, m_langBtnSize)) {
            LocalizationManager::setLanguageByCode("en");
            m_volumeWidget.setLabel(LOC("SETTINGS_VOLUME"));
            return;
        }

        // 3. Проверяем клик по шапке окна для перетаскивания
        glm::vec2 headerSize(m_windowSize.x, m_headerHeight);
        if (isPointInRect(m_currentMousePos, m_windowPos, headerSize)) {
            m_isDragging = true;
            m_dragOffset = m_currentMousePos - m_windowPos;
            return;
        }

        // 4. Проверяем клик по кнопке Закрыть
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
            SettingsManager::save();
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

        // Кнопки языков (RU, UA, EN)
        std::string curLang = SettingsManager::getLanguage();
        auto drawLangBtn = [&](glm::vec2 pos, const std::string& code) {
            bool isActive = (curLang == code);
            glm::vec3 bg = isActive ? glm::vec3(0.18f, 0.35f, 0.50f) : glm::vec3(0.16f, 0.18f, 0.24f);
            glm::vec3 border = isActive ? glm::vec3(0.35f, 0.85f, 1.0f) : glm::vec3(0.28f, 0.30f, 0.38f);
            m_renderer->drawSprite(m_whiteTexture, pos, m_langBtnSize, 0.0f, border);
            m_renderer->drawSprite(m_whiteTexture, pos + glm::vec2(2.0f), m_langBtnSize - glm::vec2(4.0f), 0.0f, bg);
        };

        drawLangBtn(m_langBtnRuPos, "ru");
        drawLangBtn(m_langBtnUaPos, "ua");
        drawLangBtn(m_langBtnEnPos, "en");

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
    std::string titleStr = LOC("SETTINGS_TITLE");
    float titleWidth = m_textRenderer->CalculateTextWidth(titleStr, 0.65f);
    m_textRenderer->RenderText(titleStr, m_windowPos.x + (m_windowSize.x - titleWidth) * 0.5f, m_windowPos.y + 10.0f, 0.65f, glm::vec3(1.0f, 0.85f, 0.2f));

    // Виджет ползунка громкости и кнопки Mute
    m_volumeWidget.render(m_renderer.get(), m_textRenderer, m_whiteTexture);

    // Подпись выбора языка
    m_textRenderer->RenderText(LOC("SETTINGS_LANGUAGE"), m_windowPos.x + 30.0f, m_windowPos.y + 158.0f, 0.54f, glm::vec3(0.9f, 0.9f, 0.95f));

    // Текст на кнопках языков
    auto drawBtnCenterText = [&](glm::vec2 pos, const std::string& txt, bool active) {
        float w = m_textRenderer->CalculateTextWidth(txt, 0.52f);
        glm::vec3 col = active ? glm::vec3(0.35f, 0.95f, 1.0f) : glm::vec3(0.8f, 0.82f, 0.88f);
        m_textRenderer->RenderText(txt, pos.x + (m_langBtnSize.x - w) * 0.5f, pos.y + 9.0f, 0.52f, col);
    };

    std::string curLang = SettingsManager::getLanguage();
    drawBtnCenterText(m_langBtnRuPos, "РУС", curLang == "ru");
    drawBtnCenterText(m_langBtnUaPos, "УКР", curLang == "ua");
    drawBtnCenterText(m_langBtnEnPos, "ENG", curLang == "en");

    // Текст на кнопке Закрыть
    std::string closeStr = LOC("SETTINGS_CLOSE");
    float btnTextWidth = m_textRenderer->CalculateTextWidth(closeStr, 0.55f);
    m_textRenderer->RenderText(closeStr, m_closeBtnPos.x + (m_closeBtnSize.x - btnTextWidth) * 0.5f, m_closeBtnPos.y + 11.0f, 0.55f, glm::vec3(0.95f, 0.95f, 0.98f));

    m_renderer->endBatch();
}

void SettingsState::resize(int width, int height) {
    m_width = width;
    m_height = height;
    m_windowPos = glm::vec2((m_width - m_windowSize.x) * 0.5f, (m_height - m_windowSize.y) * 0.5f);
}

