#include "LevelSelectState.h"
#include "GameStateManager.h"
#include "GameplayState.h"
#include "MainMenuState.h"
#include "MapEditorState.h"
#include "../resources/ResourceManager.h"
#include "../renderer/SpriteRenderer.h"
#include "../renderer/TextRenderer.h"
#include "../textures/Texture2D.h"
#include <GLFW/glfw3.h>
#include <algorithm>

LevelSelectState::LevelSelectState(GameStateManager& stateManager, int width, int height, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer)
    : m_stateManager(stateManager), m_width(width), m_height(height), m_renderer(renderer), m_textRenderer(textRenderer)
{
    m_uiTexture = std::shared_ptr<Texture2D>(ResourceManager::getTexture("uiBaseTexture"), [](Texture2D*) {});
    m_whiteTexture = std::shared_ptr<Texture2D>(ResourceManager::getWhiteTexture(), [](Texture2D*) {});
    m_mousePressedLastFrame = true;
}

void LevelSelectState::init() {
    m_levelCards.clear();

    auto allLevels = LevelManager::getAvailableLevels();
    if (allLevels.empty()) {
        LevelInfo lvl1;
        lvl1.filename = "level_1.json";
        lvl1.name = "LEVEL 1";
        lvl1.fullPath = "res/levels/level_1.json";
        lvl1.isBuiltIn = true;
        allLevels.push_back(lvl1);
    }

    int itemsPerPage = 6;
    m_totalPages = std::max(1, static_cast<int>((allLevels.size() + itemsPerPage - 1) / itemsPerPage));
    if (m_currentPage >= m_totalPages) m_currentPage = m_totalPages - 1;
    if (m_currentPage < 0) m_currentPage = 0;

    int cols = 3;
    float cardW = 210.0f;
    float cardH = 140.0f;
    float gapX = 24.0f;
    float gapY = 24.0f;
    float totalW = cols * cardW + (cols - 1) * gapX;
    float startX = (m_width - totalW) * 0.5f;
    float startY = 120.0f;

    int startIndex = m_currentPage * itemsPerPage;
    int endIndex = std::min(startIndex + itemsPerPage, static_cast<int>(allLevels.size()));

    for (int i = startIndex; i < endIndex; ++i) {
        int localIdx = i - startIndex;
        int col = localIdx % cols;
        int row = localIdx / cols;
        glm::vec2 cardPos(startX + col * (cardW + gapX), startY + row * (cardH + gapY));

        LevelCardUI card;
        card.cardBtn = { cardPos, glm::vec2(cardW, cardH), allLevels[i].name, 0 };
        card.levelPath = allLevels[i].fullPath;
        card.filename = allLevels[i].filename;
        card.displayName = allLevels[i].name;
        card.isBuiltIn = allLevels[i].isBuiltIn;

        if (!card.isBuiltIn) {
            card.renameBtn = { glm::vec2(cardPos.x + cardW - 74.0f, cardPos.y + cardH - 32.0f), glm::vec2(66.0f, 24.0f), "Rename", 0 };
        }

        m_levelCards.push_back(card);
    }

    // Кнопки пагинации
    if (m_totalPages > 1) {
        float navY = startY + 2 * (cardH + gapY) + 12.0f;
        m_btnPrevPage = { glm::vec2(startX, navY), glm::vec2(100.0f, 36.0f), "< PREV", 0 };
        m_btnNextPage = { glm::vec2(startX + totalW - 100.0f, navY), glm::vec2(100.0f, 36.0f), "NEXT >", 0 };
    }

    // Нижние кнопки управления
    float bottomY = static_cast<float>(m_height) - 80.0f;
    m_btnBack = { glm::vec2(m_width / 2.0f - 210.0f, bottomY), glm::vec2(190.0f, 44.0f), "BACK TO MENU", 0 };
    m_btnEditor = { glm::vec2(m_width / 2.0f + 20.0f, bottomY), glm::vec2(190.0f, 44.0f), "+ MAP EDITOR", 0 };
}

