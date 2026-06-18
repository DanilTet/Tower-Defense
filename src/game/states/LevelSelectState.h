#pragma once
#include "IGameState.h"
#include <memory>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "PauseState.h"

class GameStateManager;
class SpriteRenderer;
class TextRenderer;
class Texture2D;

// структура привязует джейсон к уровню короче ты понял влад
struct LevelButton {
    UIButton btn;
    std::string levelPath;
};

class LevelSelectState : public IGameState {
private:
    GameStateManager& m_stateManager;
    int m_width, m_height;
    std::shared_ptr<SpriteRenderer> m_renderer;
    TextRenderer* m_textRenderer;
    std::shared_ptr<Texture2D> m_uiTexture;

    bool m_mousePressedLastFrame = false;
    // все кнопки
    std::vector<LevelButton> m_levelButtons;
    UIButton m_btnBack; // камбек в главное меню

    bool isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize);

public:
    LevelSelectState(GameStateManager& stateManager, int width, int height, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer);
    void init() override;
    void cleanup() override;
    void processInput(GLFWwindow* window, float dt) override;
    void update(float dt) override;
    void render() override;
    void resize(int width, int height) override;
};