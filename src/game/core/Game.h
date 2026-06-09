#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "entities/Enemy.h"
#include "../renderer/TextRenderer.h"
#include "entities/Projectile.h"
#include "entities/Tower.h"
#include "ui/BuildPanel.h"
#include "ui/PlacementUI.h"
#include "ui/PathRenderer.h"
#include "ui/StatsPanel.h"
#include "gameplay/BuildManager.h"
#include "gameplay/EntityManager.h"

// Forward declarations ускоряет компиляцию и уменьшает количество включаемых заголовочных файлов
// типа просто говорим компилятору, что эти классы существуют, а их определения будут в соответствующих заголовочных файлах
class SpriteRenderer;
class Grid;
class Texture2D;
class Enemy;
class Tower;
class WaveManager;
class Pathfinder;
class EntityManager;

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
	std::unique_ptr<WaveManager> m_waveManager; // менеджер волн


	std::unique_ptr<TextRenderer> m_textRenderer; // текст


	TowerType m_selectedTowerType = TowerType::None; //какая башня вібрана

	glm::vec2 m_currentMousePos; // позиция мыши каждый кадр

	// точки спавна и точки баз
	std::vector<glm::ivec2> m_spawners;
	std::vector<glm::ivec2> m_bases;

	//маршруты
	//список маршрутов даже так
	std::vector<std::vector<glm::ivec2>> m_paths;

	std::unique_ptr<Pathfinder> m_pathfinder; // указатель на алгоритм поиска путиі

	// ИНТЕРФЕЙС
	std::unique_ptr<Buildpanel> m_buildPanel;//указатель на панельку стоительства
	std::unique_ptr<PlacementUI> m_placementUI; // указатель на голограму строительства
	std::unique_ptr<PathVisualizer> m_pathVisualizer; // указатель на стрелочки пути
	std::unique_ptr<StatsPanel> m_statsPanel; // указатель на панелтку статистик ну вот это здоровье деньки и тд

	// Геймплей
	std::unique_ptr<BuildManager> m_buildManager; // укащатель на абгрейдер та строитель башен
	std::unique_ptr<EntityManager> m_entityManager; // менеджер отвечающий за отрисовку и обновление всех башен, врагов и пули
};