void LevelSelectState::cleanup() {}

bool LevelSelectState::isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) {
    return (point.x >= rectPos.x && point.x <= rectPos.x + rectSize.x &&
            point.y >= rectPos.y && point.y <= rectPos.y + rectSize.y);
}

void LevelSelectState::openRenameModal(const std::string& targetFileName) {
    m_renameTargetFileName = targetFileName;
    std::string stem = targetFileName;
    if (stem.length() >= 5 && stem.substr(stem.length() - 5) == ".json") {
        stem = stem.substr(0, stem.length() - 5);
    }
    m_renameInputText = stem;
    m_isRenameModalOpen = true;
    m_cursorBlinkTimer = 0.0f;
}

void LevelSelectState::confirmRename() {
    if (m_renameInputText.empty()) {
        m_isRenameModalOpen = false;
        return;
    }

    std::string newFileName = LevelManager::sanitizeLevelFileName(m_renameInputText);
    if (newFileName != m_renameTargetFileName) {
        LevelManager::renameLevel(m_renameTargetFileName, newFileName);
    }
    m_isRenameModalOpen = false;
    init();
}

bool LevelSelectState::processRenameModalInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, float dt) {
    if (!m_isRenameModalOpen) return false;

    glm::vec2 modalSize(440.0f, 210.0f);
    glm::vec2 modalPos((static_cast<float>(m_width) - modalSize.x) * 0.5f, (static_cast<float>(m_height) - modalSize.y) * 0.5f);

    glm::vec2 btnSavePos(modalPos.x + 30.0f, modalPos.y + 148.0f);
    glm::vec2 btnSaveSize(190.0f, 40.0f);
    glm::vec2 btnCancelPos(modalPos.x + 240.0f, modalPos.y + 148.0f);
    glm::vec2 btnCancelSize(170.0f, 40.0f);

    auto checkKeyInput = [&](int key) -> bool {
        bool down = (glfwGetKey(window, key) == GLFW_PRESS);
        auto& ks = m_keyStates[key];
        if (down) {
            if (!ks.isDown) {
                ks.isDown = true;
                ks.holdTimer = 0.0f;
                ks.repeatTimer = 0.0f;
                return true;
            } else {
                ks.holdTimer += dt;
                if (ks.holdTimer >= 0.38f) {
                    ks.repeatTimer += dt;
                    if (ks.repeatTimer >= 0.05f) {
                        ks.repeatTimer = 0.0f;
                        return true;
                    }
                }
            }
        } else {
            ks.isDown = false;
            ks.holdTimer = 0.0f;
            ks.repeatTimer = 0.0f;
        }
        return false;
    };

    if (checkKeyInput(GLFW_KEY_ESCAPE)) {
        m_isRenameModalOpen = false;
        return true;
    }
    if (checkKeyInput(GLFW_KEY_ENTER) || checkKeyInput(GLFW_KEY_KP_ENTER)) {
        confirmRename();
        return true;
    }
    if (checkKeyInput(GLFW_KEY_BACKSPACE)) {
        if (!m_renameInputText.empty()) {
            m_renameInputText.pop_back();
        }
        m_cursorBlinkTimer = 0.0f;
        return true;
    }

    bool isShift = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);

    for (int k = GLFW_KEY_A; k <= GLFW_KEY_Z; ++k) {
        if (checkKeyInput(k) && m_renameInputText.length() < 30) {
            char ch = isShift ? static_cast<char>('A' + (k - GLFW_KEY_A)) : static_cast<char>('a' + (k - GLFW_KEY_A));
            m_renameInputText += ch;
            m_cursorBlinkTimer = 0.0f;
            return true;
        }
    }

    for (int k = GLFW_KEY_0; k <= GLFW_KEY_9; ++k) {
        if (checkKeyInput(k) && m_renameInputText.length() < 30) {
            m_renameInputText += static_cast<char>('0' + (k - GLFW_KEY_0));
            m_cursorBlinkTimer = 0.0f;
            return true;
        }
    }

    if ((checkKeyInput(GLFW_KEY_SPACE) || checkKeyInput(GLFW_KEY_MINUS)) && m_renameInputText.length() < 30) {
        m_renameInputText += '_';
        m_cursorBlinkTimer = 0.0f;
        return true;
    }

    if (leftDown && !m_mousePressedLastFrame) {
        if (isPointInRect(mousePos, btnSavePos, btnSaveSize)) {
            confirmRename();
            return true;
        }
        if (isPointInRect(mousePos, btnCancelPos, btnCancelSize)) {
            m_isRenameModalOpen = false;
            return true;
        }
        if (!isPointInRect(mousePos, modalPos, modalSize)) {
            m_isRenameModalOpen = false;
            return true;
        }
    }

    return true;
}

