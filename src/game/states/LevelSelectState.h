#pragma once
#include "IGameState.h"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include "PauseState.h"
#include "../core/LevelManager.h"

class GameStateManager;
class SpriteRenderer;
class TextRenderer;
class Texture2D;

struct LevelCardUI {
    UIButton cardBtn;
    std::string levelPath;
    std::string filename;
    std::string displayName;
    bool isBuiltIn = false;
    UIButton renameBtn;
};

class LevelSelectState : public IGameState {
private:
    GameStateManager& m_stateManager;
    int m_width, m_height;
    std::shared_ptr<SpriteRenderer> m_renderer;
    TextRenderer* m_textRenderer;
    std::shared_ptr<Texture2D> m_uiTexture;
    std::shared_ptr<Texture2D> m_whiteTexture;

    bool m_mousePressedLastFrame = false;
    glm::vec2 m_mousePos{ 0.0f, 0.0f };

    std::vector<LevelCardUI> m_levelCards;
    UIButton m_btnBack;
    UIButton m_btnEditor;

    int m_currentPage = 0;
    int m_totalPages = 1;
    UIButton m_btnPrevPage;
    UIButton m_btnNextPage;

    // Модальное окно переименования
    bool m_isRenameModalOpen = false;
    std::string m_renameInputText = "";
    std::string m_renameTargetFileName = "";
    float m_cursorBlinkTimer = 0.0f;

    struct KeyRepeatState {
        bool isDown = false;
        float holdTimer = 0.0f;
        float repeatTimer = 0.0f;
    };
    std::unordered_map<int, KeyRepeatState> m_keyStates;

    bool isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize);
    void openRenameModal(const std::string& targetFileName);
    void confirmRename();
    bool processRenameModalInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, float dt);
    void renderRenameModal();

public:
    LevelSelectState(GameStateManager& stateManager, int width, int height, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer);
    void init() override;
    void cleanup() override;
    void processInput(GLFWwindow* window, float dt) override;
    void update(float dt) override;
    void render() override;
    void resize(int width, int height) override;
};