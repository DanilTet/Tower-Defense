#include "EditorLevelSettingsModal.h"
#include "../../renderer/SpriteRenderer.h"
#include "../../renderer/TextRenderer.h"
#include "../../textures/Texture2D.h"
#include "../ui/UICommon.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <iostream>

bool EditorLevelSettingsModal::isPointInRect(glm::vec2 p, glm::vec2 pos, glm::vec2 size) {
    return p.x >= pos.x && p.x <= pos.x + size.x &&
           p.y >= pos.y && p.y <= pos.y + size.y;
}

void EditorLevelSettingsModal::open(int currentMoney, int currentHealth,
                                    const std::vector<std::string>& allowedTowers,
                                    const std::unordered_map<std::string, int>& towerMaxTiers)
{
    m_isOpen = true;
    m_justOpened = true;
    m_wasLeftDown = true;
    m_focusedField = FocusedField::None;
    m_cursorBlinkTimer = 0.0f;
    m_moneyStr = std::to_string(std::max(0, currentMoney));
    m_healthStr = std::to_string(std::max(1, currentHealth));
    m_allowedTowers = allowedTowers.empty() ? std::vector<std::string>{"Basic", "Mercury", "Piston"} : allowedTowers;
    m_towerMaxTiers = { { "Basic", 3 }, { "Mercury", 3 }, { "Piston", 3 } };
    for (const auto& kv : towerMaxTiers) {
        m_towerMaxTiers[kv.first] = std::clamp(kv.second, 1, 3);
    }
    m_keyStates.clear();
}

void EditorLevelSettingsModal::close() {
    m_isOpen = false;
    m_justOpened = false;
    m_focusedField = FocusedField::None;
    m_keyStates.clear();
}

void EditorLevelSettingsModal::toggle(int currentMoney, int currentHealth,
                                     const std::vector<std::string>& allowedTowers,
                                     const std::unordered_map<std::string, int>& towerMaxTiers)
{
    if (m_isOpen) {
        close();
    } else {
        open(currentMoney, currentHealth, allowedTowers, towerMaxTiers);
    }
}

void EditorLevelSettingsModal::update(float dt) {
    if (!m_isOpen) return;
    m_cursorBlinkTimer += dt;
    if (m_cursorBlinkTimer > 1.0f) {
        m_cursorBlinkTimer -= 1.0f;
    }
}