void LevelSelectState::processInput(GLFWwindow* window, float dt) {
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    m_mousePos = glm::vec2(static_cast<float>(mouseX), static_cast<float>(mouseY));

    int mouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    bool leftDown = (mouseState == GLFW_PRESS);

    if (m_isRenameModalOpen) {
        bool handled = processRenameModalInput(window, m_mousePos, leftDown, dt);
        m_mousePressedLastFrame = leftDown;
        if (handled) return;
    }

    // Обновляем Hover/Active состояния
    for (auto& card : m_levelCards) {
        if (!card.isBuiltIn && isPointInRect(m_mousePos, card.renameBtn.pos, card.renameBtn.size)) {
            card.renameBtn.state = leftDown ? 2 : 1;
            card.cardBtn.state = 0;
        } else if (isPointInRect(m_mousePos, card.cardBtn.pos, card.cardBtn.size)) {
            card.cardBtn.state = leftDown ? 2 : 1;
            if (!card.isBuiltIn) card.renameBtn.state = 0;
        } else {
            card.cardBtn.state = 0;
            if (!card.isBuiltIn) card.renameBtn.state = 0;
        }
    }

    if (isPointInRect(m_mousePos, m_btnBack.pos, m_btnBack.size)) {
        m_btnBack.state = leftDown ? 2 : 1;
    } else {
        m_btnBack.state = 0;
    }

    if (isPointInRect(m_mousePos, m_btnEditor.pos, m_btnEditor.size)) {
        m_btnEditor.state = leftDown ? 2 : 1;
    } else {
        m_btnEditor.state = 0;
    }

    if (m_totalPages > 1) {
        m_btnPrevPage.state = isPointInRect(m_mousePos, m_btnPrevPage.pos, m_btnPrevPage.size) ? (leftDown ? 2 : 1) : 0;
        m_btnNextPage.state = isPointInRect(m_mousePos, m_btnNextPage.pos, m_btnNextPage.size) ? (leftDown ? 2 : 1) : 0;
    }

    // Обработка клика
    if (leftDown && !m_mousePressedLastFrame) {
        m_mousePressedLastFrame = true;

        if (m_btnBack.state == 2) {
            m_stateManager.setState(std::make_unique<MainMenuState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer));
            return;
        }

        if (m_btnEditor.state == 2) {
            m_stateManager.setState(std::make_unique<MapEditorState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer));
            return;
        }

        if (m_totalPages > 1) {
            if (m_btnPrevPage.state == 2 && m_currentPage > 0) {
                m_currentPage--;
                init();
                return;
            }
            if (m_btnNextPage.state == 2 && m_currentPage < m_totalPages - 1) {
                m_currentPage++;
                init();
                return;
            }
        }

        for (const auto& card : m_levelCards) {
            if (!card.isBuiltIn && card.renameBtn.state == 2) {
                openRenameModal(card.filename);
                return;
            }

            if (card.cardBtn.state == 2) {
                m_stateManager.setState(std::make_unique<GameplayState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, card.levelPath));
                return;
            }
        }
    }
    else if (!leftDown) {
        m_mousePressedLastFrame = false;
    }
}

