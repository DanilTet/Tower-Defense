#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "Enemy.h"
#include "../renderer/TextRenderer.h"
#include "Projectile.h"
#include "Tower.h"

// Forward declarations ускоряет компиляцию и уменьшает количество включаемых заголовочных файлов
// типа просто говорим компилятору, что эти классы существуют, а их определения будут в соответствующих заголовочных файлах
class SpriteRenderer;
class Grid;
class Texture2D;
class Enemy;
class Tower;
class WaveManager;
class Pathfinder;

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

	void spawnEnemy(EnemyType type, int spawnerIndex = 0); // функция для спавна врага
	void startNextWave();

	void renderPathArrows(); // метод ренгера стрелочек пути

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
	std::unique_ptr<WaveManager> m_waveManager; // менеджер волн

	std::vector<std::unique_ptr<Tower>> m_towers;// БАШНИ

	std::unique_ptr<TextRenderer> m_textRenderer; // текст

	std::vector<std::unique_ptr<Projectile>> m_projectiles; // проджектайлы

	float m_pathAnimationTimer = 0.0f; // таймер для анимации стрелочек

	//TowerType m_selectedTowerType = TowerType::Basic; // какая башня выбрана щас

	TowerType m_selectedTowerType = TowerType::None; //какая башня вібрана

	// ИНТЕРФЕЙС ПАНЕЛИ ВІБОРА БАШНИ
	static constexpr float UI_PANEL_WIDTH = 350.0f; // длина панели
	static constexpr float UI_PANEL_HEIGHT = 120.0f; // вісота панели
	static constexpr float UI_ICON_SIZE = 60.0f; // размер иконки
	static constexpr float UI_ICON_PADDING = 110.0f; // отступ
	static constexpr float UI_OFFSET_X = 20.0f; // Отступ иконок от левого края панели
	static constexpr float UI_OFFSET_Y = 30.0f; // Отступ иконок от верхнего края панели


	glm::vec2 getUIPanelPos() const;
	glm::vec2 getTowerIconPos(int index) const;

	glm::vec2 m_currentMousePos; // позиция мыши каждый кадр

	void renderUI(); // отриовка интерфейса
	void renderHologram(); // метод лоя рисования голограммы


	// точки спавна и точки баз
	std::vector<glm::ivec2> m_spawners;
	std::vector<glm::ivec2> m_bases;

	//маршруты
	//список маршрутов даже так
	std::vector<std::vector<glm::ivec2>> m_paths;

	std::unique_ptr<Pathfinder> m_pathfinder; // указатель на алгоритм поиска путиі
};