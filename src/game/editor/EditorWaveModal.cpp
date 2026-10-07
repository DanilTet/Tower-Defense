#include "EditorWaveModal.h"
#include "../../renderer/SpriteRenderer.h"
#include "../../renderer/TextRenderer.h"
#include "../../textures/Texture2D.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

bool EditorWaveModal::isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) {
    return point.x >= rectPos.x && point.x <= rectPos.x + rectSize.x &&
           point.y >= rectPos.y && point.y <= rectPos.y + rectSize.y;
}

std::string EditorWaveModal::formatFloat1(float val) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", val);
    return std::string(buf);
}

void EditorWaveModal::open() {
    m_isOpen = true;
    m_openDropdownPartIdx = -1;
}

void EditorWaveModal::open(std::vector<WaveConfig>& waves) {
    m_isOpen = true;
    m_openDropdownPartIdx = -1;
    if (m_selectedWaveIdx < 0 || m_selectedWaveIdx >= static_cast<int>(waves.size())) {
        m_selectedWaveIdx = 0;
    }
}

void EditorWaveModal::open(LevelMapData& mapData) {
    m_boundMapData = &mapData;
    open(mapData.waves);
}

void EditorWaveModal::close() {
    m_isOpen = false;
    m_openDropdownPartIdx = -1;
    m_focusedField = WaveFocusedField::None;
    m_focusedPartIdx = -1;
    m_inputText.clear();
    m_fieldJustFocused = false;
}

void EditorWaveModal::toggle() {
    if (m_isOpen) close();
    else open();
}

void EditorWaveModal::toggle(std::vector<WaveConfig>& waves) {
    if (m_isOpen) close();
    else open(waves);
}

void EditorWaveModal::toggle(LevelMapData& mapData) {
    if (m_isOpen) close();
    else open(mapData);
}

void EditorWaveModal::update(float dt) {
    if (m_isOpen) {
        m_cursorBlinkTimer += dt;
        if (m_cursorBlinkTimer >= 1.0f) {
            m_cursorBlinkTimer -= 1.0f;
        }
    }
}

void EditorWaveModal::commitFocusedInput(std::vector<WaveConfig>& waves) {
    if (m_focusedField == WaveFocusedField::None || m_focusedPartIdx < 0) {
        m_focusedField = WaveFocusedField::None;
        m_focusedPartIdx = -1;
        m_inputText.clear();
        m_fieldJustFocused = false;
        return;
    }
    if (m_selectedWaveIdx < 0 || m_selectedWaveIdx >= static_cast<int>(waves.size())) {
        m_focusedField = WaveFocusedField::None;
        m_focusedPartIdx = -1;
        m_inputText.clear();
        m_fieldJustFocused = false;
        return;
    }

    WaveConfig& wave = waves[m_selectedWaveIdx];
    if (m_focusedPartIdx >= static_cast<int>(wave.parts.size())) {
        m_focusedField = WaveFocusedField::None;
        m_focusedPartIdx = -1;
        m_inputText.clear();
        m_fieldJustFocused = false;
        return;
    }

    WavePart& part = wave.parts[m_focusedPartIdx];

    if (!m_inputText.empty()) {
        try {
            if (m_focusedField == WaveFocusedField::Count) {
                int val = std::stoi(m_inputText);
                part.count = std::clamp(val, 1, 999);
            } else if (m_focusedField == WaveFocusedField::Interval) {
                float val = std::stof(m_inputText);
                part.spawnInterwal = std::clamp(val, 0.05f, 30.0f);
            } else if (m_focusedField == WaveFocusedField::Delay) {
                float val = std::stof(m_inputText);
                part.delayAfter = std::clamp(val, 0.0f, 120.0f);
            }
        } catch (...) {
            // Keep previous valid value
        }
    }

    m_focusedField = WaveFocusedField::None;
    m_focusedPartIdx = -1;
    m_inputText.clear();
    m_fieldJustFocused = false;
}

void EditorWaveModal::startEditingField(int partIdx, WaveFocusedField field, const std::vector<WaveConfig>& waves) {
    if (m_selectedWaveIdx >= 0 && m_selectedWaveIdx < static_cast<int>(waves.size())) {
        m_openDropdownPartIdx = -1;
        m_focusedPartIdx = partIdx;
        m_focusedField = field;
        m_fieldJustFocused = true;
        m_cursorBlinkTimer = 0.0f;

        const WaveConfig& wave = waves[m_selectedWaveIdx];
        if (partIdx >= 0 && partIdx < static_cast<int>(wave.parts.size())) {
            const WavePart& part = wave.parts[partIdx];
            if (field == WaveFocusedField::Count) {
                m_inputText = std::to_string(part.count);
            } else if (field == WaveFocusedField::Interval) {
                m_inputText = formatFloat1(part.spawnInterwal);
            } else if (field == WaveFocusedField::Delay) {
                m_inputText = formatFloat1(part.delayAfter);
            }
        }
    }
}

void EditorWaveModal::cycleNextInputField(std::vector<WaveConfig>& waves) {
    if (m_focusedField == WaveFocusedField::None || m_focusedPartIdx < 0) return;
    int curPart = m_focusedPartIdx;
    WaveFocusedField curField = m_focusedField;
    commitFocusedInput(waves);

    if (m_selectedWaveIdx < 0 || m_selectedWaveIdx >= static_cast<int>(waves.size())) return;
    WaveConfig& wave = waves[m_selectedWaveIdx];
    if (wave.parts.empty()) return;

    if (curField == WaveFocusedField::Count) {
        startEditingField(curPart, WaveFocusedField::Interval, waves);
    } else if (curField == WaveFocusedField::Interval) {
        startEditingField(curPart, WaveFocusedField::Delay, waves);
    } else if (curField == WaveFocusedField::Delay) {
        int nextPart = (curPart + 1) % static_cast<int>(wave.parts.size());
        startEditingField(nextPart, WaveFocusedField::Count, waves);
    }
}

