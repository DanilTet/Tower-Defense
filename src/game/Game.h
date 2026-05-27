#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <glm/glm.hpp>

class SpriteRenderer;
class Grid;
class Texture2D;

class Game {
public:
    int width, height;

    Game(int width, int height);
    ~Game();

    void init();
    void processInput(GLFWwindow* window, float dt);
    void update(float dt);
    void render();
    void resize(int width, int height);

private:
    std::unique_ptr<SpriteRenderer> m_renderer;
    std::unique_ptr<Grid>           m_gameGrid;
    std::shared_ptr<Texture2D>      m_cellTexture;

    glm::vec2 m_gridOffset;
    bool      m_mousePressedLastFrame;
};