void LevelSelectState::update(float dt) {
    if (m_isRenameModalOpen) {
        m_cursorBlinkTimer += dt;
        if (m_cursorBlinkTimer >= 1.0f) {
            m_cursorBlinkTimer -= 1.0f;
        }
    }
}

void LevelSelectState::renderRenameModal() {
    m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(0.0f), glm::vec2(m_width, m_height), 0.0f, glm::vec4(0.04f, 0.05f, 0.07f, 0.75f));

    glm::vec2 modalSize(440.0f, 210.0f);
    glm::vec2 modalPos((static_cast<float>(m_width) - modalSize.x) * 0.5f, (static_cast<float>(m_height) - modalSize.y) * 0.5f);

    m_renderer->drawSprite(m_whiteTexture, modalPos, modalSize, 0.0f, glm::vec3(0.12f, 0.13f, 0.17f));
    m_renderer->drawSprite(m_whiteTexture, modalPos, glm::vec2(modalSize.x, 1.0f), 0.0f, glm::vec3(0.35f, 0.85f, 1.0f));
    m_renderer->drawSprite(m_whiteTexture, modalPos + glm::vec2(0.0f, modalSize.y - 1.0f), glm::vec2(modalSize.x, 1.0f), 0.0f, glm::vec3(0.35f, 0.85f, 1.0f));
    m_renderer->drawSprite(m_whiteTexture, modalPos, glm::vec2(1.0f, modalSize.y), 0.0f, glm::vec3(0.35f, 0.85f, 1.0f));
    m_renderer->drawSprite(m_whiteTexture, modalPos + glm::vec2(modalSize.x - 1.0f, 0.0f), glm::vec2(1.0f, modalSize.y), 0.0f, glm::vec3(0.35f, 0.85f, 1.0f));

    float headerH = 38.0f;
    m_renderer->drawSprite(m_whiteTexture, modalPos, glm::vec2(modalSize.x, headerH), 0.0f, glm::vec3(0.16f, 0.18f, 0.24f));
    m_renderer->drawSprite(m_whiteTexture, modalPos + glm::vec2(0.0f, headerH), glm::vec2(modalSize.x, 2.0f), 0.0f, glm::vec3(0.35f, 0.85f, 1.0f));

    glm::vec2 boxPos(modalPos.x + 30.0f, modalPos.y + 88.0f);
    glm::vec2 boxSize(modalSize.x - 60.0f, 38.0f);
    m_renderer->drawSprite(m_whiteTexture, boxPos, boxSize, 0.0f, glm::vec3(0.35f, 0.85f, 1.0f));
    m_renderer->drawSprite(m_whiteTexture, boxPos + glm::vec2(1.0f), boxSize - glm::vec2(2.0f), 0.0f, glm::vec3(0.08f, 0.10f, 0.14f));

    glm::vec2 btnSavePos(modalPos.x + 30.0f, modalPos.y + 148.0f);
    glm::vec2 btnSaveSize(190.0f, 40.0f);
    bool hovSave = isPointInRect(m_mousePos, btnSavePos, btnSaveSize);
    m_renderer->drawSprite(m_whiteTexture, btnSavePos, btnSaveSize, 0.0f, hovSave ? glm::vec3(0.3f, 0.9f, 0.45f) : glm::vec3(0.2f, 0.7f, 0.35f));
    m_renderer->drawSprite(m_whiteTexture, btnSavePos + glm::vec2(1.0f), btnSaveSize - glm::vec2(2.0f), 0.0f, hovSave ? glm::vec3(0.18f, 0.38f, 0.22f) : glm::vec3(0.14f, 0.30f, 0.18f));

    glm::vec2 btnCancelPos(modalPos.x + 240.0f, modalPos.y + 148.0f);
    glm::vec2 btnCancelSize(170.0f, 40.0f);
    bool hovCancel = isPointInRect(m_mousePos, btnCancelPos, btnCancelSize);
    m_renderer->drawSprite(m_whiteTexture, btnCancelPos, btnCancelSize, 0.0f, hovCancel ? glm::vec3(0.6f, 0.25f, 0.25f) : glm::vec3(0.45f, 0.20f, 0.20f));
    m_renderer->drawSprite(m_whiteTexture, btnCancelPos + glm::vec2(1.0f), btnCancelSize - glm::vec2(2.0f), 0.0f, hovCancel ? glm::vec3(0.28f, 0.14f, 0.14f) : glm::vec3(0.22f, 0.11f, 0.11f));

    m_renderer->flush();

    if (m_textRenderer) {
        float titleW = m_textRenderer->CalculateTextWidth("RENAME MAP", 0.70f);
        m_textRenderer->RenderText("RENAME MAP", modalPos.x + (modalSize.x - titleW) * 0.5f, modalPos.y + 10.0f, 0.70f, glm::vec3(1.0f, 0.85f, 0.25f));

        std::string sub = "Old: " + m_renameTargetFileName;
        m_textRenderer->RenderText(sub, modalPos.x + 32.0f, modalPos.y + 55.0f, 0.50f, glm::vec3(0.70f, 0.75f, 0.85f));

        bool showCursor = (m_cursorBlinkTimer < 0.5f);
        std::string displayText = m_renameInputText + (showCursor ? "|" : "");
        m_textRenderer->RenderText(displayText, boxPos.x + 12.0f, boxPos.y + 10.0f, 0.62f, glm::vec3(0.40f, 0.95f, 1.0f));

        float sTxtW = m_textRenderer->CalculateTextWidth("Save (Enter)", 0.55f);
        m_textRenderer->RenderText("Save (Enter)", btnSavePos.x + (btnSaveSize.x - sTxtW) * 0.5f, btnSavePos.y + 12.0f, 0.55f, glm::vec3(0.95f));

        float cTxtW = m_textRenderer->CalculateTextWidth("Cancel (Esc)", 0.55f);
        m_textRenderer->RenderText("Cancel (Esc)", btnCancelPos.x + (btnCancelSize.x - cTxtW) * 0.5f, btnCancelPos.y + 12.0f, 0.55f, glm::vec3(0.95f));
    }
}

