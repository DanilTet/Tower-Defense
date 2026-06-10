#pragma once
#include <vector>
#include <memory>
#include "entities/Enemy.h"
#include "entities/Tower.h"
#include "entities/Projectile.h"

class Grid;
class SpriteRenderer;
class Texture2D;
struct PlayerStats;

class EntityManager {
private:
	std::vector<std::unique_ptr<Projectile>> m_projectiles; // проджектайлы
	std::vector<std::unique_ptr<Tower>> m_towers;// БАШНИ
	std::vector<std::unique_ptr<Enemy>> m_enemies; // вектор для хранения всех врагов на уровне

public:
	// методы обновления и рендера
	void update(float dt, Grid& gameGrid, PlayerStats& stats);
	void render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> cellTex, std::shared_ptr<Texture2D> radiusTex, Grid& gameGrid);

	// методы добавления
	void addEnemy(std::unique_ptr<Enemy> enemy) { m_enemies.push_back(std::move(enemy)); }
	void addTower(std::unique_ptr<Tower> tower) { m_towers.push_back(std::move(tower)); }
	void addProjectile(std::unique_ptr<Projectile> proj) { m_projectiles.push_back(std::move(proj)); }

	// геттеры
	std::vector<std::unique_ptr<Tower>>& getTowers() { return m_towers; }
	std::vector<std::unique_ptr<Enemy>>& getEnemies() { return m_enemies; }
};