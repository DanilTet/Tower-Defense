#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>

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
	//float splashDamage;
};

class SpriteRenderer;
class Texture2D;
class Grid;
class Enemy;

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
	Enemy* m_currentTarget;

public:

	static TowerStats getStatsfromTowerType(TowerType type) {
		switch (type) {
			case TowerType::Basic:
				return { 200.0f, 1.0f, 10 };
			case TowerType::Sniper:
				return { 500.0f, 40.0f, 50 };
			case TowerType::Cannon:
				return { 120.0f, 2.0f, 30 };
		}
		return { 100.0f, 1.0f, 10 };
	}
	//конструктор
	Tower(int gridX, int gridY, TowerType type);

	//обновление логики
	void update(float dt, const std::vector<std::unique_ptr<Enemy>>& enemies, const Grid& grid);

	// отрисовка
	void render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> texture, const Grid& grid);
};