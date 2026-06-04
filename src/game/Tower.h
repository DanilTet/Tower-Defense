#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include "CircleCollider.h"

// типы башен
enum class TowerType {
	Basic,
	Sniper,
	Cannon
};

//конфиг характеристик башни
struct TowerStats {
	float range; // радиус атаки
	float fireRate; // скорость атаки
	int damage; // урон
	int cost;
	float splashRadius;
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

public:
	// получаем характеристики башни
	static TowerStats getStatsfromTowerType(TowerType type);

	//конструктор
	Tower(int gridX, int gridY, TowerType type);

	//обновление логики
	void update(float dt, const std::vector<std::unique_ptr<Enemy>>& enemies, std::vector<std::unique_ptr<Projectile>>& projectiles, const Grid& grid);

	// отрисовка
	void render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> texture, std::shared_ptr<Texture2D> radiusTexture, const Grid& grid);
};