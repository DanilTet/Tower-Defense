#include "LevelSelectState.h"
#include "GameStateManager.h"
#include "GameplayState.h"
#include "MainMenuState.h"
#include "MapEditorState.h"
#include "../core/InputManager.h"
#include "../core/LocalizationManager.h"
#include "../core/SettingsManager.h"
#include "../resources/ResourceManager.h"
#include "../renderer/SpriteRenderer.h"
#include "../renderer/TextRenderer.h"
#include "../textures/Texture2D.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cctype>
#include <iostream>

static bool containsCaseInsensitive(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return true;
    auto toLowerUtf8 = [](const std::string& str) {
        std::string res;
        for (size_t i = 0; i < str.length(); ) {
            unsigned char c = str[i];
            if (c < 0x80) {
                res += static_cast<char>(std::tolower(c));
                i++;
            } else if ((c & 0xE0) == 0xC0 && i + 1 < str.length()) {
                unsigned char c2 = str[i + 1];
                if (c == 0xD0 && c2 >= 0x90 && c2 <= 0x9F) {
                    res += static_cast<char>(0xD0);
                    res += static_cast<char>(c2 + 0x20);
                } else if (c == 0xD0 && c2 >= 0xA0 && c2 <= 0xAF) {
                    res += static_cast<char>(0xD1);
                    res += static_cast<char>(c2 - 0x20);
                } else if (c == 0xD0 && c2 == 0x81) { // Ё -> ё
                    res += static_cast<char>(0xD1);
                    res += static_cast<char>(0x91);
                } else if (c == 0xD0 && c2 == 0x84) { // Є -> є
                    res += static_cast<char>(0xD1);
                    res += static_cast<char>(0x94);
                } else if (c == 0xD0 && c2 == 0x86) { // І -> і
                    res += static_cast<char>(0xD1);
                    res += static_cast<char>(0x96);
                } else if (c == 0xD0 && c2 == 0x87) { // Ї -> ї
                    res += static_cast<char>(0xD1);
                    res += static_cast<char>(0x97);
                } else if (c == 0xD2 && c2 == 0x90) { // Ґ -> ґ
                    res += static_cast<char>(0xD2);
                    res += static_cast<char>(0x91);
                } else {
                    res += static_cast<char>(c);
                    res += static_cast<char>(c2);
                }
                i += 2;
            } else {
                res += static_cast<char>(c);
                i++;
            }
        }
        return res;
    };
    std::string hLower = toLowerUtf8(haystack);
    std::string nLower = toLowerUtf8(needle);
    return hLower.find(nLower) != std::string::npos;
}

LevelSelectState::LevelSelectState(GameStateManager& stateManager, int width, int height, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer)
    : m_stateManager(stateManager), m_width(width), m_height(height), m_renderer(renderer), m_textRenderer(textRenderer)
{
    m_uiTexture = std::shared_ptr<Texture2D>(ResourceManager::getTexture("uiBaseTexture"), [](Texture2D*) {});
    m_whiteTexture = std::shared_ptr<Texture2D>(ResourceManager::getWhiteTexture(), [](Texture2D*) {});
    m_mousePressedLastFrame = true;
}