bool EditorLevelSettingsModal::checkKeyRepeat(GLFWwindow* window, int key, float dt) {
    if (!window) return false;
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
            if (ks.holdTimer >= 0.30f) {
                ks.repeatTimer += dt;
                if (ks.repeatTimer >= 0.06f) {
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
}

bool EditorLevelSettingsModal::handleInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown,
                                          bool leftReleased, bool isEscJustPressed, float dt,
                                          int screenWidth, int screenHeight,
                                          int& outMoney, int& outHealth,
                                          std::vector<std::string>& outAllowedTowers,
                                          std::unordered_map<std::string, int>& outTowerMaxTiers,
                                          int& outMaxUpgradeTier, bool& outDirty)
{
    if (!m_isOpen) return false;

    if (isEscJustPressed || checkKeyRepeat(window, GLFW_KEY_ESCAPE, dt)) {
        close();
        return true;
    }

    float sw = static_cast<float>(screenWidth > 0 ? screenWidth : 1280);
    float sh = static_cast<float>(screenHeight > 0 ? screenHeight : 720);
    float scale = GetUIScale(static_cast<int>(sw), static_cast<int>(sh));

    float modalW = std::clamp(540.0f * scale, 480.0f, sw - 30.0f);
    float modalH = std::clamp(510.0f * scale, 460.0f, sh - 30.0f);
    glm::vec2 modalPos((sw - modalW) * 0.5f, (sh - modalH) * 0.5f);

    float headerH = 42.0f * scale;
    glm::vec2 crossPos(modalPos.x + modalW - 38.0f * scale, modalPos.y + 7.0f * scale);
    glm::vec2 crossSize(28.0f * scale, 28.0f * scale);

    // Геометрия полей ввода
    float contentLeft = modalPos.x + 32.0f * scale;
    float contentW = modalW - 64.0f * scale;

    float moneyLabelY = modalPos.y + headerH + 16.0f * scale;
    glm::vec2 moneyBoxPos(contentLeft, moneyLabelY + 20.0f * scale);
    glm::vec2 moneyBoxSize(contentW, 32.0f * scale);

    float healthLabelY = moneyBoxPos.y + moneyBoxSize.y + 12.0f * scale;
    glm::vec2 healthBoxPos(contentLeft, healthLabelY + 20.0f * scale);
    glm::vec2 healthBoxSize(contentW, 32.0f * scale);

    // Геометрия строк башен и тиров
    float towersLabelY = healthBoxPos.y + healthBoxSize.y + 14.0f * scale;
    float towerRowH = 34.0f * scale;
    float rowSpacing = 8.0f * scale;
    float tierBtnW = 40.0f * scale;
    float tierBtnH = 26.0f * scale;
    float tierBtnGap = 6.0f * scale;
    float totalTierGroupW = 3.0f * tierBtnW + 2.0f * tierBtnGap;
    float rowToggleW = contentW - totalTierGroupW - 14.0f * scale;

    float rowBasicY = towersLabelY + 24.0f * scale;
    glm::vec2 rowBasicTogglePos(contentLeft, rowBasicY);
    glm::vec2 rowBasicToggleSize(rowToggleW, towerRowH);
    glm::vec2 basicT1Pos(contentLeft + contentW - totalTierGroupW, rowBasicY + (towerRowH - tierBtnH) * 0.5f);
    glm::vec2 basicT2Pos(basicT1Pos.x + tierBtnW + tierBtnGap, basicT1Pos.y);
    glm::vec2 basicT3Pos(basicT2Pos.x + tierBtnW + tierBtnGap, basicT1Pos.y);

    float rowMercuryY = rowBasicY + towerRowH + rowSpacing;
    glm::vec2 rowMercuryTogglePos(contentLeft, rowMercuryY);
    glm::vec2 rowMercuryToggleSize(rowToggleW, towerRowH);
    glm::vec2 mercuryT1Pos(contentLeft + contentW - totalTierGroupW, rowMercuryY + (towerRowH - tierBtnH) * 0.5f);
    glm::vec2 mercuryT2Pos(mercuryT1Pos.x + tierBtnW + tierBtnGap, mercuryT1Pos.y);
    glm::vec2 mercuryT3Pos(mercuryT2Pos.x + tierBtnW + tierBtnGap, mercuryT1Pos.y);

    float rowPistonY = rowMercuryY + towerRowH + rowSpacing;
    glm::vec2 rowPistonTogglePos(contentLeft, rowPistonY);
    glm::vec2 rowPistonToggleSize(rowToggleW, towerRowH);
    glm::vec2 pistonT1Pos(contentLeft + contentW - totalTierGroupW, rowPistonY + (towerRowH - tierBtnH) * 0.5f);
    glm::vec2 pistonT2Pos(pistonT1Pos.x + tierBtnW + tierBtnGap, pistonT1Pos.y);
    glm::vec2 pistonT3Pos(pistonT2Pos.x + tierBtnW + tierBtnGap, pistonT1Pos.y);

    glm::vec2 tierBtnSize(tierBtnW, tierBtnH);

    // Геометрия нижних кнопок
    float btnH = 34.0f * scale;
    float btnY = modalPos.y + modalH - btnH - 16.0f * scale;
    float saveBtnW = 160.0f * scale;
    float cancelBtnW = 130.0f * scale;
    float btnGap = 20.0f * scale;
    float totalBtnW = saveBtnW + cancelBtnW + btnGap;
    glm::vec2 btnSavePos(modalPos.x + (modalW - totalBtnW) * 0.5f, btnY);
    glm::vec2 btnSaveSize(saveBtnW, btnH);
    glm::vec2 btnCancelPos(btnSavePos.x + saveBtnW + btnGap, btnY);
    glm::vec2 btnCancelSize(cancelBtnW, btnH);

    // Клавиатурный фокус и Tab
    if (checkKeyRepeat(window, GLFW_KEY_TAB, dt)) {
        if (m_focusedField == FocusedField::Money) {
            m_focusedField = FocusedField::Health;
        } else if (m_focusedField == FocusedField::Health) {
            m_focusedField = FocusedField::Money;
        } else {
            m_focusedField = FocusedField::Money;
        }
        m_cursorBlinkTimer = 0.0f;
    }

    // Сохранение по Enter
    auto commitAndSave = [&]() {
        int mVal = 50;
        try {
            if (!m_moneyStr.empty()) mVal = std::max(0, std::stoi(m_moneyStr));
        } catch (...) { mVal = 50; }

        int hVal = 20;
        try {
            if (!m_healthStr.empty()) hVal = std::max(1, std::stoi(m_healthStr));
        } catch (...) { hVal = 20; }

        if (m_allowedTowers.empty()) {
            m_allowedTowers = { "Basic", "Mercury", "Piston" };
        }

        outMoney = mVal;
        outHealth = hVal;
        outAllowedTowers = m_allowedTowers;
        outTowerMaxTiers = m_towerMaxTiers;
        int maxT = 1;
        for (const auto& kv : m_towerMaxTiers) {
            maxT = std::max(maxT, kv.second);
        }
        outMaxUpgradeTier = maxT;
        outDirty = true;
        close();
    };

    if (checkKeyRepeat(window, GLFW_KEY_ENTER, dt) || checkKeyRepeat(window, GLFW_KEY_KP_ENTER, dt)) {
        commitAndSave();
        return true;
    }

    // Текстовый ввод для активного поля
    if (m_focusedField != FocusedField::None) {
        std::string& targetStr = (m_focusedField == FocusedField::Money) ? m_moneyStr : m_healthStr;
        size_t maxLen = (m_focusedField == FocusedField::Money) ? 7 : 4;

        if (checkKeyRepeat(window, GLFW_KEY_BACKSPACE, dt)) {
            if (!targetStr.empty()) {
                targetStr.pop_back();
                m_cursorBlinkTimer = 0.0f;
            }
        }

        for (int d = 0; d <= 9; ++d) {
            if (checkKeyRepeat(window, GLFW_KEY_0 + d, dt) || checkKeyRepeat(window, GLFW_KEY_KP_0 + d, dt)) {
                if (targetStr == "0") {
                    targetStr.clear();
                }
                if (targetStr.length() < maxLen) {
                    targetStr += std::to_string(d);
                    m_cursorBlinkTimer = 0.0f;
                }
            }
        }
    }

    // Обработка клика мыши: строго одиночный клик по переходу (lmbJustPressed)
    if (m_justOpened) {
        if (!leftDown) {
            m_justOpened = false;
        }
        m_wasLeftDown = leftDown;
        return true;
    }

    bool lmbJustPressed = leftDown && !m_wasLeftDown;
    m_wasLeftDown = leftDown;

    if (!lmbJustPressed) {
        return true;
    }

    // Клик по крестику
    if (isPointInRect(mousePos, crossPos, crossSize)) {
        close();
        return true;
    }

    // Клик по [ Отмена ]
    if (isPointInRect(mousePos, btnCancelPos, btnCancelSize)) {
        close();
        return true;
    }

    // Клик по [ Сохранить ]
    if (isPointInRect(mousePos, btnSavePos, btnSaveSize)) {
        commitAndSave();
        return true;
    }

    // Фокус полей ввода
    if (isPointInRect(mousePos, moneyBoxPos, moneyBoxSize)) {
        m_focusedField = FocusedField::Money;
        m_cursorBlinkTimer = 0.0f;
        return true;
    }

    if (isPointInRect(mousePos, healthBoxPos, healthBoxSize)) {
        m_focusedField = FocusedField::Health;
        m_cursorBlinkTimer = 0.0f;
        return true;
    }

    // Переключение чекбоксов башен
    auto toggleTower = [this](const std::string& type) {
        auto it = std::find(m_allowedTowers.begin(), m_allowedTowers.end(), type);
        if (it != m_allowedTowers.end()) {
            if (m_allowedTowers.size() > 1) {
                m_allowedTowers.erase(it);
            }
        } else {
            m_allowedTowers.push_back(type);
        }
    };

    auto setTowerTier = [this](const std::string& type, int tier) {
        m_towerMaxTiers[type] = tier;
    };

    // Строка Basic: клик по названию/чекбоксу или по тирам T1, T2, T3
    if (isPointInRect(mousePos, rowBasicTogglePos, rowBasicToggleSize)) {
        toggleTower("Basic");
        m_focusedField = FocusedField::None;
        return true;
    }
    if (isPointInRect(mousePos, basicT1Pos, tierBtnSize)) {
        setTowerTier("Basic", 1);
        m_focusedField = FocusedField::None;
        return true;
    }
    if (isPointInRect(mousePos, basicT2Pos, tierBtnSize)) {
        setTowerTier("Basic", 2);
        m_focusedField = FocusedField::None;
        return true;
    }
    if (isPointInRect(mousePos, basicT3Pos, tierBtnSize)) {
        setTowerTier("Basic", 3);
        m_focusedField = FocusedField::None;
        return true;
    }

    // Строка Mercury: клик по названию/чекбоксу или по тирам T1, T2, T3
    if (isPointInRect(mousePos, rowMercuryTogglePos, rowMercuryToggleSize)) {
        toggleTower("Mercury");
        m_focusedField = FocusedField::None;
        return true;
    }
    if (isPointInRect(mousePos, mercuryT1Pos, tierBtnSize)) {
        setTowerTier("Mercury", 1);
        m_focusedField = FocusedField::None;
        return true;
    }
    if (isPointInRect(mousePos, mercuryT2Pos, tierBtnSize)) {
        setTowerTier("Mercury", 2);
        m_focusedField = FocusedField::None;
        return true;
    }
    if (isPointInRect(mousePos, mercuryT3Pos, tierBtnSize)) {
        setTowerTier("Mercury", 3);
        m_focusedField = FocusedField::None;
        return true;
    }

    // Строка Piston: клик по названию/чекбоксу или по тирам T1, T2, T3
    if (isPointInRect(mousePos, rowPistonTogglePos, rowPistonToggleSize)) {
        toggleTower("Piston");
        m_focusedField = FocusedField::None;
        return true;
    }
    if (isPointInRect(mousePos, pistonT1Pos, tierBtnSize)) {
        setTowerTier("Piston", 1);
        m_focusedField = FocusedField::None;
        return true;
    }
    if (isPointInRect(mousePos, pistonT2Pos, tierBtnSize)) {
        setTowerTier("Piston", 2);
        m_focusedField = FocusedField::None;
        return true;
    }
    if (isPointInRect(mousePos, pistonT3Pos, tierBtnSize)) {
        setTowerTier("Piston", 3);
        m_focusedField = FocusedField::None;
        return true;
    }

    // Клик внутри окна сбрасывает фокус с полей ввода
    if (isPointInRect(mousePos, modalPos, glm::vec2(modalW, modalH))) {
        m_focusedField = FocusedField::None;
        return true;
    }

    // Клик вне окна закрывает его
    close();
    return true;
}

void EditorLevelSettingsModal::render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                                     const std::shared_ptr<Texture2D>& whiteTexture,
                                     int screenWidth, int screenHeight, glm::vec2 mousePos)
{
    if (!m_isOpen || !renderer || !whiteTexture) return;

    float sw = static_cast<float>(screenWidth);
    float sh = static_cast<float>(screenHeight);
    float scale = GetUIScale(screenWidth, screenHeight);

    // 1. Полупрозрачная затемняющая подложка
    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(0.0f), glm::vec2(sw, sh), 0.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.72f));

    // 2. Размеры и позиция модалки
    float modalW = std::clamp(540.0f * scale, 480.0f, sw - 30.0f);
    float modalH = std::clamp(510.0f * scale, 460.0f, sh - 30.0f);
    glm::vec2 modalPos((sw - modalW) * 0.5f, (sh - modalH) * 0.5f);

    // Рамка и фон модалки
    renderer->drawSprite(whiteTexture, modalPos, glm::vec2(modalW, modalH), 0.0f, glm::vec3(0.35f, 0.48f, 0.70f));
    renderer->drawSprite(whiteTexture, modalPos + glm::vec2(2.0f), glm::vec2(modalW - 4.0f, modalH - 4.0f), 0.0f, glm::vec3(0.10f, 0.12f, 0.16f));

    // 3. Шапка
    float headerH = 42.0f * scale;
    renderer->drawSprite(whiteTexture, modalPos + glm::vec2(2.0f), glm::vec2(modalW - 4.0f, headerH), 0.0f, glm::vec3(0.14f, 0.17f, 0.24f));
    renderer->drawSprite(whiteTexture, glm::vec2(modalPos.x + 2.0f, modalPos.y + headerH + 1.0f), glm::vec2(modalW - 4.0f, 1.5f), 0.0f, glm::vec3(0.28f, 0.38f, 0.52f));

    // Крестик [X]
    glm::vec2 crossPos(modalPos.x + modalW - 38.0f * scale, modalPos.y + 7.0f * scale);
    glm::vec2 crossSize(28.0f * scale, 28.0f * scale);
    bool crossHover = isPointInRect(mousePos, crossPos, crossSize);
    renderer->drawSprite(whiteTexture, crossPos, crossSize, 0.0f, crossHover ? glm::vec3(0.85f, 0.25f, 0.25f) : glm::vec3(0.25f, 0.30f, 0.40f));
    renderer->drawSprite(whiteTexture, crossPos + glm::vec2(1.5f), crossSize - glm::vec2(3.0f), 0.0f, glm::vec3(0.18f, 0.22f, 0.30f));

    // Геометрия контента
    float contentLeft = modalPos.x + 32.0f * scale;
    float contentW = modalW - 64.0f * scale;

    // Секция 1: Стартовые деньги
    float moneyLabelY = modalPos.y + headerH + 16.0f * scale;
    glm::vec2 moneyBoxPos(contentLeft, moneyLabelY + 20.0f * scale);
    glm::vec2 moneyBoxSize(contentW, 32.0f * scale);
    bool moneyFocused = (m_focusedField == FocusedField::Money);
    glm::vec3 moneyBorder = moneyFocused ? glm::vec3(1.0f, 0.84f, 0.25f) : glm::vec3(0.28f, 0.34f, 0.46f);
    renderer->drawSprite(whiteTexture, moneyBoxPos, moneyBoxSize, 0.0f, moneyBorder);
    renderer->drawSprite(whiteTexture, moneyBoxPos + glm::vec2(1.5f), moneyBoxSize - glm::vec2(3.0f), 0.0f, glm::vec3(0.06f, 0.08f, 0.11f));

    // Секция 2: Стартовое здоровье
    float healthLabelY = moneyBoxPos.y + moneyBoxSize.y + 12.0f * scale;
    glm::vec2 healthBoxPos(contentLeft, healthLabelY + 20.0f * scale);
    glm::vec2 healthBoxSize(contentW, 32.0f * scale);
    bool healthFocused = (m_focusedField == FocusedField::Health);
    glm::vec3 healthBorder = healthFocused ? glm::vec3(1.0f, 0.84f, 0.25f) : glm::vec3(0.28f, 0.34f, 0.46f);
    renderer->drawSprite(whiteTexture, healthBoxPos, healthBoxSize, 0.0f, healthBorder);
    renderer->drawSprite(whiteTexture, healthBoxPos + glm::vec2(1.5f), healthBoxSize - glm::vec2(3.0f), 0.0f, glm::vec3(0.06f, 0.08f, 0.11f));

    // Секция 3: Разрешенные башни и тиры
    float towersLabelY = healthBoxPos.y + healthBoxSize.y + 14.0f * scale;
    float towerRowH = 34.0f * scale;
    float rowSpacing = 8.0f * scale;
    float cbSize = 20.0f * scale;
    float tierBtnW = 40.0f * scale;
    float tierBtnH = 26.0f * scale;
    float tierBtnGap = 6.0f * scale;
    float totalTierGroupW = 3.0f * tierBtnW + 2.0f * tierBtnGap;

    float rowBasicY = towersLabelY + 24.0f * scale;
    float rowMercuryY = rowBasicY + towerRowH + rowSpacing;
    float rowPistonY = rowMercuryY + towerRowH + rowSpacing;

    auto hasTower = [this](const std::string& type) {
        return std::find(m_allowedTowers.begin(), m_allowedTowers.end(), type) != m_allowedTowers.end();
    };

    auto renderCheckbox = [&](glm::vec2 cbPos, bool checked) {
        renderer->drawSprite(whiteTexture, cbPos, glm::vec2(cbSize), 0.0f, checked ? glm::vec3(0.3f, 0.85f, 0.45f) : glm::vec3(0.35f, 0.40f, 0.50f));
        renderer->drawSprite(whiteTexture, cbPos + glm::vec2(1.5f), glm::vec2(cbSize - 3.0f), 0.0f, glm::vec3(0.08f, 0.10f, 0.14f));
        if (checked) {
            renderer->drawSprite(whiteTexture, cbPos + glm::vec2(4.0f * scale), glm::vec2(cbSize - 8.0f * scale), 0.0f, glm::vec3(0.25f, 0.90f, 0.40f));
        }
    };

    auto renderRowTierButtons = [&](const std::string& tower, float rowY) {
        bool enabled = hasTower(tower);
        auto it = m_towerMaxTiers.find(tower);
        int currentTier = (it != m_towerMaxTiers.end()) ? it->second : 3;

        float tierBtnY = rowY + (towerRowH - tierBtnH) * 0.5f;
        glm::vec2 t1Pos(contentLeft + contentW - totalTierGroupW, tierBtnY);
        glm::vec2 t2Pos(t1Pos.x + tierBtnW + tierBtnGap, tierBtnY);
        glm::vec2 t3Pos(t2Pos.x + tierBtnW + tierBtnGap, tierBtnY);
        glm::vec2 btnSize(tierBtnW, tierBtnH);

        auto drawBtn = [&](glm::vec2 pos, int tier) {
            bool selected = (currentTier == tier);
            bool hover = isPointInRect(mousePos, pos, btnSize);

            glm::vec3 border;
            glm::vec3 bg;
            if (!enabled) {
                border = glm::vec3(0.22f, 0.25f, 0.30f);
                bg = glm::vec3(0.10f, 0.12f, 0.15f);
            } else if (selected) {
                border = glm::vec3(1.0f, 0.84f, 0.25f);
                bg = glm::vec3(0.20f, 0.38f, 0.65f);
            } else if (hover) {
                border = glm::vec3(0.50f, 0.60f, 0.75f);
                bg = glm::vec3(0.20f, 0.24f, 0.32f);
            } else {
                border = glm::vec3(0.30f, 0.36f, 0.48f);
                bg = glm::vec3(0.14f, 0.17f, 0.23f);
            }

            renderer->drawSprite(whiteTexture, pos, btnSize, 0.0f, border);
            renderer->drawSprite(whiteTexture, pos + glm::vec2(1.5f), btnSize - glm::vec2(3.0f), 0.0f, bg);
        };

        drawBtn(t1Pos, 1);
        drawBtn(t2Pos, 2);
        drawBtn(t3Pos, 3);
    };

    // Чекбоксы и кнопки тиров
    glm::vec2 cbBasicPos(contentLeft + 6.0f * scale, rowBasicY + (towerRowH - cbSize) * 0.5f);
    renderCheckbox(cbBasicPos, hasTower("Basic"));
    renderRowTierButtons("Basic", rowBasicY);

    glm::vec2 cbMercuryPos(contentLeft + 6.0f * scale, rowMercuryY + (towerRowH - cbSize) * 0.5f);
    renderCheckbox(cbMercuryPos, hasTower("Mercury"));
    renderRowTierButtons("Mercury", rowMercuryY);

    glm::vec2 cbPistonPos(contentLeft + 6.0f * scale, rowPistonY + (towerRowH - cbSize) * 0.5f);
    renderCheckbox(cbPistonPos, hasTower("Piston"));
    renderRowTierButtons("Piston", rowPistonY);

    // Секция 4: Нижние кнопки
    float btnH = 34.0f * scale;
    float btnY = modalPos.y + modalH - btnH - 16.0f * scale;
    float saveBtnW = 160.0f * scale;
    float cancelBtnW = 130.0f * scale;
    float btnGap = 20.0f * scale;
    float totalBtnW = saveBtnW + cancelBtnW + btnGap;
    glm::vec2 btnSavePos(modalPos.x + (modalW - totalBtnW) * 0.5f, btnY);
    glm::vec2 btnSaveSize(saveBtnW, btnH);
    glm::vec2 btnCancelPos(btnSavePos.x + saveBtnW + btnGap, btnY);
    glm::vec2 btnCancelSize(cancelBtnW, btnH);

    bool saveHover = isPointInRect(mousePos, btnSavePos, btnSaveSize);
    renderer->drawSprite(whiteTexture, btnSavePos, btnSaveSize, 0.0f, saveHover ? glm::vec3(0.35f, 0.95f, 0.45f) : glm::vec3(0.25f, 0.75f, 0.35f));
    renderer->drawSprite(whiteTexture, btnSavePos + glm::vec2(1.5f), btnSaveSize - glm::vec2(3.0f), 0.0f, saveHover ? glm::vec3(0.18f, 0.48f, 0.24f) : glm::vec3(0.14f, 0.38f, 0.20f));

    bool cancelHover = isPointInRect(mousePos, btnCancelPos, btnCancelSize);
    renderer->drawSprite(whiteTexture, btnCancelPos, btnCancelSize, 0.0f, cancelHover ? glm::vec3(0.70f, 0.75f, 0.85f) : glm::vec3(0.40f, 0.45f, 0.55f));
    renderer->drawSprite(whiteTexture, btnCancelPos + glm::vec2(1.5f), btnCancelSize - glm::vec2(3.0f), 0.0f, cancelHover ? glm::vec3(0.24f, 0.28f, 0.36f) : glm::vec3(0.18f, 0.22f, 0.28f));

    renderer->flush();

    // 5. Текстовый слой
    if (!textRenderer) return;

    // Шапка: Заголовок и крестик
    float titleScale = 0.54f * scale;
    std::string titleStr = "НАСТРОЙКИ УРОВНЯ И ЭКОНОМИКИ";
    float titleW = textRenderer->CalculateTextWidth(titleStr, titleScale);
    textRenderer->RenderText(titleStr, modalPos.x + (modalW - titleW) * 0.5f, modalPos.y + 11.0f * scale, titleScale, glm::vec3(1.0f, 0.86f, 0.25f));

    float crossW = textRenderer->CalculateTextWidth("X", 0.50f * scale);
    textRenderer->RenderText("X", crossPos.x + (crossSize.x - crossW) * 0.5f, crossPos.y + 5.0f * scale, 0.50f * scale, glm::vec3(0.9f));

    // Секция 1: Деньги
    float labelScale = 0.44f * scale;
    textRenderer->RenderText("Стартовые деньги ($):", contentLeft, moneyLabelY, labelScale, glm::vec3(0.85f, 0.90f, 0.95f));

    bool showCursor = (m_cursorBlinkTimer < 0.5f);
    std::string moneyDisplay = m_moneyStr + ((moneyFocused && showCursor) ? "|" : "");
    textRenderer->RenderText(moneyDisplay, moneyBoxPos.x + 10.0f * scale, moneyBoxPos.y + 7.0f * scale, 0.48f * scale, glm::vec3(1.0f, 0.92f, 0.40f));

    // Секция 2: Здоровье
    textRenderer->RenderText("Стартовое здоровье базы (HP):", contentLeft, healthLabelY, labelScale, glm::vec3(0.85f, 0.90f, 0.95f));
    std::string healthDisplay = m_healthStr + ((healthFocused && showCursor) ? "|" : "");
    textRenderer->RenderText(healthDisplay, healthBoxPos.x + 10.0f * scale, healthBoxPos.y + 7.0f * scale, 0.48f * scale, glm::vec3(0.55f, 0.95f, 0.65f));

    // Секция 3: Разрешенные башни и тиры
    textRenderer->RenderText("Разрешенные башни и лимиты улучшений:", contentLeft, towersLabelY, labelScale, glm::vec3(1.0f, 0.86f, 0.35f));

    float cbTextScale = 0.45f * scale;
    float cbTextOffX = 34.0f * scale;
    float cbTextOffY = 8.0f * scale;

    textRenderer->RenderText("Классическая (Basic)", contentLeft + cbTextOffX, rowBasicY + cbTextOffY, cbTextScale,
                             hasTower("Basic") ? glm::vec3(1.0f) : glm::vec3(0.50f, 0.52f, 0.56f));
    textRenderer->RenderText("Ртутная (Mercury / Goo)", contentLeft + cbTextOffX, rowMercuryY + cbTextOffY, cbTextScale,
                             hasTower("Mercury") ? glm::vec3(1.0f) : glm::vec3(0.50f, 0.52f, 0.56f));
    textRenderer->RenderText("Поршень (Piston)", contentLeft + cbTextOffX, rowPistonY + cbTextOffY, cbTextScale,
                             hasTower("Piston") ? glm::vec3(1.0f) : glm::vec3(0.50f, 0.52f, 0.56f));

    auto renderRowTierTexts = [&](const std::string& tower, float rowY) {
        bool enabled = hasTower(tower);
        auto it = m_towerMaxTiers.find(tower);
        int currentTier = (it != m_towerMaxTiers.end()) ? it->second : 3;

        float tierBtnY = rowY + (towerRowH - tierBtnH) * 0.5f;
        glm::vec2 t1Pos(contentLeft + contentW - totalTierGroupW, tierBtnY);
        glm::vec2 t2Pos(t1Pos.x + tierBtnW + tierBtnGap, tierBtnY);
        glm::vec2 t3Pos(t2Pos.x + tierBtnW + tierBtnGap, tierBtnY);
        glm::vec2 btnSize(tierBtnW, tierBtnH);

        auto drawBtnText = [&](const std::string& text, glm::vec2 pos, int tier) {
            bool selected = (currentTier == tier);
            float tw = textRenderer->CalculateTextWidth(text, 0.42f * scale);
            glm::vec3 col;
            if (!enabled) {
                col = glm::vec3(0.38f, 0.40f, 0.45f);
            } else if (selected) {
                col = glm::vec3(1.0f, 0.95f, 0.60f);
            } else {
                col = glm::vec3(0.70f, 0.75f, 0.80f);
            }
            textRenderer->RenderText(text, pos.x + (btnSize.x - tw) * 0.5f, pos.y + 6.0f * scale, 0.42f * scale, col);
        };

        drawBtnText("T1", t1Pos, 1);
        drawBtnText("T2", t2Pos, 2);
        drawBtnText("T3", t3Pos, 3);
    };

    renderRowTierTexts("Basic", rowBasicY);
    renderRowTierTexts("Mercury", rowMercuryY);
    renderRowTierTexts("Piston", rowPistonY);

    // Секция 4: Кнопки [ Сохранить ] и [ Отмена ]
    float btnTextScale = 0.46f * scale;
    std::string saveText = "Сохранить";
    float saveW = textRenderer->CalculateTextWidth(saveText, btnTextScale);
    textRenderer->RenderText(saveText, btnSavePos.x + (btnSaveSize.x - saveW) * 0.5f, btnSavePos.y + 8.0f * scale, btnTextScale, glm::vec3(1.0f));

    std::string cancelText = "Отмена";
    float cancelW = textRenderer->CalculateTextWidth(cancelText, btnTextScale);
    textRenderer->RenderText(cancelText, btnCancelPos.x + (btnCancelSize.x - cancelW) * 0.5f, btnCancelPos.y + 8.0f * scale, btnTextScale, glm::vec3(0.90f));
}

