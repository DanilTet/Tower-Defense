#pragma once
#include "IGameState.h"
#include <memory>
#include <string>
#include <glm/glm.hpp>

class GameStateManager;
class SpriteRenderer;
class TextRenderer;
class Texture2D;

// кнопка
struct UIButton {
    glm::vec2 pos;
    glm::vec2 size;
    std::string text;
    int state; // 0 = Idle, 1 = Hover, 2 = Pressed
};

class PauseState : public IGameState {
private:
    GameStateManager& m_stateManager;
    int m_width, m_height;
    std::shared_ptr<SpriteRenderer> m_renderer;
    TextRenderer* m_textRenderer;
    // Ui текстурка
    std::shared_ptr<Texture2D> m_uiTexture;
    //переменные мыши
    bool m_mousePressedLastFrame;
    glm::vec2 m_currentMousePos;
    // переменніе окна
    glm::vec2 m_windowPos;
    glm::vec2 m_windowSize;
    float m_headerHeight;
    bool m_isDragging;
    glm::vec2 m_dragOffset;
    //кнопки
    UIButton m_btnResume;
    UIButton m_btnExit;

    bool isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize);

public:
    PauseState(GameStateManager& stateManager, int width, int height, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer);

    void init() override;
    void cleanup() override;
    void processInput(GLFWwindow* window, float dt) override;
    void update(float dt) override;
    void render() override;
    void resize(int width, int height) override;
};