bool EditorWaveModal::handleInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, float dt,
                                  int screenWidth, int screenHeight, float topBarHeight, float bottomDockHeight,
                                  std::vector<WaveConfig>& waves, bool& isDirty) {
    if (!m_isOpen) return false;

    bool isShiftDown = window && (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                                 glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);

    float panelW = std::clamp(640.0f, 540.0f, static_cast<float>(screenWidth) - 180.0f);
    float panelX = static_cast<float>(screenWidth) - panelW;
    float panelY = topBarHeight;
    float panelH = static_cast<float>(screenHeight) - bottomDockHeight - topBarHeight;

    // 1. Обработка ввода с клавиатуры
    if (window && m_focusedField != WaveFocusedField::None && m_focusedPartIdx >= 0) {
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

        if (checkKeyInput(GLFW_KEY_BACKSPACE)) {
            if (m_fieldJustFocused) {
                m_inputText.clear();
                m_fieldJustFocused = false;
            } else if (!m_inputText.empty()) {
                m_inputText.pop_back();
            }
            m_cursorBlinkTimer = 0.0f;
            isDirty = true;
        }
        else if (checkKeyInput(GLFW_KEY_ENTER) || checkKeyInput(GLFW_KEY_KP_ENTER)) {
            commitFocusedInput(waves);
            isDirty = true;
        }
        else if (checkKeyInput(GLFW_KEY_TAB)) {
            cycleNextInputField(waves);
            isDirty = true;
        }
        else if (checkKeyInput(GLFW_KEY_ESCAPE)) {
            commitFocusedInput(waves);
        }

        for (int k = 0; k <= 9; ++k) {
            if (checkKeyInput(GLFW_KEY_0 + k) || checkKeyInput(GLFW_KEY_KP_0 + k)) {
                char ch = '0' + k;
                if (m_fieldJustFocused) {
                    m_inputText.clear();
                    m_fieldJustFocused = false;
                }
                if (m_focusedField == WaveFocusedField::Count) {
                    if (m_inputText.length() < 3) {
                        m_inputText += ch;
                        m_cursorBlinkTimer = 0.0f;
                    }
                } else {
                    if (m_inputText.length() < 5) {
                        m_inputText += ch;
                        m_cursorBlinkTimer = 0.0f;
                    }
                }
                isDirty = true;
            }
        }

        if (m_focusedField != WaveFocusedField::Count) {
            if (checkKeyInput(GLFW_KEY_PERIOD) || checkKeyInput(GLFW_KEY_COMMA) || checkKeyInput(GLFW_KEY_KP_DECIMAL)) {
                if (m_fieldJustFocused) {
                    m_inputText.clear();
                    m_fieldJustFocused = false;
                }
                if (m_inputText.find('.') == std::string::npos && m_inputText.length() < 5) {
                    if (m_inputText.empty()) m_inputText += "0.";
                    else m_inputText += '.';
                    m_cursorBlinkTimer = 0.0f;
                }
                isDirty = true;
            }
        }
    }

    // 2. Обработка кликов мыши
    bool isClick = leftDown && !m_wasLeftDown;
    m_wasLeftDown = leftDown;

    if (isClick) {
        float headerH = 36.0f;
        float footerH = 32.0f;

        float colLeftW = 145.0f;
        float colLeftX = panelX + 8.0f;
        float colLeftY = panelY + headerH + 6.0f;
        float colLeftH = panelH - headerH - footerH - 12.0f;

        float colRightX = colLeftX + colLeftW + 8.0f;
        float colRightW = panelW - colLeftW - 24.0f;
        float colRightY = colLeftY;

        float cardsStartY = colRightY + 34.0f;
        float cardH = 92.0f;
        float cardGap = 8.0f;
        int maxVisibleParts = static_cast<int>((colLeftH - 38.0f) / (cardH + cardGap));
        if (maxVisibleParts < 1) maxVisibleParts = 1;

        // ПЕРВОЕ: Проверяем клик по выпадающему списку типов, если он открыт
        if (m_openDropdownPartIdx >= 0 && m_selectedWaveIdx >= 0 && m_selectedWaveIdx < static_cast<int>(waves.size())) {
            WaveConfig& curWave = waves[m_selectedWaveIdx];
            int visIdx = m_openDropdownPartIdx - m_wavePartsScrollOffset;
            if (visIdx >= 0 && visIdx < maxVisibleParts && m_openDropdownPartIdx < static_cast<int>(curWave.parts.size())) {
                glm::vec2 cardPos(colRightX, cardsStartY + static_cast<float>(visIdx) * (cardH + cardGap));
                glm::vec2 typeBtnPos(cardPos.x + 38.0f, cardPos.y + 8.0f);
                glm::vec2 typeBtnSize(115.0f, 26.0f);

                float dropX = cardPos.x + 38.0f;
                float dropW = 120.0f;
                float itemH = 28.0f;
                float totalDropH = itemH * 3.0f;
                float dropY = cardPos.y + 36.0f;
                if (dropY + totalDropH > panelY + panelH - footerH) {
                    dropY = cardPos.y + 8.0f - totalDropH - 2.0f;
                }

                if (isPointInRect(mousePos, glm::vec2(dropX, dropY), glm::vec2(dropW, itemH))) {
                    curWave.parts[m_openDropdownPartIdx].type = "Basic";
                    m_openDropdownPartIdx = -1;
                    isDirty = true;
                    return true;
                }
                if (isPointInRect(mousePos, glm::vec2(dropX, dropY + itemH), glm::vec2(dropW, itemH))) {
                    curWave.parts[m_openDropdownPartIdx].type = "Fast";
                    m_openDropdownPartIdx = -1;
                    isDirty = true;
                    return true;
                }
                if (isPointInRect(mousePos, glm::vec2(dropX, dropY + itemH * 2.0f), glm::vec2(dropW, itemH))) {
                    curWave.parts[m_openDropdownPartIdx].type = "Tank";
                    m_openDropdownPartIdx = -1;
                    isDirty = true;
                    return true;
                }
                if (isPointInRect(mousePos, typeBtnPos, typeBtnSize)) {
                    m_openDropdownPartIdx = -1;
                    return true;
                }
            }
            m_openDropdownPartIdx = -1;
            return true;
        }

        // Если кликнули за пределами боковой панели
        if (mousePos.x < panelX || mousePos.y < panelY || mousePos.y > panelY + panelH) {
            commitFocusedInput(waves);
            return false; // Клик передается в сетку/тулбар
        }

        // Кнопка [X Close] в заголовке
        glm::vec2 closeBtnPos(panelX + panelW - 74.0f, panelY + 5.0f);
        glm::vec2 closeBtnSize(68.0f, 26.0f);
        if (isPointInRect(mousePos, closeBtnPos, closeBtnSize)) {
            commitFocusedInput(waves);
            close();
            return true;
        }

        // Кнопка [+ Add Wave]
        glm::vec2 addWaveBtnPos(colLeftX + 4.0f, colLeftY + 4.0f);
        glm::vec2 addWaveBtnSize(colLeftW - 8.0f, 26.0f);
        if (isPointInRect(mousePos, addWaveBtnPos, addWaveBtnSize)) {
            commitFocusedInput(waves);
            WaveConfig newWave;
            WavePart p;
            p.type = "Basic";
            p.count = 10;
            p.spawnInterwal = 0.8f;
            p.delayAfter = 2.0f;
            newWave.parts.push_back(p);
            waves.push_back(newWave);
            m_selectedWaveIdx = static_cast<int>(waves.size()) - 1;
            m_wavePartsScrollOffset = 0;
            isDirty = true;
            return true;
        }

        // Список волн
        float waveItemY = colLeftY + 34.0f;
        float waveItemH = 30.0f;
        int maxVisibleWaves = static_cast<int>((colLeftH - 38.0f) / (waveItemH + 4.0f));

        for (size_t i = 0; i < waves.size() && static_cast<int>(i) < maxVisibleWaves; ++i) {
            float itemY = waveItemY + static_cast<float>(i) * (waveItemH + 4.0f);
            glm::vec2 itemPos(colLeftX + 4.0f, itemY);
            glm::vec2 itemSize(colLeftW - 36.0f, waveItemH);
            glm::vec2 delPos(colLeftX + colLeftW - 30.0f, itemY);
            glm::vec2 delSize(24.0f, waveItemH);

            if (isPointInRect(mousePos, itemPos, itemSize)) {
                commitFocusedInput(waves);
                m_selectedWaveIdx = static_cast<int>(i);
                m_wavePartsScrollOffset = 0;
                return true;
            }

            if (isPointInRect(mousePos, delPos, delSize)) {
                commitFocusedInput(waves);
                waves.erase(waves.begin() + i);
                if (waves.empty()) {
                    WaveConfig defW;
                    defW.parts.push_back({ "Basic", 10, 0.8f, 2.0f });
                    waves.push_back(defW);
                }
                m_selectedWaveIdx = std::clamp(m_selectedWaveIdx, 0, static_cast<int>(waves.size()) - 1);
                m_wavePartsScrollOffset = 0;
                isDirty = true;
                return true;
            }
        }

        // Правая колонка: подволны
        if (m_selectedWaveIdx >= 0 && m_selectedWaveIdx < static_cast<int>(waves.size())) {
            WaveConfig& curWave = waves[m_selectedWaveIdx];

            // [+ Add Batch]
            glm::vec2 addPartBtnPos(colRightX + colRightW - 110.0f, colRightY + 2.0f);
            glm::vec2 addPartBtnSize(110.0f, 26.0f);
            if (isPointInRect(mousePos, addPartBtnPos, addPartBtnSize)) {
                commitFocusedInput(waves);
                WavePart p;
                p.type = "Basic";
                p.count = 10;
                p.spawnInterwal = 0.8f;
                p.delayAfter = 2.0f;
                curWave.parts.push_back(p);
                isDirty = true;
                return true;
            }

            // Scroll buttons [^] and [v]
            glm::vec2 scrollUpPos(colRightX + colRightW - 176.0f, colRightY + 2.0f);
            glm::vec2 scrollDownPos(colRightX + colRightW - 144.0f, colRightY + 2.0f);
            glm::vec2 scrollBtnSize(28.0f, 26.0f);

            if (isPointInRect(mousePos, scrollUpPos, scrollBtnSize)) {
                commitFocusedInput(waves);
                if (m_wavePartsScrollOffset > 0) m_wavePartsScrollOffset--;
                return true;
            }
            if (isPointInRect(mousePos, scrollDownPos, scrollBtnSize)) {
                commitFocusedInput(waves);
                m_wavePartsScrollOffset++;
                return true;
            }

            int maxOffset = std::max(0, static_cast<int>(curWave.parts.size()) - maxVisibleParts);
            m_wavePartsScrollOffset = std::clamp(m_wavePartsScrollOffset, 0, maxOffset);

            for (int v = 0; v < maxVisibleParts; ++v) {
                int pIdx = m_wavePartsScrollOffset + v;
                if (pIdx >= static_cast<int>(curWave.parts.size())) break;

                WavePart& part = curWave.parts[pIdx];
                glm::vec2 cardPos(colRightX, cardsStartY + static_cast<float>(v) * (cardH + cardGap));
                glm::vec2 cardSize(colRightW, cardH);

                // Ряд 1: Кнопка выпадающего списка типа
                glm::vec2 typeBtnPos(cardPos.x + 38.0f, cardPos.y + 8.0f);
                glm::vec2 typeBtnSize(115.0f, 26.0f);
                if (isPointInRect(mousePos, typeBtnPos, typeBtnSize)) {
                    commitFocusedInput(waves);
                    m_openDropdownPartIdx = (m_openDropdownPartIdx == pIdx) ? -1 : pIdx;
                    return true;
                }

                // Ряд 1: Кнопка удаления пачки
                glm::vec2 delPartBtnPos(cardPos.x + cardSize.x - 34.0f, cardPos.y + 8.0f);
                glm::vec2 delPartBtnSize(26.0f, 26.0f);
                if (isPointInRect(mousePos, delPartBtnPos, delPartBtnSize)) {
                    commitFocusedInput(waves);
                    curWave.parts.erase(curWave.parts.begin() + pIdx);
                    if (m_wavePartsScrollOffset > 0 && m_wavePartsScrollOffset + maxVisibleParts > static_cast<int>(curWave.parts.size())) {
                        m_wavePartsScrollOffset--;
                    }
                    isDirty = true;
                    return true;
                }

                // Ряд 2: Count, Rate, Pause
                float row2Y = cardPos.y + 50.0f;
                float btnH = 26.0f;
                float nudgeW = 20.0f;

                // Count
                glm::vec2 cntDecPos(cardPos.x + 60.0f, row2Y);
                glm::vec2 cntBoxPos(cardPos.x + 82.0f, row2Y);
                glm::vec2 cntBoxSize(44.0f, btnH);
                glm::vec2 cntIncPos(cardPos.x + 128.0f, row2Y);

                if (isPointInRect(mousePos, cntDecPos, glm::vec2(nudgeW, btnH))) {
                    commitFocusedInput(waves);
                    int delta = isShiftDown ? 5 : 1;
                    part.count = std::max(1, part.count - delta);
                    isDirty = true;
                    return true;
                }
                if (isPointInRect(mousePos, cntBoxPos, cntBoxSize)) {
                    commitFocusedInput(waves);
                    startEditingField(pIdx, WaveFocusedField::Count, waves);
                    return true;
                }
                if (isPointInRect(mousePos, cntIncPos, glm::vec2(nudgeW, btnH))) {
                    commitFocusedInput(waves);
                    int delta = isShiftDown ? 5 : 1;
                    part.count = std::min(999, part.count + delta);
                    isDirty = true;
                    return true;
                }

                // Rate
                glm::vec2 rateDecPos(cardPos.x + 204.0f, row2Y);
                glm::vec2 rateBoxPos(cardPos.x + 226.0f, row2Y);
                glm::vec2 rateBoxSize(48.0f, btnH);
                glm::vec2 rateIncPos(cardPos.x + 276.0f, row2Y);

                if (isPointInRect(mousePos, rateDecPos, glm::vec2(nudgeW, btnH))) {
                    commitFocusedInput(waves);
                    part.spawnInterwal = std::max(0.05f, std::round((part.spawnInterwal - 0.1f) * 10.0f) / 10.0f);
                    isDirty = true;
                    return true;
                }
                if (isPointInRect(mousePos, rateBoxPos, rateBoxSize)) {
                    commitFocusedInput(waves);
                    startEditingField(pIdx, WaveFocusedField::Interval, waves);
                    return true;
                }
                if (isPointInRect(mousePos, rateIncPos, glm::vec2(nudgeW, btnH))) {
                    commitFocusedInput(waves);
                    part.spawnInterwal = std::min(30.0f, std::round((part.spawnInterwal + 0.1f) * 10.0f) / 10.0f);
                    isDirty = true;
                    return true;
                }

                // Pause
                glm::vec2 pauseDecPos(cardPos.x + 358.0f, row2Y);
                glm::vec2 pauseBoxPos(cardPos.x + 380.0f, row2Y);
                glm::vec2 pauseBoxSize(48.0f, btnH);
                glm::vec2 pauseIncPos(cardPos.x + 430.0f, row2Y);

                if (isPointInRect(mousePos, pauseDecPos, glm::vec2(nudgeW, btnH))) {
                    commitFocusedInput(waves);
                    part.delayAfter = std::max(0.0f, std::round((part.delayAfter - 0.5f) * 10.0f) / 10.0f);
                    isDirty = true;
                    return true;
                }
                if (isPointInRect(mousePos, pauseBoxPos, pauseBoxSize)) {
                    commitFocusedInput(waves);
                    startEditingField(pIdx, WaveFocusedField::Delay, waves);
                    return true;
                }
                if (isPointInRect(mousePos, pauseIncPos, glm::vec2(nudgeW, btnH))) {
                    commitFocusedInput(waves);
                    part.delayAfter = std::min(120.0f, std::round((part.delayAfter + 0.5f) * 10.0f) / 10.0f);
                    isDirty = true;
                    return true;
                }
            }
        }
        commitFocusedInput(waves);
        return true;
    }

    return true;
}