void LevelSelectState::render() {
    m_renderer->beginBatch();

    // 1. Фон
    m_renderer->drawSprite(m_uiTexture, glm::vec2(0.0f), glm::vec2(m_width, m_height), 0.0f, glm::vec3(0.08f, 0.08f, 0.10f));

    // 2. Карточки уровней
    for (const auto& card : m_levelCards) {
        glm::vec3 cardBg = (card.cardBtn.state == 2) ? glm::vec3(0.12f, 0.18f, 0.26f) :
                           (card.cardBtn.state == 1) ? glm::vec3(0.22f, 0.30f, 0.42f) : glm::vec3(0.15f, 0.19f, 0.28f);

        glm::vec3 cardBorder = card.isBuiltIn ? glm::vec3(0.35f, 0.75f, 1.0f) : glm::vec3(1.0f, 0.82f, 0.25f);
        if (card.cardBtn.state == 1) cardBorder *= 1.25f;

        m_renderer->drawSprite(m_whiteTexture, card.cardBtn.pos, card.cardBtn.size, 0.0f, cardBorder);
        m_renderer->drawSprite(m_whiteTexture, card.cardBtn.pos + glm::vec2(2.0f), card.cardBtn.size - glm::vec2(4.0f), 0.0f, cardBg);

        // Бейдж типа карты вверху карточки
        glm::vec2 badgePos = card.cardBtn.pos + glm::vec2(12.0f, 12.0f);
        glm::vec2 badgeSize(card.isBuiltIn ? 82.0f : 96.0f, 20.0f);
        glm::vec3 badgeBg = card.isBuiltIn ? glm::vec3(0.12f, 0.35f, 0.55f) : glm::vec3(0.48f, 0.36f, 0.12f);
        m_renderer->drawSprite(m_whiteTexture, badgePos, badgeSize, 0.0f, badgeBg);

        // Кнопка Rename (для некастомных карт)
        if (!card.isBuiltIn) {
            glm::vec3 renBg = (card.renameBtn.state == 2) ? glm::vec3(0.30f, 0.22f, 0.08f) :
                              (card.renameBtn.state == 1) ? glm::vec3(0.55f, 0.45f, 0.15f) : glm::vec3(0.38f, 0.30f, 0.12f);
            m_renderer->drawSprite(m_whiteTexture, card.renameBtn.pos, card.renameBtn.size, 0.0f, glm::vec3(0.95f, 0.80f, 0.25f));
            m_renderer->drawSprite(m_whiteTexture, card.renameBtn.pos + glm::vec2(1.0f), card.renameBtn.size - glm::vec2(2.0f), 0.0f, renBg);
        }
    }

    // 3. Пагинация
    if (m_totalPages > 1) {
        auto drawNavBtn = [&](const UIButton& btn) {
            glm::vec3 bg = (btn.state == 2) ? glm::vec3(0.15f, 0.20f, 0.28f) :
                           (btn.state == 1) ? glm::vec3(0.28f, 0.38f, 0.52f) : glm::vec3(0.20f, 0.26f, 0.36f);
            m_renderer->drawSprite(m_whiteTexture, btn.pos, btn.size, 0.0f, glm::vec3(0.40f, 0.70f, 1.0f));
            m_renderer->drawSprite(m_whiteTexture, btn.pos + glm::vec2(1.0f), btn.size - glm::vec2(2.0f), 0.0f, bg);
        };
        drawNavBtn(m_btnPrevPage);
        drawNavBtn(m_btnNextPage);
    }

    // 4. Нижние кнопки
    auto drawBtn = [&](const UIButton& btn, glm::vec3 border, glm::vec3 bgNormal) {
        glm::vec3 bg = (btn.state == 2) ? (bgNormal * 0.7f) :
                       (btn.state == 1) ? (bgNormal * 1.3f) : bgNormal;
        m_renderer->drawSprite(m_whiteTexture, btn.pos, btn.size, 0.0f, border);
        m_renderer->drawSprite(m_whiteTexture, btn.pos + glm::vec2(2.0f), btn.size - glm::vec2(4.0f), 0.0f, bg);
    };

    drawBtn(m_btnBack, glm::vec3(0.55f, 0.60f, 0.70f), glm::vec3(0.22f, 0.25f, 0.30f));
    drawBtn(m_btnEditor, glm::vec3(1.0f, 0.85f, 0.25f), glm::vec3(0.35f, 0.30f, 0.12f));

    m_renderer->flush();

    // 5. Текстовая отрисовка
    if (m_textRenderer) {
        float titleW = m_textRenderer->CalculateTextWidth("SELECT LEVEL", 1.35f);
        m_textRenderer->RenderText("SELECT LEVEL", (m_width - titleW) * 0.5f, 45.0f, 1.35f, glm::vec3(1.0f, 0.85f, 0.25f));

        for (const auto& card : m_levelCards) {
            // Текст бейджа
            std::string badgeText = card.isBuiltIn ? "CAMPAIGN" : "CUSTOM MAP";
            glm::vec3 badgeCol = card.isBuiltIn ? glm::vec3(0.45f, 0.90f, 1.0f) : glm::vec3(1.0f, 0.90f, 0.35f);
            m_textRenderer->RenderText(badgeText, card.cardBtn.pos.x + 18.0f, card.cardBtn.pos.y + 16.0f, 0.42f, badgeCol);

            // Название карты
            float nameW = m_textRenderer->CalculateTextWidth(card.displayName, 0.75f);
            float nameX = card.cardBtn.pos.x + (card.cardBtn.size.x - nameW) * 0.5f;
            glm::vec3 nameCol = card.isBuiltIn ? glm::vec3(1.0f) : glm::vec3(1.0f, 0.95f, 0.75f);
            m_textRenderer->RenderText(card.displayName, nameX, card.cardBtn.pos.y + 58.0f, 0.75f, nameCol);

            // Имя файла
            float fileW = m_textRenderer->CalculateTextWidth(card.filename, 0.44f);
            float fileX = card.cardBtn.pos.x + (card.cardBtn.size.x - fileW) * 0.5f;
            m_textRenderer->RenderText(card.filename, fileX, card.cardBtn.pos.y + 88.0f, 0.44f, glm::vec3(0.60f, 0.68f, 0.78f));

            // Текст на кнопке Rename
            if (!card.isBuiltIn) {
                float renW = m_textRenderer->CalculateTextWidth("Rename", 0.44f);
                m_textRenderer->RenderText("Rename", card.renameBtn.pos.x + (card.renameBtn.size.x - renW) * 0.5f, card.renameBtn.pos.y + 6.0f, 0.44f, glm::vec3(1.0f, 0.92f, 0.65f));
            }
        }

        // Пагинация
        if (m_totalPages > 1) {
            float pTxtW = m_textRenderer->CalculateTextWidth(m_btnPrevPage.text, 0.52f);
            m_textRenderer->RenderText(m_btnPrevPage.text, m_btnPrevPage.pos.x + (m_btnPrevPage.size.x - pTxtW) * 0.5f, m_btnPrevPage.pos.y + 10.0f, 0.52f, glm::vec3(0.95f));

            float nTxtW = m_textRenderer->CalculateTextWidth(m_btnNextPage.text, 0.52f);
            m_textRenderer->RenderText(m_btnNextPage.text, m_btnNextPage.pos.x + (m_btnNextPage.size.x - nTxtW) * 0.5f, m_btnNextPage.pos.y + 10.0f, 0.52f, glm::vec3(0.95f));

            std::string pageStr = "Page " + std::to_string(m_currentPage + 1) + " / " + std::to_string(m_totalPages);
            float pageW = m_textRenderer->CalculateTextWidth(pageStr, 0.56f);
            m_textRenderer->RenderText(pageStr, (m_width - pageW) * 0.5f, m_btnPrevPage.pos.y + 10.0f, 0.56f, glm::vec3(0.85f, 0.88f, 0.95f));
        }

        // Текст на нижних кнопках
        float backW = m_textRenderer->CalculateTextWidth(m_btnBack.text, 0.70f);
        m_textRenderer->RenderText(m_btnBack.text, m_btnBack.pos.x + (m_btnBack.size.x - backW) * 0.5f, m_btnBack.pos.y + 13.0f, 0.70f, glm::vec3(0.95f));

        float editW = m_textRenderer->CalculateTextWidth(m_btnEditor.text, 0.70f);
        m_textRenderer->RenderText(m_btnEditor.text, m_btnEditor.pos.x + (m_btnEditor.size.x - editW) * 0.5f, m_btnEditor.pos.y + 13.0f, 0.70f, glm::vec3(1.0f, 0.90f, 0.35f));
    }

    m_renderer->endBatch();

    // 6. Отрисовка модального окна переименования (если открыто)
    if (m_isRenameModalOpen) {
        m_renderer->beginBatch();
        renderRenameModal();
        m_renderer->endBatch();
    }
}

void LevelSelectState::resize(int width, int height) {
    m_width = width;
    m_height = height;
    init();
}