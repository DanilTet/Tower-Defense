#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <glm/glm.hpp>
#include "Enemy.h"
#include "../renderer/TextRenderer.h"

// Forward declarations ускоряет компиляцию и уменьшает количество включаемых заголовочных файлов
// типа просто говорим компилятору, что эти классы существуют, а их определения будут в соответствующих заголовочных файлах
class SpriteRenderer;
class Grid;
class Texture2D;
class Enemy;
class Tower;

class Game {
public:
	int width, height; // размеры окна, которые могут изменяться при ресайзе

    Game(int width, int height);
    ~Game();

    void init();
    void processInput(GLFWwindow* window, float dt);
    void update(float dt);
    void render();
    void resize(int width, int height);

private:
	int m_playerMoney; // деньги игрока
	int m_baseHealth; // здоровье базы

	std::unique_ptr<SpriteRenderer> m_renderer; // рендерер для отрисовки спрайтов
	std::unique_ptr<Grid> m_gameGrid; // указатель на сетку

	// текстурки 
	std::shared_ptr<Texture2D> m_cellTexture; // текстура для клеток сетки и врагов
	std::shared_ptr<Texture2D> m_grassTexture; // текстура травы сетки
	std::shared_ptr<Texture2D> m_radiusTexture; // текстура радиус атаки


	std::unique_ptr<Enemy> m_testEnemy; // удалить после тестов!!!!
	bool m_mousePressedLastFrame; // флаг для отслеживания состояния мыши, чтобы не спавнить башню при каждом кадре, когда мышь нажата


	// Движение врага и спавн
	std::vector<glm::ivec2> m_levelPath; // маршрут врага по клеткам сетки
	std::vector<std::unique_ptr<Enemy>> m_enemies; // вектор для хранения всех врагов на уровне
	void spawnEnemy(EnemyType type); // функция для спавна врага

	std::vector<std::unique_ptr<Tower>> m_towers;// БАШНИ

	std::unique_ptr<TextRenderer> m_textRenderer; // текст
	
};