void LevelSelectState::init() {
    m_levelCards.clear();
    m_filterChips.clear();

    auto allLevels = LevelManager::getAvailableLevels();
    if (allLevels.empty()) {
        LevelInfo lvl1;
        lvl1.filename = "level_1.json";
        lvl1.name = "Рівень 1";
        lvl1.fullPath = "res/levels/level_1.json";
        lvl1.isCampaign = true;
        lvl1.isBuiltIn = true;
        lvl1.tags = { "Кампания" };
        allLevels.push_back(lvl1);
    }

    // Фильтрация
    std::vector<LevelInfo> filteredLevels;
    for (const auto& lvl : allLevels) {
        if (m_selectedCategory == CategoryFilter::Campaign && !lvl.isCampaign) continue;
        if (m_selectedCategory == CategoryFilter::Test && lvl.isCampaign) continue;

        if (!m_searchQuery.empty()) {
            bool matchesName = containsCaseInsensitive(lvl.name, m_searchQuery);
            bool matchesFile = containsCaseInsensitive(lvl.filename, m_searchQuery);
            bool matchesTag = false;
            for (const auto& t : lvl.tags) {
                if (containsCaseInsensitive(t, m_searchQuery)) {
                    matchesTag = true;
                    break;
                }
            }
            if (!matchesName && !matchesFile && !matchesTag) continue;
        }

        filteredLevels.push_back(lvl);
    }

    const int itemsPerPage = 6;
    m_totalPages = std::max(1, static_cast<int>((filteredLevels.size() + itemsPerPage - 1) / itemsPerPage));
    if (m_currentPage >= m_totalPages) m_currentPage = m_totalPages - 1;
    if (m_currentPage < 0) m_currentPage = 0;

    int startIndex = m_currentPage * itemsPerPage;
    int endIndex = std::min(startIndex + itemsPerPage, static_cast<int>(filteredLevels.size()));

    float topY = 125.0f;

    float uiScale = SettingsManager::getUIScaleMultiplier();

    // Размеры карточек с учётом масштаба и ограничений экрана
    float baseCardW = 390.0f * uiScale;
    float baseCardH = 170.0f * uiScale;
    float gapX = std::clamp(30.0f * uiScale, 16.0f, 50.0f);
    float gapY = std::clamp(20.0f * uiScale, 10.0f, 36.0f);

    float maxAvailW = (m_width - 60.0f - gapX) * 0.5f;
    float maxAvailH = (m_height - 210.0f - 2.0f * gapY) / 3.0f;
    float cardWidth  = std::clamp(baseCardW, 220.0f, maxAvailW);
    float cardHeight = std::clamp(baseCardH, 110.0f, maxAvailH);

    float totalW = 2.0f * cardWidth + gapX;
    float startX = (m_width - totalW) * 0.5f;

    // Размеры кнопок внутри карточки
    float toggleW  = std::clamp(105.0f * uiScale, 80.0f, cardWidth * 0.38f);
    float toggleH  = std::clamp(26.0f  * uiScale, 20.0f, 34.0f);
    float smallBtnW = std::clamp(76.0f * uiScale, 58.0f, cardWidth * 0.30f);
    float smallBtnH = std::clamp(28.0f * uiScale, 20.0f, 36.0f);
    float playBtnW  = std::clamp(96.0f * uiScale, 70.0f, cardWidth * 0.35f);

    for (int i = startIndex; i < endIndex; ++i) {
        const auto& lvl = filteredLevels[i];
        int cardIdx = i - startIndex;
        int col = cardIdx % 2;
        int row = cardIdx / 2;

        float x = startX + col * (cardWidth + gapX);
        float y = topY + row * (cardHeight + gapY);

        LevelCardUI card;
        card.filename = lvl.filename;
        card.displayName = lvl.name;
        card.levelPath = lvl.fullPath;
        card.isCampaign = lvl.isCampaign;
        card.isBuiltIn = lvl.isBuiltIn;
        card.tags = lvl.tags;

        card.cardBtn.pos = glm::vec2(x, y);
        card.cardBtn.size = glm::vec2(cardWidth, cardHeight);
        card.cardBtn.state = 0;

        // Кнопка переключения типа Кампания / Тестовый (в правом верхнем углу карточки)
        card.typeToggleBtn.pos = glm::vec2(x + cardWidth - toggleW - 8.0f, y + 8.0f);
        card.typeToggleBtn.size = glm::vec2(toggleW, toggleH);
        card.typeToggleBtn.text = lvl.isCampaign ? LOC("LEVEL_BADGE_CAMPAIGN") : LOC("LEVEL_BADGE_TEST");
        card.typeToggleBtn.state = 0;

        // Кнопка Переименовать
        card.renameBtn.pos = glm::vec2(x + 10.0f, y + cardHeight - smallBtnH - 8.0f);
        card.renameBtn.size = glm::vec2(smallBtnW, smallBtnH);
        card.renameBtn.text = LOC("LEVEL_BTN_RENAME");
        card.renameBtn.state = 0;

        // Кнопка Теги
        card.tagsBtn.pos = glm::vec2(x + 10.0f + smallBtnW + 6.0f, y + cardHeight - smallBtnH - 8.0f);
        card.tagsBtn.size = glm::vec2(smallBtnW, smallBtnH);
        card.tagsBtn.text = LOC("LEVEL_BTN_TAGS");
        card.tagsBtn.state = 0;

        // Кнопка Играть
        card.playBtn.pos = glm::vec2(x + cardWidth - playBtnW - 8.0f, y + cardHeight - smallBtnH - 8.0f);
        card.playBtn.size = glm::vec2(playBtnW, smallBtnH);
        card.playBtn.text = LOC("LEVEL_BTN_PLAY");
        card.playBtn.state = 0;

        m_levelCards.push_back(card);
    }

    // Вкладки фильтра категорий
    float tabW = std::clamp(110.0f * uiScale, 80.0f, 160.0f);
    float tabH = std::clamp(32.0f  * uiScale, 24.0f, 44.0f);
    float tabGap = std::clamp(8.0f * uiScale, 4.0f, 14.0f);
    float tabStartX = startX;
    float tabY = std::clamp(72.0f * uiScale, 60.0f, 100.0f);

    m_tabAll.pos = glm::vec2(tabStartX, tabY);
    m_tabAll.size = glm::vec2(tabW, tabH);
    m_tabAll.text = LOC("LEVEL_TAB_ALL");

    m_tabCampaign.pos = glm::vec2(tabStartX + tabW + tabGap, tabY);
    m_tabCampaign.size = glm::vec2(tabW, tabH);
    m_tabCampaign.text = LOC("LEVEL_TAB_CAMPAIGN");

    m_tabTest.pos = glm::vec2(tabStartX + (tabW + tabGap) * 2.0f, tabY);
    m_tabTest.size = glm::vec2(tabW, tabH);
    m_tabTest.text = LOC("LEVEL_TAB_TEST");

    // Поле поиска
    float searchW = std::clamp(240.0f * uiScale, 160.0f, 340.0f);
    float searchX = startX + totalW - searchW;
    m_searchBoxPos = glm::vec2(searchX, tabY);
    m_searchBoxSize = glm::vec2(searchW, tabH);

    // Быстрые чипы тегов
    float chipFontScale = std::clamp(0.44f * uiScale, 0.34f, 0.62f);
    std::vector<std::string> popularTags = { "Кампания", "Тест", "Сложный", "Ртуть", "Шахта" };
    float chipX = searchX - 10.0f;
    for (int idx = static_cast<int>(popularTags.size()) - 1; idx >= 0; --idx) {
        float w = m_textRenderer ? (m_textRenderer->CalculateTextWidth("#" + popularTags[idx], chipFontScale) + 16.0f) : 60.0f;
        chipX -= (w + 6.0f);
        if (chipX < tabStartX + (tabW + tabGap) * 3.0f + 10.0f) break;
        TagChip chip;
        chip.tag = popularTags[idx];
        chip.pos = glm::vec2(chipX, tabY);
        chip.size = glm::vec2(w, tabH);
        m_filterChips.push_back(chip);
    }

    // Нижние кнопки навигации
    float navBtnH = std::clamp(38.0f * uiScale, 26.0f, 52.0f);
    float bottomY = static_cast<float>(m_height) - navBtnH - 14.0f;

    float backBtnW = std::clamp(160.0f * uiScale, 110.0f, 220.0f);
    m_btnBack.size = glm::vec2(backBtnW, navBtnH);
    m_btnBack.pos = glm::vec2(startX, bottomY);
    m_btnBack.text = LOC("LEVEL_BTN_BACK");

    float createBtnW = std::clamp(190.0f * uiScale, 130.0f, 260.0f);
    m_btnEditor.size = glm::vec2(createBtnW, navBtnH);
    m_btnEditor.pos = glm::vec2(startX + totalW - m_btnEditor.size.x, bottomY);
    m_btnEditor.text = LOC("LEVEL_BTN_CREATE");

    float pageBtnW = std::clamp(100.0f * uiScale, 70.0f, 140.0f);
    float pageBtnH = std::clamp(34.0f * uiScale, 24.0f, 46.0f);
    float centerX = m_width * 0.5f;
    float pageGap = std::clamp(40.0f * uiScale, 20.0f, 60.0f);

    m_btnPrevPage.size = glm::vec2(pageBtnW, pageBtnH);
    m_btnPrevPage.pos = glm::vec2(centerX - pageBtnW - pageGap, bottomY + (navBtnH - pageBtnH) * 0.5f);
    m_btnPrevPage.text = LOC("LEVEL_BTN_PREV");

    m_btnNextPage.size = glm::vec2(pageBtnW, pageBtnH);
    m_btnNextPage.pos = glm::vec2(centerX + pageGap, bottomY + (navBtnH - pageBtnH) * 0.5f);
    m_btnNextPage.text = LOC("LEVEL_BTN_NEXT");
}

void LevelSelectState::cleanup() {}

bool LevelSelectState::isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) {
    return point.x >= rectPos.x && point.x <= rectPos.x + rectSize.x &&
           point.y >= rectPos.y && point.y <= rectPos.y + rectSize.y;
}

void LevelSelectState::openRenameModal(const std::string& targetFileName) {
    m_renameTargetFileName = targetFileName;
    for (const auto& c : m_levelCards) {
        if (c.filename == targetFileName) {
            m_renameInputText = c.displayName;
            break;
        }
    }
    m_isRenameModalOpen = true;
    m_cursorBlinkTimer = 0.0f;
}

