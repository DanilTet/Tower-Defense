#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include "CircleCollider.h"
#include <string>

// типы башен
enum class TowerType {
	Basic,
	Sniper,
	Cannon,
	None
};

//конфиг характеристик башни
struct TowerStats {
	float range; // радиус атаки
	float fireRate; // скорость атаки
	int damage; // урон
	int cost; // цена башни
	float splashRadius; // сплеш урон
	float rotationSpeed; // скорость поворота башни

	std::string buildSound; // звук строительства
	std::string attackSound; // звук атаки

};

class SpriteRenderer;
class Texture2D;
class Grid;
class Enemy;
class Projectile;

class Tower {
private:
	TowerType m_type; //тип башни
	float m_range; //радиус атаки
	float m_fireRate; //скорость атаки
	int m_damage; //урон

	int m_gridX; // індекс столбца Х
	int m_gridY; // индекс рядка У

	//float m_splashDamage;

	float m_shotTimer;

	int m_currentLevel; // текущий левел
	int m_maxLevel; // макс левел


	float m_angle = 0.0f;
	float m_rotationSpeed;

	// debug
	bool m_showDebugArrow = true;

	// кеш
	float m_splashRadius;
	std::string m_attackSound;
	std::string m_buildSound;

public:
	// получаем характеристики башни
	static TowerStats getStatsfromTowerType(TowerType type);

	//конструктор
	Tower(int gridX, int gridY, TowerType type);

	//обновление логики
	void update(float dt, const std::vector<std::unique_ptr<Enemy>>& enemies, std::vector<Projectile>& projectiles, const Grid& grid);

	// отрисовка
	void render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> texture, std::shared_ptr<Texture2D> radiusTexture, std::shared_ptr<Texture2D> arrowTexture, const Grid& grid);

	bool upgrade(int& playerMoney);
	int getUpgradeCost() const;

	int getGridX() const { return m_gridX; }
	int getGridY() const { return m_gridY; }
	int getLevel() const { return m_currentLevel; }	

};