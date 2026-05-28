#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <glm/glm.hpp>

class SpriteRenderer;
class Grid;
class Texture2D;
class Enemy;

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
    std::unique_ptr<Grid> m_gameGrid;
    std::shared_ptr<Texture2D> m_cellTexture;
    std::unique_ptr<Enemy> m_testEnemy;
    glm::vec2 m_gridOffset;
    bool m_mousePressedLastFrame;


	// Движение врага и спавн
	std::vector<glm::ivec2> m_levelPath; // маршрут врага по клеткам сетки
	std::vector<std::unique_ptr<Enemy>> m_enemies; // вектор для хранения всех врагов на уровне
	void spawnEnemy(float speed = 100.0f); // функция для спавна врага
};