void LevelSelectState::confirmRename() {
    if (m_renameInputText.empty()) {
        m_isRenameModalOpen = false;
        return;
    }

    LevelManager::setLevelDisplayName(m_renameTargetFileName, m_renameInputText);
    std::string safeBase = InputManager::transliterateToAscii(m_renameInputText);
    std::string newFileName = LevelManager::sanitizeLevelFileName(safeBase);

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
        InputManager::popUtf8(m_renameInputText);
        m_cursorBlinkTimer = 0.0f;
        return true;
    }

    std::string typed = InputManager::getFrameText();
    if (!typed.empty() && m_renameInputText.length() < 60) {
        m_renameInputText += typed;
        m_cursorBlinkTimer = 0.0f;
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

void LevelSelectState::openTagsModal(const std::string& targetFileName) {
    m_tagsTargetFileName = targetFileName;
    m_currentLevelTags.clear();
    for (const auto& c : m_levelCards) {
        if (c.filename == targetFileName) {
            m_currentLevelTags = c.tags;
            break;
        }
    }
    m_tagInputText.clear();
    m_isTagsModalOpen = true;

    // Быстрые теги для добавления
    m_quickAddChips.clear();
    std::vector<std::string> presets = { "Кампания", "Тест", "Сложный", "Ртуть", "Шахта", "Лабиринт", "Быстрый" };
    float chipX = 0.0f;
    for (const auto& p : presets) {
        float w = m_textRenderer ? (m_textRenderer->CalculateTextWidth("+" + p, 0.45f) + 16.0f) : 64.0f;
        TagChip chip;
        chip.tag = p;
        chip.size = glm::vec2(w, 26.0f);
        m_quickAddChips.push_back(chip);
    }
}

void LevelSelectState::saveAndCloseTagsModal() {
    LevelManager::setLevelTags(m_tagsTargetFileName, m_currentLevelTags);
    m_isTagsModalOpen = false;
    init();
}

bool LevelSelectState::processTagsModalInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, float dt) {
    if (!m_isTagsModalOpen) return false;

    glm::vec2 modalSize(500.0f, 380.0f);
    glm::vec2 modalPos((static_cast<float>(m_width) - modalSize.x) * 0.5f, (static_cast<float>(m_height) - modalSize.y) * 0.5f);

    glm::vec2 btnDonePos(modalPos.x + (modalSize.x - 140.0f) * 0.5f, modalPos.y + modalSize.y - 48.0f);
    glm::vec2 btnDoneSize(140.0f, 36.0f);

    glm::vec2 btnAddPos(modalPos.x + modalSize.x - 130.0f, modalPos.y + modalSize.y - 100.0f);
    glm::vec2 btnAddSize(100.0f, 32.0f);

    glm::vec2 boxPos(modalPos.x + 30.0f, modalPos.y + modalSize.y - 100.0f);
    glm::vec2 boxSize(modalSize.x - 170.0f, 32.0f);

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
        saveAndCloseTagsModal();
        return true;
    }
    if (checkKeyInput(GLFW_KEY_ENTER) || checkKeyInput(GLFW_KEY_KP_ENTER)) {
        if (!m_tagInputText.empty()) {
            if (std::find(m_currentLevelTags.begin(), m_currentLevelTags.end(), m_tagInputText) == m_currentLevelTags.end()) {
                m_currentLevelTags.push_back(m_tagInputText);
            }
            m_tagInputText.clear();
        } else {
            saveAndCloseTagsModal();
        }
        return true;
    }
    if (checkKeyInput(GLFW_KEY_BACKSPACE)) {
        InputManager::popUtf8(m_tagInputText);
        return true;
    }

    std::string typed = InputManager::getFrameText();
    if (!typed.empty() && m_tagInputText.length() < 30) {
        m_tagInputText += typed;
    }

    if (leftDown && !m_mousePressedLastFrame) {
        // Клик по кнопке Готово
        if (isPointInRect(mousePos, btnDonePos, btnDoneSize)) {
            saveAndCloseTagsModal();
            return true;
        }

        // Клик по кнопке Добавить
        if (isPointInRect(mousePos, btnAddPos, btnAddSize)) {
            if (!m_tagInputText.empty()) {
                if (std::find(m_currentLevelTags.begin(), m_currentLevelTags.end(), m_tagInputText) == m_currentLevelTags.end()) {
                    m_currentLevelTags.push_back(m_tagInputText);
                }
                m_tagInputText.clear();
            }
            return true;
        }

        // Клик по быстрым тегам (пресетам)
        float curX = modalPos.x + 30.0f;
        float curY = modalPos.y + 195.0f;
        for (const auto& chip : m_quickAddChips) {
            if (curX + chip.size.x > modalPos.x + modalSize.x - 30.0f) {
                curX = modalPos.x + 30.0f;
                curY += chip.size.y + 6.0f;
            }
            if (isPointInRect(mousePos, glm::vec2(curX, curY), chip.size)) {
                if (std::find(m_currentLevelTags.begin(), m_currentLevelTags.end(), chip.tag) == m_currentLevelTags.end()) {
                    m_currentLevelTags.push_back(chip.tag);
                }
                return true;
            }
            curX += chip.size.x + 6.0f;
        }

        // Клик по удалению существующих тегов (крестик x)
        float tagX = modalPos.x + 30.0f;
        float tagY = modalPos.y + 65.0f;
        for (size_t i = 0; i < m_currentLevelTags.size(); ++i) {
            float tw = m_textRenderer ? (m_textRenderer->CalculateTextWidth(m_currentLevelTags[i], 0.48f) + 34.0f) : 70.0f;
            if (tagX + tw > modalPos.x + modalSize.x - 30.0f) {
                tagX = modalPos.x + 30.0f;
                tagY += 30.0f;
            }
            // Зона крестика
            glm::vec2 delPos(tagX + tw - 22.0f, tagY + 4.0f);
            glm::vec2 delSize(18.0f, 18.0f);
            if (isPointInRect(mousePos, delPos, delSize)) {
                m_currentLevelTags.erase(m_currentLevelTags.begin() + i);
                return true;
            }
            tagX += tw + 8.0f;
        }

        if (!isPointInRect(mousePos, modalPos, modalSize)) {
            saveAndCloseTagsModal();
            return true;
        }
    }

    return true;
}