bool EditorWaveModal::handleInput(float mouseX, float mouseY, bool mousePressed, LevelMapData& mapData, int, int) {
    bool dummyDirty = false;
    return handleInput(nullptr, glm::vec2(mouseX, mouseY), mousePressed, 0.016f,
                       1920, 1080, 48.0f, 64.0f, mapData.waves, dummyDirty);
}

void EditorWaveModal::render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                             const std::shared_ptr<Texture2D>& whiteTexture,
                             int screenWidth, int screenHeight, float topBarHeight, float bottomDockHeight,
                             glm::vec2 mousePos, const std::vector<WaveConfig>& waves) {
    if (!m_isOpen || !renderer || !whiteTexture) return;

    float panelW = std::clamp(640.0f, 540.0f, static_cast<float>(screenWidth) - 180.0f);
    float panelX = static_cast<float>(screenWidth) - panelW;
    float panelY = topBarHeight;
    float panelH = static_cast<float>(screenHeight) - bottomDockHeight - topBarHeight;

    float headerH = 36.0f;
    float footerH = 32.0f;

    // 1. БОКОВАЯ ПАНЕЛЬ ПОВЕРХ КАРТЫ
    renderer->drawSprite(whiteTexture, glm::vec2(panelX - 2.0f, panelY), glm::vec2(2.0f, panelH), 0.0f, glm::vec3(0.25f, 0.65f, 0.95f));
    renderer->drawSprite(whiteTexture, glm::vec2(panelX, panelY), glm::vec2(panelW, panelH), 0.0f, glm::vec3(0.09f, 0.10f, 0.14f));

    // 2. ХЕДЕР БОКОВОЙ ПАНЕЛИ
    renderer->drawSprite(whiteTexture, glm::vec2(panelX, panelY), glm::vec2(panelW, headerH), 0.0f, glm::vec3(0.13f, 0.15f, 0.20f));
    renderer->drawSprite(whiteTexture, glm::vec2(panelX, panelY + headerH - 1.0f), glm::vec2(panelW, 1.0f), 0.0f, glm::vec3(0.25f, 0.60f, 0.90f));

    // Кнопка [X Close] в хедере
    glm::vec2 closeBtnPos(panelX + panelW - 74.0f, panelY + 5.0f);
    glm::vec2 closeBtnSize(68.0f, 26.0f);
    bool closeHov = isPointInRect(mousePos, closeBtnPos, closeBtnSize);
    renderer->drawSprite(whiteTexture, closeBtnPos, closeBtnSize, 0.0f, closeHov ? glm::vec3(0.85f, 0.30f, 0.30f) : glm::vec3(0.65f, 0.20f, 0.20f));
    renderer->drawSprite(whiteTexture, closeBtnPos + glm::vec2(1.0f), closeBtnSize - glm::vec2(2.0f), 0.0f, closeHov ? glm::vec3(0.48f, 0.16f, 0.16f) : glm::vec3(0.35f, 0.12f, 0.12f));

    // 3. ФУТЕР БОКОВОЙ ПАНЕЛИ
    float footerY = panelY + panelH - footerH;
    renderer->drawSprite(whiteTexture, glm::vec2(panelX, footerY), glm::vec2(panelW, footerH), 0.0f, glm::vec3(0.11f, 0.12f, 0.16f));
    renderer->drawSprite(whiteTexture, glm::vec2(panelX, footerY), glm::vec2(panelW, 1.0f), 0.0f, glm::vec3(0.22f, 0.25f, 0.32f));

    // 4. ЛЕВАЯ КОЛОНКА (Список волн)
    float colLeftW = 145.0f;
    float colLeftX = panelX + 8.0f;
    float colLeftY = panelY + headerH + 6.0f;
    float colLeftH = panelH - headerH - footerH - 12.0f;

    renderer->drawSprite(whiteTexture, glm::vec2(colLeftX, colLeftY), glm::vec2(colLeftW, colLeftH), 0.0f, glm::vec3(0.20f, 0.23f, 0.29f));
    renderer->drawSprite(whiteTexture, glm::vec2(colLeftX + 1.0f, colLeftY + 1.0f), glm::vec2(colLeftW - 2.0f, colLeftH - 2.0f), 0.0f, glm::vec3(0.07f, 0.08f, 0.11f));

    // Кнопка [+ Add Wave]
    glm::vec2 addWaveBtnPos(colLeftX + 4.0f, colLeftY + 4.0f);
    glm::vec2 addWaveBtnSize(colLeftW - 8.0f, 26.0f);
    bool addWaveHov = isPointInRect(mousePos, addWaveBtnPos, addWaveBtnSize);
    renderer->drawSprite(whiteTexture, addWaveBtnPos, addWaveBtnSize, 0.0f, addWaveHov ? glm::vec3(0.35f, 0.85f, 0.50f) : glm::vec3(0.25f, 0.70f, 0.40f));
    renderer->drawSprite(whiteTexture, addWaveBtnPos + glm::vec2(1.0f), addWaveBtnSize - glm::vec2(2.0f), 0.0f, addWaveHov ? glm::vec3(0.18f, 0.42f, 0.24f) : glm::vec3(0.13f, 0.32f, 0.18f));

    // Список волн
    float waveItemY = colLeftY + 34.0f;
    float waveItemH = 30.0f;
    int maxVisibleWaves = static_cast<int>((colLeftH - 38.0f) / (waveItemH + 4.0f));

    for (size_t i = 0; i < waves.size() && static_cast<int>(i) < maxVisibleWaves; ++i) {
        float itemY = waveItemY + static_cast<float>(i) * (waveItemH + 4.0f);
        glm::vec2 itemPos(colLeftX + 4.0f, itemY);
        glm::vec2 itemSize(colLeftW - 36.0f, waveItemH);
        glm::vec2 delPos(colLeftX + colLeftW - 30.0f, itemY);
        glm::vec2 delSize(24.0f, waveItemH);

        bool isSel = (static_cast<int>(i) == m_selectedWaveIdx);
        bool waveHov = isPointInRect(mousePos, itemPos, itemSize);
        bool delHov = isPointInRect(mousePos, delPos, delSize);

        glm::vec3 waveBorder = isSel ? glm::vec3(0.35f, 0.85f, 1.0f) : (waveHov ? glm::vec3(0.35f, 0.42f, 0.55f) : glm::vec3(0.25f, 0.28f, 0.35f));
        glm::vec3 waveBg = isSel ? glm::vec3(0.20f, 0.38f, 0.60f) : (waveHov ? glm::vec3(0.16f, 0.19f, 0.25f) : glm::vec3(0.12f, 0.14f, 0.18f));

        renderer->drawSprite(whiteTexture, itemPos, itemSize, 0.0f, waveBorder);
        renderer->drawSprite(whiteTexture, itemPos + glm::vec2(1.0f), itemSize - glm::vec2(2.0f), 0.0f, waveBg);

        renderer->drawSprite(whiteTexture, delPos, delSize, 0.0f, delHov ? glm::vec3(0.75f, 0.25f, 0.25f) : glm::vec3(0.55f, 0.20f, 0.20f));
        renderer->drawSprite(whiteTexture, delPos + glm::vec2(1.0f), delSize - glm::vec2(2.0f), 0.0f, delHov ? glm::vec3(0.40f, 0.15f, 0.15f) : glm::vec3(0.28f, 0.12f, 0.12f));
    }

    // 5. ПРАВАЯ КОЛОНКА (Sub-waves / Batches)
    float colRightX = colLeftX + colLeftW + 8.0f;
    float colRightW = panelW - colLeftW - 24.0f;
    float colRightY = colLeftY;

    // Кнопка [+ Add Batch]
    glm::vec2 addPartBtnPos(colRightX + colRightW - 110.0f, colRightY + 2.0f);
    glm::vec2 addPartBtnSize(110.0f, 26.0f);
    bool addPartHov = isPointInRect(mousePos, addPartBtnPos, addPartBtnSize);
    renderer->drawSprite(whiteTexture, addPartBtnPos, addPartBtnSize, 0.0f, addPartHov ? glm::vec3(0.35f, 0.85f, 0.50f) : glm::vec3(0.25f, 0.70f, 0.40f));
    renderer->drawSprite(whiteTexture, addPartBtnPos + glm::vec2(1.0f), addPartBtnSize - glm::vec2(2.0f), 0.0f, addPartHov ? glm::vec3(0.18f, 0.42f, 0.24f) : glm::vec3(0.13f, 0.32f, 0.18f));

    // Скролл-кнопки
    glm::vec2 scrollUpPos(colRightX + colRightW - 176.0f, colRightY + 2.0f);
    glm::vec2 scrollDownPos(colRightX + colRightW - 144.0f, colRightY + 2.0f);
    glm::vec2 scrollBtnSize(28.0f, 26.0f);
    bool upHov = isPointInRect(mousePos, scrollUpPos, scrollBtnSize);
    bool downHov = isPointInRect(mousePos, scrollDownPos, scrollBtnSize);

    renderer->drawSprite(whiteTexture, scrollUpPos, scrollBtnSize, 0.0f, upHov ? glm::vec3(0.50f, 0.55f, 0.65f) : glm::vec3(0.35f, 0.38f, 0.46f));
    renderer->drawSprite(whiteTexture, scrollUpPos + glm::vec2(1.0f), scrollBtnSize - glm::vec2(2.0f), 0.0f, upHov ? glm::vec3(0.24f, 0.27f, 0.34f) : glm::vec3(0.16f, 0.18f, 0.23f));
    renderer->drawSprite(whiteTexture, scrollDownPos, scrollBtnSize, 0.0f, downHov ? glm::vec3(0.50f, 0.55f, 0.65f) : glm::vec3(0.35f, 0.38f, 0.46f));
    renderer->drawSprite(whiteTexture, scrollDownPos + glm::vec2(1.0f), scrollBtnSize - glm::vec2(2.0f), 0.0f, downHov ? glm::vec3(0.24f, 0.27f, 0.34f) : glm::vec3(0.16f, 0.18f, 0.23f));

    // Cards
    float cardsStartY = colRightY + 34.0f;
    float cardH = 92.0f;
    float cardGap = 8.0f;
    int maxVisibleParts = static_cast<int>((colLeftH - 38.0f) / (cardH + cardGap));
    if (maxVisibleParts < 1) maxVisibleParts = 1;

    int totalMobsInWave = 0;
    if (m_selectedWaveIdx >= 0 && m_selectedWaveIdx < static_cast<int>(waves.size())) {
        const auto& curWave = waves[m_selectedWaveIdx];
        for (const auto& p : curWave.parts) totalMobsInWave += p.count;

        if (curWave.parts.empty()) {
            glm::vec2 emptyPos(colRightX, cardsStartY);
            glm::vec2 emptySize(colRightW, 90.0f);
            renderer->drawSprite(whiteTexture, emptyPos, emptySize, 0.0f, glm::vec3(0.22f, 0.25f, 0.30f));
            renderer->drawSprite(whiteTexture, emptyPos + glm::vec2(1.0f), emptySize - glm::vec2(2.0f), 0.0f, glm::vec3(0.10f, 0.11f, 0.14f));
        } else {
            for (int v = 0; v < maxVisibleParts; ++v) {
                int pIdx = m_wavePartsScrollOffset + v;
                if (pIdx >= static_cast<int>(curWave.parts.size())) break;
                const auto& part = curWave.parts[pIdx];

                glm::vec2 cardPos(colRightX, cardsStartY + static_cast<float>(v) * (cardH + cardGap));
                glm::vec2 cardSize(colRightW, cardH);

                renderer->drawSprite(whiteTexture, cardPos, cardSize, 0.0f, glm::vec3(0.24f, 0.28f, 0.36f));
                renderer->drawSprite(whiteTexture, cardPos + glm::vec2(1.0f), cardSize - glm::vec2(2.0f), 0.0f, glm::vec3(0.12f, 0.14f, 0.18f));

                // Кнопка типа
                glm::vec2 typeBtnPos(cardPos.x + 38.0f, cardPos.y + 8.0f);
                glm::vec2 typeBtnSize(115.0f, 26.0f);
                bool typeHov = isPointInRect(mousePos, typeBtnPos, typeBtnSize);
                bool isDropOpen = (m_openDropdownPartIdx == pIdx);

                glm::vec3 typeBg(0.15f, 0.28f, 0.45f);
                glm::vec3 typeBorder(0.30f, 0.60f, 0.95f);
                if (part.type == "Fast") {
                    typeBg = isDropOpen ? glm::vec3(0.48f, 0.40f, 0.16f) : (typeHov ? glm::vec3(0.44f, 0.36f, 0.14f) : glm::vec3(0.38f, 0.32f, 0.12f));
                    typeBorder = glm::vec3(0.95f, 0.80f, 0.20f);
                } else if (part.type == "Tank") {
                    typeBg = isDropOpen ? glm::vec3(0.52f, 0.20f, 0.20f) : (typeHov ? glm::vec3(0.48f, 0.18f, 0.18f) : glm::vec3(0.42f, 0.16f, 0.16f));
                    typeBorder = glm::vec3(0.95f, 0.32f, 0.32f);
                } else {
                    typeBg = isDropOpen ? glm::vec3(0.22f, 0.38f, 0.58f) : (typeHov ? glm::vec3(0.20f, 0.34f, 0.52f) : glm::vec3(0.15f, 0.28f, 0.45f));
                    typeBorder = isDropOpen ? glm::vec3(0.40f, 0.90f, 1.0f) : (typeHov ? glm::vec3(0.40f, 0.75f, 1.0f) : glm::vec3(0.30f, 0.60f, 0.95f));
                }
                renderer->drawSprite(whiteTexture, typeBtnPos, typeBtnSize, 0.0f, typeBorder);
                renderer->drawSprite(whiteTexture, typeBtnPos + glm::vec2(1.0f), typeBtnSize - glm::vec2(2.0f), 0.0f, typeBg);

                // Кнопка удаления пачки [X]
                glm::vec2 delPartBtnPos(cardPos.x + cardSize.x - 34.0f, cardPos.y + 8.0f);
                glm::vec2 delPartBtnSize(26.0f, 26.0f);
                bool delPartHov = isPointInRect(mousePos, delPartBtnPos, delPartBtnSize);
                renderer->drawSprite(whiteTexture, delPartBtnPos, delPartBtnSize, 0.0f, delPartHov ? glm::vec3(0.80f, 0.25f, 0.25f) : glm::vec3(0.60f, 0.20f, 0.20f));
                renderer->drawSprite(whiteTexture, delPartBtnPos + glm::vec2(1.0f), delPartBtnSize - glm::vec2(2.0f), 0.0f, delPartHov ? glm::vec3(0.45f, 0.16f, 0.16f) : glm::vec3(0.32f, 0.12f, 0.12f));

                // Ряд 2: Контролы параметров
                float row2Y = cardPos.y + 50.0f;
                float btnH = 26.0f;
                float nudgeW = 20.0f;

                // --- Count ---
                glm::vec2 cntDecPos(cardPos.x + 60.0f, row2Y);
                glm::vec2 cntBoxPos(cardPos.x + 82.0f, row2Y);
                glm::vec2 cntBoxSize(44.0f, btnH);
                glm::vec2 cntIncPos(cardPos.x + 128.0f, row2Y);

                bool cDecHov = isPointInRect(mousePos, cntDecPos, glm::vec2(nudgeW, btnH));
                renderer->drawSprite(whiteTexture, cntDecPos, glm::vec2(nudgeW, btnH), 0.0f, cDecHov ? glm::vec3(0.45f, 0.50f, 0.60f) : glm::vec3(0.32f, 0.36f, 0.45f));
                renderer->drawSprite(whiteTexture, cntDecPos + glm::vec2(1.0f), glm::vec2(nudgeW - 2.0f, btnH - 2.0f), 0.0f, cDecHov ? glm::vec3(0.24f, 0.27f, 0.34f) : glm::vec3(0.18f, 0.20f, 0.26f));

                bool isCntFocused = (m_focusedPartIdx == pIdx && m_focusedField == WaveFocusedField::Count);
                bool cBoxHov = isPointInRect(mousePos, cntBoxPos, cntBoxSize);
                glm::vec3 cntBorder = isCntFocused ? glm::vec3(0.35f, 0.85f, 1.0f) : (cBoxHov ? glm::vec3(0.40f, 0.46f, 0.58f) : glm::vec3(0.28f, 0.32f, 0.40f));
                glm::vec3 cntBoxBg = isCntFocused ? glm::vec3(0.10f, 0.15f, 0.24f) : (cBoxHov ? glm::vec3(0.11f, 0.13f, 0.17f) : glm::vec3(0.08f, 0.09f, 0.12f));
                renderer->drawSprite(whiteTexture, cntBoxPos, cntBoxSize, 0.0f, cntBorder);
                renderer->drawSprite(whiteTexture, cntBoxPos + glm::vec2(1.0f), cntBoxSize - glm::vec2(2.0f), 0.0f, cntBoxBg);

                bool cIncHov = isPointInRect(mousePos, cntIncPos, glm::vec2(nudgeW, btnH));
                renderer->drawSprite(whiteTexture, cntIncPos, glm::vec2(nudgeW, btnH), 0.0f, cIncHov ? glm::vec3(0.45f, 0.50f, 0.60f) : glm::vec3(0.32f, 0.36f, 0.45f));
                renderer->drawSprite(whiteTexture, cntIncPos + glm::vec2(1.0f), glm::vec2(nudgeW - 2.0f, btnH - 2.0f), 0.0f, cIncHov ? glm::vec3(0.24f, 0.27f, 0.34f) : glm::vec3(0.18f, 0.20f, 0.26f));

                // --- Rate ---
                glm::vec2 rateDecPos(cardPos.x + 204.0f, row2Y);
                glm::vec2 rateBoxPos(cardPos.x + 226.0f, row2Y);
                glm::vec2 rateBoxSize(48.0f, btnH);
                glm::vec2 rateIncPos(cardPos.x + 276.0f, row2Y);

                bool rDecHov = isPointInRect(mousePos, rateDecPos, glm::vec2(nudgeW, btnH));
                renderer->drawSprite(whiteTexture, rateDecPos, glm::vec2(nudgeW, btnH), 0.0f, rDecHov ? glm::vec3(0.45f, 0.50f, 0.60f) : glm::vec3(0.32f, 0.36f, 0.45f));
                renderer->drawSprite(whiteTexture, rateDecPos + glm::vec2(1.0f), glm::vec2(nudgeW - 2.0f, btnH - 2.0f), 0.0f, rDecHov ? glm::vec3(0.24f, 0.27f, 0.34f) : glm::vec3(0.18f, 0.20f, 0.26f));

                bool isRateFocused = (m_focusedPartIdx == pIdx && m_focusedField == WaveFocusedField::Interval);
                bool rBoxHov = isPointInRect(mousePos, rateBoxPos, rateBoxSize);
                glm::vec3 rateBorder = isRateFocused ? glm::vec3(0.35f, 0.85f, 1.0f) : (rBoxHov ? glm::vec3(0.40f, 0.46f, 0.58f) : glm::vec3(0.28f, 0.32f, 0.40f));
                glm::vec3 rateBoxBg = isRateFocused ? glm::vec3(0.10f, 0.15f, 0.24f) : (rBoxHov ? glm::vec3(0.11f, 0.13f, 0.17f) : glm::vec3(0.08f, 0.09f, 0.12f));
                renderer->drawSprite(whiteTexture, rateBoxPos, rateBoxSize, 0.0f, rateBorder);
                renderer->drawSprite(whiteTexture, rateBoxPos + glm::vec2(1.0f), rateBoxSize - glm::vec2(2.0f), 0.0f, rateBoxBg);

                bool rIncHov = isPointInRect(mousePos, rateIncPos, glm::vec2(nudgeW, btnH));
                renderer->drawSprite(whiteTexture, rateIncPos, glm::vec2(nudgeW, btnH), 0.0f, rIncHov ? glm::vec3(0.45f, 0.50f, 0.60f) : glm::vec3(0.32f, 0.36f, 0.45f));
                renderer->drawSprite(whiteTexture, rateIncPos + glm::vec2(1.0f), glm::vec2(nudgeW - 2.0f, btnH - 2.0f), 0.0f, rIncHov ? glm::vec3(0.24f, 0.27f, 0.34f) : glm::vec3(0.18f, 0.20f, 0.26f));

                // --- Pause ---
                glm::vec2 pauseDecPos(cardPos.x + 358.0f, row2Y);
                glm::vec2 pauseBoxPos(cardPos.x + 380.0f, row2Y);
                glm::vec2 pauseBoxSize(48.0f, btnH);
                glm::vec2 pauseIncPos(cardPos.x + 430.0f, row2Y);

                bool pDecHov = isPointInRect(mousePos, pauseDecPos, glm::vec2(nudgeW, btnH));
                renderer->drawSprite(whiteTexture, pauseDecPos, glm::vec2(nudgeW, btnH), 0.0f, pDecHov ? glm::vec3(0.45f, 0.50f, 0.60f) : glm::vec3(0.32f, 0.36f, 0.45f));
                renderer->drawSprite(whiteTexture, pauseDecPos + glm::vec2(1.0f), glm::vec2(nudgeW - 2.0f, btnH - 2.0f), 0.0f, pDecHov ? glm::vec3(0.24f, 0.27f, 0.34f) : glm::vec3(0.18f, 0.20f, 0.26f));

                bool isPauseFocused = (m_focusedPartIdx == pIdx && m_focusedField == WaveFocusedField::Delay);
                bool pBoxHov = isPointInRect(mousePos, pauseBoxPos, pauseBoxSize);
                glm::vec3 pauseBorder = isPauseFocused ? glm::vec3(0.35f, 0.85f, 1.0f) : (pBoxHov ? glm::vec3(0.40f, 0.46f, 0.58f) : glm::vec3(0.28f, 0.32f, 0.40f));
                glm::vec3 pauseBoxBg = isPauseFocused ? glm::vec3(0.10f, 0.15f, 0.24f) : (pBoxHov ? glm::vec3(0.11f, 0.13f, 0.17f) : glm::vec3(0.08f, 0.09f, 0.12f));
                renderer->drawSprite(whiteTexture, pauseBoxPos, pauseBoxSize, 0.0f, pauseBorder);
                renderer->drawSprite(whiteTexture, pauseBoxPos + glm::vec2(1.0f), pauseBoxSize - glm::vec2(2.0f), 0.0f, pauseBoxBg);

                bool pIncHov = isPointInRect(mousePos, pauseIncPos, glm::vec2(nudgeW, btnH));
                renderer->drawSprite(whiteTexture, pauseIncPos, glm::vec2(nudgeW, btnH), 0.0f, pIncHov ? glm::vec3(0.45f, 0.50f, 0.60f) : glm::vec3(0.32f, 0.36f, 0.45f));
                renderer->drawSprite(whiteTexture, pauseIncPos + glm::vec2(1.0f), glm::vec2(nudgeW - 2.0f, btnH - 2.0f), 0.0f, pIncHov ? glm::vec3(0.24f, 0.27f, 0.34f) : glm::vec3(0.18f, 0.20f, 0.26f));
            }
        }
    }

    renderer->flush();

    // 6. ТЕКСТ В ПАНЕЛИ И КАРТОЧКАХ
    if (textRenderer) {
        textRenderer->RenderText("WAVE CONFIGURATOR", panelX + 16.0f, panelY + 10.0f, 0.88f, glm::vec3(0.35f, 0.85f, 1.0f));
        textRenderer->RenderText("[X] Close", closeBtnPos.x + 8.0f, closeBtnPos.y + 6.0f, 0.58f, glm::vec3(1.0f, 0.85f, 0.85f));

        textRenderer->RenderText("+ Add Wave", addWaveBtnPos.x + 22.0f, addWaveBtnPos.y + 6.0f, 0.58f, glm::vec3(0.9f, 1.0f, 0.9f));

        for (size_t i = 0; i < waves.size() && static_cast<int>(i) < maxVisibleWaves; ++i) {
            float itemY = waveItemY + static_cast<float>(i) * (waveItemH + 4.0f);
            glm::vec2 itemPos(colLeftX + 4.0f, itemY);
            glm::vec2 delPos(colLeftX + colLeftW - 30.0f, itemY);

            bool isSel = (static_cast<int>(i) == m_selectedWaveIdx);
            glm::vec3 textColor = isSel ? glm::vec3(1.0f, 0.95f, 0.4f) : glm::vec3(0.9f);
            std::string label = "Wave #" + std::to_string(i + 1) + " (" + std::to_string(waves[i].parts.size()) + ")";
            textRenderer->RenderText(label, itemPos.x + 6.0f, itemPos.y + 8.0f, 0.56f, textColor);

            textRenderer->RenderText("x", delPos.x + 8.0f, delPos.y + 7.0f, 0.56f, glm::vec3(1.0f, 0.65f, 0.65f));
        }

        if (m_selectedWaveIdx >= 0 && m_selectedWaveIdx < static_cast<int>(waves.size())) {
            const auto& curWave = waves[m_selectedWaveIdx];
            std::string rTitle = "Wave #" + std::to_string(m_selectedWaveIdx + 1) + " Batches (" + std::to_string(curWave.parts.size()) + ")";
            textRenderer->RenderText(rTitle, colRightX, colRightY + 6.0f, 0.70f, glm::vec3(0.95f, 0.95f, 0.95f));

            textRenderer->RenderText("^", scrollUpPos.x + 10.0f, scrollUpPos.y + 5.0f, 0.60f, glm::vec3(0.85f));
            textRenderer->RenderText("v", scrollDownPos.x + 10.0f, scrollDownPos.y + 5.0f, 0.60f, glm::vec3(0.85f));
            textRenderer->RenderText("+ Add Batch", addPartBtnPos.x + 14.0f, addPartBtnPos.y + 6.0f, 0.58f, glm::vec3(0.9f, 1.0f, 0.9f));

            if (curWave.parts.empty()) {
                textRenderer->RenderText("No sub-waves configured.", colRightX + 15.0f, cardsStartY + 30.0f, 0.64f, glm::vec3(0.7f, 0.7f, 0.7f));
                textRenderer->RenderText("Click '+ Add Batch' to add enemies!", colRightX + 15.0f, cardsStartY + 55.0f, 0.58f, glm::vec3(0.4f, 0.8f, 1.0f));
            } else {
                bool showBlinkCursor = (m_cursorBlinkTimer < 0.53f);

                for (int v = 0; v < maxVisibleParts; ++v) {
                    int pIdx = m_wavePartsScrollOffset + v;
                    if (pIdx >= static_cast<int>(curWave.parts.size())) break;
                    const auto& part = curWave.parts[pIdx];
                    glm::vec2 cardPos(colRightX, cardsStartY + static_cast<float>(v) * (cardH + cardGap));

                    std::string badge = "#" + std::to_string(pIdx + 1);
                    textRenderer->RenderText(badge, cardPos.x + 10.0f, cardPos.y + 12.0f, 0.65f, glm::vec3(0.35f, 0.85f, 1.0f));

                    glm::vec2 typeBtnPos(cardPos.x + 38.0f, cardPos.y + 8.0f);
                    std::string typeLabel = part.type + (m_openDropdownPartIdx == pIdx ? "  ^" : "  v");
                    glm::vec3 typeTxtCol = (part.type == "Basic") ? glm::vec3(0.4f, 0.9f, 1.0f) : (part.type == "Fast" ? glm::vec3(1.0f, 0.9f, 0.3f) : glm::vec3(1.0f, 0.45f, 0.45f));
                    textRenderer->RenderText(typeLabel, typeBtnPos.x + 10.0f, typeBtnPos.y + 6.0f, 0.58f, typeTxtCol);

                    textRenderer->RenderText(std::to_string(part.count) + " mobs", cardPos.x + 165.0f, cardPos.y + 14.0f, 0.52f, glm::vec3(0.75f, 0.78f, 0.85f));
                    textRenderer->RenderText("x", cardPos.x + colRightW - 26.0f, cardPos.y + 13.0f, 0.55f, glm::vec3(1.0f, 0.70f, 0.70f));

                    float row2Y = cardPos.y + 50.0f;

                    // Count
                    textRenderer->RenderText("Count:", cardPos.x + 10.0f, row2Y + 6.0f, 0.52f, glm::vec3(0.85f));
                    textRenderer->RenderText("-", cardPos.x + 66.0f, row2Y + 5.0f, 0.60f, glm::vec3(1.0f));

                    bool isCntFocused = (m_focusedPartIdx == pIdx && m_focusedField == WaveFocusedField::Count);
                    std::string cntStr = isCntFocused ? (m_inputText + (showBlinkCursor ? "|" : "")) : std::to_string(part.count);
                    glm::vec3 cntCol = isCntFocused ? glm::vec3(0.4f, 1.0f, 1.0f) : glm::vec3(1.0f, 0.95f, 0.4f);
                    textRenderer->RenderText(cntStr, cardPos.x + 88.0f, row2Y + 6.0f, 0.54f, cntCol);
                    textRenderer->RenderText("+", cardPos.x + 133.0f, row2Y + 5.0f, 0.60f, glm::vec3(1.0f));

                    // Rate
                    textRenderer->RenderText("Rate:", cardPos.x + 162.0f, row2Y + 6.0f, 0.52f, glm::vec3(0.85f));
                    textRenderer->RenderText("-", cardPos.x + 210.0f, row2Y + 5.0f, 0.60f, glm::vec3(1.0f));

                    bool isRateFocused = (m_focusedPartIdx == pIdx && m_focusedField == WaveFocusedField::Interval);
                    std::string rateStr = isRateFocused ? (m_inputText + (showBlinkCursor ? "|" : "")) : (formatFloat1(part.spawnInterwal) + "s");
                    glm::vec3 rateCol = isRateFocused ? glm::vec3(0.4f, 1.0f, 1.0f) : glm::vec3(0.4f, 1.0f, 0.6f);
                    textRenderer->RenderText(rateStr, cardPos.x + 232.0f, row2Y + 6.0f, 0.54f, rateCol);
                    textRenderer->RenderText("+", cardPos.x + 281.0f, row2Y + 5.0f, 0.60f, glm::vec3(1.0f));

                    // Pause
                    textRenderer->RenderText("Pause:", cardPos.x + 310.0f, row2Y + 6.0f, 0.52f, glm::vec3(0.85f));
                    textRenderer->RenderText("-", cardPos.x + 364.0f, row2Y + 5.0f, 0.60f, glm::vec3(1.0f));

                    bool isPauseFocused = (m_focusedPartIdx == pIdx && m_focusedField == WaveFocusedField::Delay);
                    std::string pauseStr = isPauseFocused ? (m_inputText + (showBlinkCursor ? "|" : "")) : (formatFloat1(part.delayAfter) + "s");
                    glm::vec3 pauseCol = isPauseFocused ? glm::vec3(0.4f, 1.0f, 1.0f) : glm::vec3(1.0f, 0.8f, 0.4f);
                    textRenderer->RenderText(pauseStr, cardPos.x + 386.0f, row2Y + 6.0f, 0.54f, pauseCol);
                    textRenderer->RenderText("+", cardPos.x + 435.0f, row2Y + 5.0f, 0.60f, glm::vec3(1.0f));
                }
            }
        }

        std::string summary = "Waves: " + std::to_string(waves.size()) + " | Wave #" + std::to_string(m_selectedWaveIdx + 1) + ": " + std::to_string(totalMobsInWave) + " mobs | [Tab] Next field";
        textRenderer->RenderText(summary, panelX + 14.0f, panelY + panelH - 22.0f, 0.52f, glm::vec3(0.70f, 0.75f, 0.85f));
    }

    // 7. СЛОЙ ВЫПАДАЮЩЕГО СПИСКА
    if (m_openDropdownPartIdx >= 0 && m_selectedWaveIdx >= 0 && m_selectedWaveIdx < static_cast<int>(waves.size())) {
        const WaveConfig& curWave = waves[m_selectedWaveIdx];
        int visIdx = m_openDropdownPartIdx - m_wavePartsScrollOffset;
        if (visIdx >= 0 && visIdx < maxVisibleParts && m_openDropdownPartIdx < static_cast<int>(curWave.parts.size())) {
            glm::vec2 cardPos(colRightX, cardsStartY + static_cast<float>(visIdx) * (cardH + cardGap));
            float dropX = cardPos.x + 38.0f;
            float dropW = 120.0f;
            float itemH = 28.0f;
            float totalDropH = itemH * 3.0f;
            float dropY = cardPos.y + 36.0f;
            if (dropY + totalDropH > panelY + panelH - footerH) {
                dropY = cardPos.y + 8.0f - totalDropH - 2.0f;
            }

            renderer->drawSprite(whiteTexture, glm::vec2(dropX - 3.0f, dropY - 2.0f), glm::vec2(dropW + 6.0f, totalDropH + 7.0f), 0.0f, glm::vec3(0.02f, 0.03f, 0.05f));
            renderer->drawSprite(whiteTexture, glm::vec2(dropX - 1.0f, dropY - 1.0f), glm::vec2(dropW + 2.0f, totalDropH + 2.0f), 0.0f, glm::vec3(0.35f, 0.85f, 1.0f));
            renderer->drawSprite(whiteTexture, glm::vec2(dropX, dropY), glm::vec2(dropW, totalDropH), 0.0f, glm::vec3(0.10f, 0.12f, 0.17f));

            bool hov0 = isPointInRect(mousePos, glm::vec2(dropX, dropY), glm::vec2(dropW, itemH));
            glm::vec3 bg0 = hov0 ? glm::vec3(0.20f, 0.35f, 0.55f) : (curWave.parts[m_openDropdownPartIdx].type == "Basic" ? glm::vec3(0.14f, 0.22f, 0.32f) : glm::vec3(0.11f, 0.13f, 0.18f));
            renderer->drawSprite(whiteTexture, glm::vec2(dropX + 1.0f, dropY + 1.0f), glm::vec2(dropW - 2.0f, itemH - 2.0f), 0.0f, bg0);

            bool hov1 = isPointInRect(mousePos, glm::vec2(dropX, dropY + itemH), glm::vec2(dropW, itemH));
            glm::vec3 bg1 = hov1 ? glm::vec3(0.38f, 0.35f, 0.15f) : (curWave.parts[m_openDropdownPartIdx].type == "Fast" ? glm::vec3(0.25f, 0.22f, 0.12f) : glm::vec3(0.11f, 0.13f, 0.18f));
            renderer->drawSprite(whiteTexture, glm::vec2(dropX + 1.0f, dropY + itemH + 1.0f), glm::vec2(dropW - 2.0f, itemH - 2.0f), 0.0f, bg1);

            bool hov2 = isPointInRect(mousePos, glm::vec2(dropX, dropY + itemH * 2.0f), glm::vec2(dropW, itemH));
            glm::vec3 bg2 = hov2 ? glm::vec3(0.42f, 0.20f, 0.20f) : (curWave.parts[m_openDropdownPartIdx].type == "Tank" ? glm::vec3(0.28f, 0.14f, 0.14f) : glm::vec3(0.11f, 0.13f, 0.18f));
            renderer->drawSprite(whiteTexture, glm::vec2(dropX + 1.0f, dropY + itemH * 2.0f + 1.0f), glm::vec2(dropW - 2.0f, itemH - 2.0f), 0.0f, bg2);

            renderer->drawSprite(whiteTexture, glm::vec2(dropX + 4.0f, dropY + itemH), glm::vec2(dropW - 8.0f, 1.0f), 0.0f, glm::vec3(0.25f, 0.30f, 0.40f));
            renderer->drawSprite(whiteTexture, glm::vec2(dropX + 4.0f, dropY + itemH * 2.0f), glm::vec2(dropW - 8.0f, 1.0f), 0.0f, glm::vec3(0.25f, 0.30f, 0.40f));

            renderer->flush();

            if (textRenderer) {
                textRenderer->RenderText("Basic", dropX + 12.0f, dropY + 7.0f, 0.58f, glm::vec3(0.4f, 0.9f, 1.0f));
                textRenderer->RenderText("Fast",  dropX + 12.0f, dropY + itemH + 7.0f, 0.58f, glm::vec3(1.0f, 0.9f, 0.3f));
                textRenderer->RenderText("Tank",  dropX + 12.0f, dropY + itemH * 2.0f + 7.0f, 0.58f, glm::vec3(1.0f, 0.45f, 0.45f));
            }
        }
    }
}

void EditorWaveModal::render(SpriteRenderer* renderer, TextRenderer* textRenderer, float screenWidth, float screenHeight, const LevelMapData& mapData) {
    render(renderer, textRenderer, nullptr, static_cast<int>(screenWidth), static_cast<int>(screenHeight),
           48.0f, 64.0f, glm::vec2(-1000.0f), mapData.waves);
}