void LevelSelectState::processInput(GLFWwindow* window, float dt) {
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    m_mousePos = glm::vec2(static_cast<float>(mouseX), static_cast<float>(mouseY));

    bool leftDown = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);

    // Защита от клика-сквозь при переходе из другого стейта
    if (m_suppressClickUntilRelease) {
        if (!leftDown) {
            std::cout << "[LevelSelect] Mouse released -> interaction ready" << std::endl;
            m_suppressClickUntilRelease = false;
            m_mousePressedLastFrame = false;
        } else {
            return;
        }
    }

    if (m_isRenameModalOpen) {
        bool handled = processRenameModalInput(window, m_mousePos, leftDown, dt);
        m_mousePressedLastFrame = leftDown;
        if (handled) return;
    }

    if (m_isTagsModalOpen) {
        bool handled = processTagsModalInput(window, m_mousePos, leftDown, dt);
        m_mousePressedLastFrame = leftDown;
        if (handled) return;
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        if (m_isSearchActive) {
            m_isSearchActive = false;
            m_searchQuery.clear();
            init();
            return;
        }
        m_stateManager.setState(std::make_unique<MainMenuState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer));
        return;
    }

    // Ввод в поисковую строку
    if (m_isSearchActive) {
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
            InputManager::popUtf8(m_searchQuery);
            init();
        }

        std::string typed = InputManager::getFrameText();
        if (!typed.empty() && m_searchQuery.length() < 30) {
            m_searchQuery += typed;
            init();
        }
    }

    // Обработка клика
    if (leftDown && !m_mousePressedLastFrame) {
        m_mousePressedLastFrame = true;

        // Клик по вкладкам категорий
        if (isPointInRect(m_mousePos, m_tabAll.pos, m_tabAll.size)) {
            m_selectedCategory = CategoryFilter::All;
            m_currentPage = 0;
            init();
            return;
        }
        if (isPointInRect(m_mousePos, m_tabCampaign.pos, m_tabCampaign.size)) {
            m_selectedCategory = CategoryFilter::Campaign;
            m_currentPage = 0;
            init();
            return;
        }
        if (isPointInRect(m_mousePos, m_tabTest.pos, m_tabTest.size)) {
            m_selectedCategory = CategoryFilter::Test;
            m_currentPage = 0;
            init();
            return;
        }

        // Клик по строке поиска
        if (isPointInRect(m_mousePos, m_searchBoxPos, m_searchBoxSize)) {
            m_isSearchActive = true;
            return;
        } else {
            m_isSearchActive = false;
        }

        // Клик по чипам быстрого поиска тегов
        for (const auto& chip : m_filterChips) {
            if (isPointInRect(m_mousePos, chip.pos, chip.size)) {
                if (m_searchQuery == chip.tag) {
                    m_searchQuery.clear();
                } else {
                    m_searchQuery = chip.tag;
                }
                m_currentPage = 0;
                init();
                return;
            }
        }

        // Клик по карточкам и их кнопкам
        for (auto& card : m_levelCards) {
            if (isPointInRect(m_mousePos, card.typeToggleBtn.pos, card.typeToggleBtn.size)) {
                card.isCampaign = !card.isCampaign;
                LevelManager::setLevelCampaign(card.filename, card.isCampaign);
                init();
                return;
            }

            if (isPointInRect(m_mousePos, card.renameBtn.pos, card.renameBtn.size)) {
                openRenameModal(card.filename);
                return;
            }

            if (isPointInRect(m_mousePos, card.tagsBtn.pos, card.tagsBtn.size)) {
                openTagsModal(card.filename);
                return;
            }

            if (isPointInRect(m_mousePos, card.playBtn.pos, card.playBtn.size) || isPointInRect(m_mousePos, card.cardBtn.pos, card.cardBtn.size)) {
                std::cout << "[LevelSelect] Launching level: " << card.displayName << " (" << card.filename << ")" << std::endl;
                m_stateManager.setState(std::make_unique<GameplayState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, card.levelPath));
                return;
            }
        }

        // Пагинация
        if (m_totalPages > 1) {
            if (isPointInRect(m_mousePos, m_btnPrevPage.pos, m_btnPrevPage.size) && m_currentPage > 0) {
                m_currentPage--;
                init();
                return;
            }
            if (isPointInRect(m_mousePos, m_btnNextPage.pos, m_btnNextPage.size) && m_currentPage < m_totalPages - 1) {
                m_currentPage++;
                init();
                return;
            }
        }

        // Навигация
        if (isPointInRect(m_mousePos, m_btnBack.pos, m_btnBack.size)) {
            std::cout << "[LevelSelect] Clicked '< BACK' -> MainMenuState" << std::endl;
            m_stateManager.setState(std::make_unique<MainMenuState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer));
            return;
        }

        if (isPointInRect(m_mousePos, m_btnEditor.pos, m_btnEditor.size)) {
            std::cout << "[LevelSelect] Clicked '+ CREATE MAP' -> MapEditorState" << std::endl;
            m_stateManager.setState(std::make_unique<MapEditorState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer));
            return;
        }
    }
    else if (!leftDown) {
        m_mousePressedLastFrame = false;
    }
}

void LevelSelectState::update(float dt) {
    if (m_isRenameModalOpen || m_isTagsModalOpen || m_isSearchActive) {
        m_cursorBlinkTimer += dt;
        if (m_cursorBlinkTimer >= 1.0f) {
            m_cursorBlinkTimer -= 1.0f;
        }
    }
}

void LevelSelectState::render() {
    if (!m_renderer || !m_textRenderer) return;

    m_renderer->beginBatch();

    // 1. Отрисовка вкладок категорий
    auto drawTab = [&](const UIButton& tab, bool active) {
        glm::vec3 bg = active ? glm::vec3(0.20f, 0.38f, 0.55f) : glm::vec3(0.14f, 0.16f, 0.22f);
        glm::vec3 border = active ? glm::vec3(0.35f, 0.85f, 1.0f) : glm::vec3(0.25f, 0.28f, 0.35f);
        m_renderer->drawSprite(m_whiteTexture, tab.pos, tab.size, 0.0f, border);
        m_renderer->drawSprite(m_whiteTexture, tab.pos + glm::vec2(1.0f), tab.size - glm::vec2(2.0f), 0.0f, bg);
    };

    drawTab(m_tabAll, m_selectedCategory == CategoryFilter::All);
    drawTab(m_tabCampaign, m_selectedCategory == CategoryFilter::Campaign);
    drawTab(m_tabTest, m_selectedCategory == CategoryFilter::Test);

    // 2. Отрисовка строки поиска
    glm::vec3 searchBorder = m_isSearchActive ? glm::vec3(0.35f, 0.85f, 1.0f) : glm::vec3(0.25f, 0.28f, 0.35f);
    m_renderer->drawSprite(m_whiteTexture, m_searchBoxPos, m_searchBoxSize, 0.0f, searchBorder);
    m_renderer->drawSprite(m_whiteTexture, m_searchBoxPos + glm::vec2(1.0f), m_searchBoxSize - glm::vec2(2.0f), 0.0f, glm::vec3(0.12f, 0.13f, 0.18f));

    // 3. Отрисовка чипов быстрых тегов
    for (const auto& chip : m_filterChips) {
        bool active = (m_searchQuery == chip.tag);
        glm::vec3 bg = active ? glm::vec3(0.28f, 0.38f, 0.22f) : glm::vec3(0.14f, 0.18f, 0.20f);
        glm::vec3 border = active ? glm::vec3(0.4f, 0.95f, 0.4f) : glm::vec3(0.25f, 0.35f, 0.30f);
        m_renderer->drawSprite(m_whiteTexture, chip.pos, chip.size, 0.0f, border);
        m_renderer->drawSprite(m_whiteTexture, chip.pos + glm::vec2(1.0f), chip.size - glm::vec2(2.0f), 0.0f, bg);
    }

    // 4. Отрисовка карточек
    for (const auto& card : m_levelCards) {
        bool hover = isPointInRect(m_mousePos, card.cardBtn.pos, card.cardBtn.size);
        glm::vec3 borderColor = hover ? glm::vec3(0.40f, 0.75f, 1.0f) : (card.isCampaign ? glm::vec3(0.20f, 0.45f, 0.25f) : glm::vec3(0.25f, 0.28f, 0.35f));
        glm::vec3 cardBg = hover ? glm::vec3(0.16f, 0.19f, 0.26f) : glm::vec3(0.12f, 0.13f, 0.17f);

        m_renderer->drawSprite(m_whiteTexture, card.cardBtn.pos, card.cardBtn.size, 0.0f, borderColor);
        m_renderer->drawSprite(m_whiteTexture, card.cardBtn.pos + glm::vec2(2.0f), card.cardBtn.size - glm::vec2(4.0f), 0.0f, cardBg);

        // Кнопка переключения типа
        glm::vec3 typeBg = card.isCampaign ? glm::vec3(0.16f, 0.38f, 0.20f) : glm::vec3(0.38f, 0.25f, 0.12f);
        glm::vec3 typeBorder = card.isCampaign ? glm::vec3(0.35f, 0.95f, 0.45f) : glm::vec3(1.0f, 0.70f, 0.25f);
        m_renderer->drawSprite(m_whiteTexture, card.typeToggleBtn.pos, card.typeToggleBtn.size, 0.0f, typeBorder);
        m_renderer->drawSprite(m_whiteTexture, card.typeToggleBtn.pos + glm::vec2(1.0f), card.typeToggleBtn.size - glm::vec2(2.0f), 0.0f, typeBg);

        // Кнопка Rename
        bool hovRen = isPointInRect(m_mousePos, card.renameBtn.pos, card.renameBtn.size);
        m_renderer->drawSprite(m_whiteTexture, card.renameBtn.pos, card.renameBtn.size, 0.0f, hovRen ? glm::vec3(0.4f, 0.8f, 1.0f) : glm::vec3(0.28f, 0.32f, 0.42f));
        m_renderer->drawSprite(m_whiteTexture, card.renameBtn.pos + glm::vec2(1.0f), card.renameBtn.size - glm::vec2(2.0f), 0.0f, glm::vec3(0.16f, 0.18f, 0.24f));

        // Кнопка Tags
        bool hovTags = isPointInRect(m_mousePos, card.tagsBtn.pos, card.tagsBtn.size);
        m_renderer->drawSprite(m_whiteTexture, card.tagsBtn.pos, card.tagsBtn.size, 0.0f, hovTags ? glm::vec3(0.4f, 0.8f, 1.0f) : glm::vec3(0.28f, 0.32f, 0.42f));
        m_renderer->drawSprite(m_whiteTexture, card.tagsBtn.pos + glm::vec2(1.0f), card.tagsBtn.size - glm::vec2(2.0f), 0.0f, glm::vec3(0.16f, 0.18f, 0.24f));

        // Кнопка Play
        bool hovPlay = isPointInRect(m_mousePos, card.playBtn.pos, card.playBtn.size);
        m_renderer->drawSprite(m_whiteTexture, card.playBtn.pos, card.playBtn.size, 0.0f, hovPlay ? glm::vec3(0.35f, 0.95f, 0.50f) : glm::vec3(0.25f, 0.75f, 0.35f));
        m_renderer->drawSprite(m_whiteTexture, card.playBtn.pos + glm::vec2(1.0f), card.playBtn.size - glm::vec2(2.0f), 0.0f, hovPlay ? glm::vec3(0.18f, 0.38f, 0.22f) : glm::vec3(0.14f, 0.30f, 0.18f));
    }

    // 5. Отрисовка нижних кнопок
    auto drawNavBtn = [&](const UIButton& btn, const glm::vec3& primaryColor) {
        bool hover = isPointInRect(m_mousePos, btn.pos, btn.size);
        m_renderer->drawSprite(m_whiteTexture, btn.pos, btn.size, 0.0f, hover ? (primaryColor + glm::vec3(0.2f)) : primaryColor);
        m_renderer->drawSprite(m_whiteTexture, btn.pos + glm::vec2(1.0f), btn.size - glm::vec2(2.0f), 0.0f, glm::vec3(0.14f, 0.16f, 0.22f));
    };

    drawNavBtn(m_btnBack, glm::vec3(0.35f, 0.38f, 0.48f));
    drawNavBtn(m_btnEditor, glm::vec3(0.35f, 0.75f, 0.95f));

    if (m_totalPages > 1) {
        if (m_currentPage > 0) drawNavBtn(m_btnPrevPage, glm::vec3(0.30f, 0.55f, 0.85f));
        if (m_currentPage < m_totalPages - 1) drawNavBtn(m_btnNextPage, glm::vec3(0.30f, 0.55f, 0.85f));
    }

    m_renderer->flush();

    // 6. Отрисовка текста
    float uiScale = SettingsManager::getUIScaleMultiplier();

    // Масштабы шрифтов с учётом uiScale
    float fTitle       = std::clamp(1.0f  * uiScale, 0.70f, 1.50f);
    float fTab         = std::clamp(0.52f * uiScale, 0.38f, 0.72f);
    float fSearch      = std::clamp(0.46f * uiScale, 0.36f, 0.65f);
    float fSearchIn    = std::clamp(0.50f * uiScale, 0.38f, 0.68f);
    float fChip        = std::clamp(0.44f * uiScale, 0.34f, 0.62f);
    float fCardName    = std::clamp(0.64f * uiScale, 0.46f, 0.90f);
    float fCardBadge   = std::clamp(0.44f * uiScale, 0.34f, 0.62f);
    float fCardFile    = std::clamp(0.44f * uiScale, 0.34f, 0.60f);
    float fCardTags    = std::clamp(0.46f * uiScale, 0.36f, 0.64f);
    float fCardSmBtn   = std::clamp(0.46f * uiScale, 0.36f, 0.64f);
    float fCardPlayBtn = std::clamp(0.52f * uiScale, 0.40f, 0.72f);
    float fNavBtn      = std::clamp(0.55f * uiScale, 0.42f, 0.76f);
    float fPage        = std::clamp(0.52f * uiScale, 0.40f, 0.70f);

    // Заголовок
    std::string mainTitle = LOC("LEVEL_TITLE");
    float tW = m_textRenderer->CalculateTextWidth(mainTitle, fTitle);
    m_textRenderer->RenderText(mainTitle, (m_width - tW) * 0.5f, 22.0f, fTitle, glm::vec3(1.0f, 0.85f, 0.25f));

    // Текст вкладок
    auto drawCenterText = [&](const UIButton& btn, const glm::vec3& color, float scale) {
        float w = m_textRenderer->CalculateTextWidth(btn.text, scale);
        m_textRenderer->RenderText(btn.text, btn.pos.x + (btn.size.x - w) * 0.5f, btn.pos.y + (btn.size.y - scale * 28.0f) * 0.5f + 2.0f, scale, color);
    };

    drawCenterText(m_tabAll,      m_selectedCategory == CategoryFilter::All      ? glm::vec3(0.35f, 0.95f, 1.0f) : glm::vec3(0.75f, 0.78f, 0.85f), fTab);
    drawCenterText(m_tabCampaign, m_selectedCategory == CategoryFilter::Campaign ? glm::vec3(0.35f, 0.95f, 1.0f) : glm::vec3(0.75f, 0.78f, 0.85f), fTab);
    drawCenterText(m_tabTest,     m_selectedCategory == CategoryFilter::Test     ? glm::vec3(0.35f, 0.95f, 1.0f) : glm::vec3(0.75f, 0.78f, 0.85f), fTab);

    // Текст в строке поиска
    float searchTextY = m_searchBoxPos.y + (m_searchBoxSize.y - fSearch * 28.0f) * 0.5f + 2.0f;
    if (m_searchQuery.empty()) {
        m_textRenderer->RenderText(LOC("LEVEL_SEARCH_HINT"), m_searchBoxPos.x + 8.0f, searchTextY, fSearch, glm::vec3(0.50f, 0.55f, 0.65f));
    } else {
        bool showCursor = m_isSearchActive && (m_cursorBlinkTimer < 0.5f);
        m_textRenderer->RenderText(m_searchQuery + (showCursor ? "|" : ""), m_searchBoxPos.x + 8.0f, searchTextY, fSearchIn, glm::vec3(0.35f, 0.95f, 1.0f));
    }

    // Текст чипов быстрых тегов
    for (const auto& chip : m_filterChips) {
        bool active = (m_searchQuery == chip.tag);
        float w = m_textRenderer->CalculateTextWidth("#" + chip.tag, fChip);
        float cy = chip.pos.y + (chip.size.y - fChip * 28.0f) * 0.5f + 2.0f;
        m_textRenderer->RenderText("#" + chip.tag, chip.pos.x + (chip.size.x - w) * 0.5f, cy, fChip, active ? glm::vec3(0.4f, 0.95f, 0.4f) : glm::vec3(0.65f, 0.80f, 0.70f));
    }

    // Текст на карточках
    for (const auto& card : m_levelCards) {
        float cx = card.cardBtn.pos.x;
        float cy = card.cardBtn.pos.y;

        // Название уровня
        m_textRenderer->RenderText(card.displayName, cx + 12.0f, cy + 10.0f, fCardName, glm::vec3(1.0f, 0.88f, 0.35f));

        // Бейдж Кампания/Тест
        float typeW = m_textRenderer->CalculateTextWidth(card.typeToggleBtn.text, fCardBadge);
        glm::vec3 typeColor = card.isCampaign ? glm::vec3(0.40f, 0.95f, 0.45f) : glm::vec3(1.0f, 0.75f, 0.35f);
        float badgeTextY = card.typeToggleBtn.pos.y + (card.typeToggleBtn.size.y - fCardBadge * 28.0f) * 0.5f + 2.0f;
        m_textRenderer->RenderText(card.typeToggleBtn.text, card.typeToggleBtn.pos.x + (card.typeToggleBtn.size.x - typeW) * 0.5f, badgeTextY, fCardBadge, typeColor);

        // Имя файла
        m_textRenderer->RenderText(card.filename, cx + 12.0f, cy + fCardName * 28.0f + 14.0f, fCardFile, glm::vec3(0.55f, 0.60f, 0.70f));

        // Теги карточки
        std::string tagLine;
        for (const auto& t : card.tags) {
            tagLine += "#" + t + " ";
        }
        if (tagLine.empty()) tagLine = "#default";
        m_textRenderer->RenderText(tagLine, cx + 12.0f, cy + fCardName * 28.0f + 14.0f + fCardFile * 26.0f + 6.0f, fCardTags, glm::vec3(0.40f, 0.80f, 0.65f));

        // Кнопки карточки
        drawCenterText(card.renameBtn, glm::vec3(0.85f, 0.88f, 0.95f), fCardSmBtn);
        drawCenterText(card.tagsBtn,   glm::vec3(0.85f, 0.88f, 0.95f), fCardSmBtn);
        drawCenterText(card.playBtn,   glm::vec3(0.95f, 1.0f,  0.95f), fCardPlayBtn);
    }

    // Текст нижних кнопок
    drawCenterText(m_btnBack,   glm::vec3(0.90f, 0.92f, 0.96f), fNavBtn);
    drawCenterText(m_btnEditor, glm::vec3(0.40f, 0.90f, 1.0f),  fNavBtn);

    if (m_totalPages > 1) {
        if (m_currentPage > 0)               drawCenterText(m_btnPrevPage, glm::vec3(0.85f, 0.90f, 1.0f), fPage);
        if (m_currentPage < m_totalPages - 1) drawCenterText(m_btnNextPage, glm::vec3(0.85f, 0.90f, 1.0f), fPage);

        std::string pageStr = LOC("LEVEL_PAGE") + " " + std::to_string(m_currentPage + 1) + " / " + std::to_string(m_totalPages);
        float pW = m_textRenderer->CalculateTextWidth(pageStr, fPage);
        m_textRenderer->RenderText(pageStr, (m_width - pW) * 0.5f, m_btnPrevPage.pos.y + 8.0f, fPage, glm::vec3(0.70f, 0.75f, 0.85f));
    }

    m_renderer->endBatch();

    // 7. Отрисовка модальных окон (если открыты)
    if (m_isRenameModalOpen) {
        m_renderer->beginBatch();
        renderRenameModal();
        m_renderer->endBatch();
    }

    if (m_isTagsModalOpen) {
        m_renderer->beginBatch();
        renderTagsModal();
        m_renderer->endBatch();
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
    m_renderer->drawSprite(m_whiteTexture, boxPos + glm::vec2(1.0f), boxSize - glm::vec2(2.0f), 0.0f, glm::vec3(0.08f, 0.09f, 0.12f));

    glm::vec2 btnSavePos(modalPos.x + 30.0f, modalPos.y + 148.0f);
    glm::vec2 btnSaveSize(190.0f, 40.0f);
    bool hovSave = isPointInRect(m_mousePos, btnSavePos, btnSaveSize);
    m_renderer->drawSprite(m_whiteTexture, btnSavePos, btnSaveSize, 0.0f, hovSave ? glm::vec3(0.35f, 0.95f, 0.50f) : glm::vec3(0.25f, 0.80f, 0.40f));
    m_renderer->drawSprite(m_whiteTexture, btnSavePos + glm::vec2(1.0f), btnSaveSize - glm::vec2(2.0f), 0.0f, hovSave ? glm::vec3(0.16f, 0.36f, 0.20f) : glm::vec3(0.12f, 0.28f, 0.16f));

    glm::vec2 btnCancelPos(modalPos.x + 240.0f, modalPos.y + 148.0f);
    glm::vec2 btnCancelSize(170.0f, 40.0f);
    bool hovCancel = isPointInRect(m_mousePos, btnCancelPos, btnCancelSize);
    m_renderer->drawSprite(m_whiteTexture, btnCancelPos, btnCancelSize, 0.0f, hovCancel ? glm::vec3(0.6f, 0.25f, 0.25f) : glm::vec3(0.45f, 0.20f, 0.20f));
    m_renderer->drawSprite(m_whiteTexture, btnCancelPos + glm::vec2(1.0f), btnCancelSize - glm::vec2(2.0f), 0.0f, hovCancel ? glm::vec3(0.28f, 0.14f, 0.14f) : glm::vec3(0.22f, 0.11f, 0.11f));

    m_renderer->flush();

    if (m_textRenderer) {
        std::string titleStr = LOC("RENAME_TITLE");
        float titleW = m_textRenderer->CalculateTextWidth(titleStr, 0.70f);
        m_textRenderer->RenderText(titleStr, modalPos.x + (modalSize.x - titleW) * 0.5f, modalPos.y + 10.0f, 0.70f, glm::vec3(1.0f, 0.85f, 0.25f));

        std::string sub = "File: " + m_renameTargetFileName;
        m_textRenderer->RenderText(sub, modalPos.x + 32.0f, modalPos.y + 55.0f, 0.50f, glm::vec3(0.70f, 0.75f, 0.85f));

        bool showCursor = (m_cursorBlinkTimer < 0.5f);
        std::string displayText = m_renameInputText + (showCursor ? "|" : "");
        m_textRenderer->RenderText(displayText, boxPos.x + 12.0f, boxPos.y + 10.0f, 0.62f, glm::vec3(0.40f, 0.95f, 1.0f));

        std::string saveStr = LOC("RENAME_SAVE") + " (Enter)";
        float sTxtW = m_textRenderer->CalculateTextWidth(saveStr, 0.55f);
        m_textRenderer->RenderText(saveStr, btnSavePos.x + (btnSaveSize.x - sTxtW) * 0.5f, btnSavePos.y + 12.0f, 0.55f, glm::vec3(0.95f));

        std::string cancelStr = LOC("RENAME_CANCEL") + " (Esc)";
        float cTxtW = m_textRenderer->CalculateTextWidth(cancelStr, 0.55f);
        m_textRenderer->RenderText(cancelStr, btnCancelPos.x + (btnCancelSize.x - cTxtW) * 0.5f, btnCancelPos.y + 12.0f, 0.55f, glm::vec3(0.95f));
    }
}

void LevelSelectState::renderTagsModal() {
    m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(0.0f), glm::vec2(m_width, m_height), 0.0f, glm::vec4(0.04f, 0.05f, 0.07f, 0.75f));

    glm::vec2 modalSize(500.0f, 380.0f);
    glm::vec2 modalPos((static_cast<float>(m_width) - modalSize.x) * 0.5f, (static_cast<float>(m_height) - modalSize.y) * 0.5f);

    m_renderer->drawSprite(m_whiteTexture, modalPos, modalSize, 0.0f, glm::vec3(0.12f, 0.13f, 0.17f));
    m_renderer->drawSprite(m_whiteTexture, modalPos, glm::vec2(modalSize.x, 1.0f), 0.0f, glm::vec3(0.35f, 0.85f, 1.0f));
    m_renderer->drawSprite(m_whiteTexture, modalPos + glm::vec2(0.0f, modalSize.y - 1.0f), glm::vec2(modalSize.x, 1.0f), 0.0f, glm::vec3(0.35f, 0.85f, 1.0f));
    m_renderer->drawSprite(m_whiteTexture, modalPos, glm::vec2(1.0f, modalSize.y), 0.0f, glm::vec3(0.35f, 0.85f, 1.0f));
    m_renderer->drawSprite(m_whiteTexture, modalPos + glm::vec2(modalSize.x - 1.0f, 0.0f), glm::vec2(1.0f, modalSize.y), 0.0f, glm::vec3(0.35f, 0.85f, 1.0f));

    float headerH = 38.0f;
    m_renderer->drawSprite(m_whiteTexture, modalPos, glm::vec2(modalSize.x, headerH), 0.0f, glm::vec3(0.16f, 0.18f, 0.24f));
    m_renderer->drawSprite(m_whiteTexture, modalPos + glm::vec2(0.0f, headerH), glm::vec2(modalSize.x, 2.0f), 0.0f, glm::vec3(0.35f, 0.85f, 1.0f));

    // Отрисовка чипов существующих тегов с кнопкой удаления (x)
    float tagX = modalPos.x + 30.0f;
    float tagY = modalPos.y + 55.0f;
    for (size_t i = 0; i < m_currentLevelTags.size(); ++i) {
        float tw = m_textRenderer ? (m_textRenderer->CalculateTextWidth(m_currentLevelTags[i], 0.48f) + 34.0f) : 70.0f;
        if (tagX + tw > modalPos.x + modalSize.x - 30.0f) {
            tagX = modalPos.x + 30.0f;
            tagY += 30.0f;
        }
        glm::vec2 tPos(tagX, tagY);
        glm::vec2 tSize(tw, 24.0f);
        m_renderer->drawSprite(m_whiteTexture, tPos, tSize, 0.0f, glm::vec3(0.30f, 0.50f, 0.40f));
        m_renderer->drawSprite(m_whiteTexture, tPos + glm::vec2(1.0f), tSize - glm::vec2(2.0f), 0.0f, glm::vec3(0.14f, 0.22f, 0.18f));

        // Крестик x
        glm::vec2 delPos(tagX + tw - 20.0f, tagY + 3.0f);
        glm::vec2 delSize(16.0f, 18.0f);
        m_renderer->drawSprite(m_whiteTexture, delPos, delSize, 0.0f, glm::vec3(0.55f, 0.20f, 0.20f));

        tagX += tw + 8.0f;
    }

    // Быстрые теги (пресеты)
    float curX = modalPos.x + 30.0f;
    float curY = modalPos.y + 195.0f;
    for (const auto& chip : m_quickAddChips) {
        if (curX + chip.size.x > modalPos.x + modalSize.x - 30.0f) {
            curX = modalPos.x + 30.0f;
            curY += chip.size.y + 6.0f;
        }
        bool hover = isPointInRect(m_mousePos, glm::vec2(curX, curY), chip.size);
        m_renderer->drawSprite(m_whiteTexture, glm::vec2(curX, curY), chip.size, 0.0f, hover ? glm::vec3(0.40f, 0.85f, 0.95f) : glm::vec3(0.25f, 0.35f, 0.45f));
        m_renderer->drawSprite(m_whiteTexture, glm::vec2(curX + 1.0f, curY + 1.0f), chip.size - glm::vec2(2.0f), 0.0f, glm::vec3(0.14f, 0.18f, 0.24f));
        curX += chip.size.x + 6.0f;
    }

    // Поле ввода нового тега
    glm::vec2 boxPos(modalPos.x + 30.0f, modalPos.y + modalSize.y - 100.0f);
    glm::vec2 boxSize(modalSize.x - 170.0f, 32.0f);
    m_renderer->drawSprite(m_whiteTexture, boxPos, boxSize, 0.0f, glm::vec3(0.35f, 0.85f, 1.0f));
    m_renderer->drawSprite(m_whiteTexture, boxPos + glm::vec2(1.0f), boxSize - glm::vec2(2.0f), 0.0f, glm::vec3(0.08f, 0.09f, 0.12f));

    // Кнопка Добавить
    glm::vec2 btnAddPos(modalPos.x + modalSize.x - 130.0f, modalPos.y + modalSize.y - 100.0f);
    glm::vec2 btnAddSize(100.0f, 32.0f);
    bool hovAdd = isPointInRect(m_mousePos, btnAddPos, btnAddSize);
    m_renderer->drawSprite(m_whiteTexture, btnAddPos, btnAddSize, 0.0f, hovAdd ? glm::vec3(0.35f, 0.95f, 0.50f) : glm::vec3(0.25f, 0.75f, 0.35f));
    m_renderer->drawSprite(m_whiteTexture, btnAddPos + glm::vec2(1.0f), btnAddSize - glm::vec2(2.0f), 0.0f, glm::vec3(0.14f, 0.28f, 0.18f));

    // Кнопка Готово
    glm::vec2 btnDonePos(modalPos.x + (modalSize.x - 140.0f) * 0.5f, modalPos.y + modalSize.y - 48.0f);
    glm::vec2 btnDoneSize(140.0f, 36.0f);
    bool hovDone = isPointInRect(m_mousePos, btnDonePos, btnDoneSize);
    m_renderer->drawSprite(m_whiteTexture, btnDonePos, btnDoneSize, 0.0f, hovDone ? glm::vec3(0.40f, 0.85f, 1.0f) : glm::vec3(0.30f, 0.65f, 0.85f));
    m_renderer->drawSprite(m_whiteTexture, btnDonePos + glm::vec2(1.0f), btnDoneSize - glm::vec2(2.0f), 0.0f, glm::vec3(0.14f, 0.22f, 0.32f));

    m_renderer->flush();

    if (m_textRenderer) {
        std::string titleStr = LOC("TAGS_TITLE");
        float titleW = m_textRenderer->CalculateTextWidth(titleStr, 0.65f);
        m_textRenderer->RenderText(titleStr, modalPos.x + (modalSize.x - titleW) * 0.5f, modalPos.y + 10.0f, 0.65f, glm::vec3(1.0f, 0.85f, 0.25f));

        // Текст существующих тегов
        float tx = modalPos.x + 30.0f;
        float ty = modalPos.y + 55.0f;
        for (const auto& t : m_currentLevelTags) {
            float tw = m_textRenderer->CalculateTextWidth(t, 0.48f) + 34.0f;
            if (tx + tw > modalPos.x + modalSize.x - 30.0f) {
                tx = modalPos.x + 30.0f;
                ty += 30.0f;
            }
            m_textRenderer->RenderText(t, tx + 6.0f, ty + 6.0f, 0.48f, glm::vec3(0.85f, 0.95f, 0.90f));
            m_textRenderer->RenderText("x", tx + tw - 16.0f, ty + 4.0f, 0.44f, glm::vec3(1.0f, 0.85f, 0.85f));
            tx += tw + 8.0f;
        }

        // Подпись популярных тегов
        m_textRenderer->RenderText(LOC("TAGS_POPULAR"), modalPos.x + 30.0f, modalPos.y + 172.0f, 0.46f, glm::vec3(0.70f, 0.75f, 0.85f));

        // Текст чипов
        curX = modalPos.x + 30.0f;
        curY = modalPos.y + 195.0f;
        for (const auto& chip : m_quickAddChips) {
            if (curX + chip.size.x > modalPos.x + modalSize.x - 30.0f) {
                curX = modalPos.x + 30.0f;
                curY += chip.size.y + 6.0f;
            }
            m_textRenderer->RenderText("+" + chip.tag, curX + 8.0f, curY + 6.0f, 0.44f, glm::vec3(0.40f, 0.85f, 0.95f));
            curX += chip.size.x + 6.0f;
        }

        // Текст в поле ввода нового тега
        if (m_tagInputText.empty()) {
            m_textRenderer->RenderText(LOC("TAGS_ADD_HINT"), boxPos.x + 8.0f, boxPos.y + 8.0f, 0.48f, glm::vec3(0.45f, 0.50f, 0.60f));
        } else {
            bool showCursor = (m_cursorBlinkTimer < 0.5f);
            m_textRenderer->RenderText(m_tagInputText + (showCursor ? "|" : ""), boxPos.x + 8.0f, boxPos.y + 8.0f, 0.52f, glm::vec3(0.35f, 0.95f, 1.0f));
        }

        // Кнопка Добавить
        std::string addStr = LOC("TAGS_ADD_BTN");
        float addW = m_textRenderer->CalculateTextWidth(addStr, 0.48f);
        m_textRenderer->RenderText(addStr, btnAddPos.x + (btnAddSize.x - addW) * 0.5f, btnAddPos.y + 8.0f, 0.48f, glm::vec3(0.95f));

        // Кнопка Готово
        std::string doneStr = LOC("TAGS_CLOSE");
        float doneW = m_textRenderer->CalculateTextWidth(doneStr, 0.55f);
        m_textRenderer->RenderText(doneStr, btnDonePos.x + (btnDoneSize.x - doneW) * 0.5f, btnDonePos.y + 10.0f, 0.55f, glm::vec3(0.95f));
    }
}

void LevelSelectState::resize(int width, int height) {
    m_width = width;
    m_height = height;
    init();
}