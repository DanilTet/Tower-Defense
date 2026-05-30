#include "Tower.h"
#include "Grid.h"
#include "Enemy.h"
#include "renderer/SpriteRenderer.h"

// конструктор
Tower::Tower(int gridX, int gridY, TowerType type)
	: m_gridX(gridX),
	m_gridY(gridY),
	m_type(type) {
	// считываем статистику
	TowerStats stats = Tower::getStatsfromTowerType(type);
	m_range = stats.range; // присваеваем радиус атаки
	m_damage = stats.damage; // присваиваем урон
	m_fireRate = stats.fireRate; // присваеваем скорость атаки
	
	m_currentTarget = nullptr; // текущая цель это нулпоинтер

	m_shotTimer = 0.0f; // переменная таймер
}

void Tower::render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> texture, const Grid& grid) {
	// получаем пиксели клетки
	glm::vec2 pixelPos = grid.gridToPixel(m_gridX, m_gridY);
	
	//подтягиваем рязмер чтобы в клетку попала башня
	float cellSize = grid.getCellSize();
	// создаем вектор чтобы башня идеально стала в клетку
	glm::vec2 size(cellSize, cellSize);
	
	glm::vec3 color(1.0f);
	if (m_type == TowerType::Basic) color = glm::vec3(0.5f, 0.5f, 0.5f);
	else if (m_type == TowerType::Sniper) color = glm::vec3(0.8f, 0.2f, 0.2f);
	else if (m_type == TowerType::Cannon) color = glm::vec3(0.2f, 0.2f, 0.8f);
	renderer->drawSprite(texture, pixelPos, size, 0.0f, color);
}

void Tower::update(float dt, const std::vector<std::unique_ptr<Enemy>>& enemies, const Grid& grid) {
	if (m_shotTimer > 0.0f) {
		m_shotTimer -= dt;
	}
	float cellSize = grid.getCellSize(); // достаем размер клетки для центров
	glm::vec2 towerCenter = grid.gridToPixel(m_gridX, m_gridY) + glm::vec2(cellSize / 2.0f);

	if (m_currentTarget != nullptr) {
		if (!m_currentTarget->isReachedEnd()) {
			glm::vec2 enemyCenter = m_currentTarget->getPixelPos();

			float dist = glm::distance(towerCenter, enemyCenter);
			if (dist > m_range) {
				m_currentTarget = nullptr;
			}
		}
		else {
			m_currentTarget = nullptr;
		}
	}
	if (m_currentTarget == nullptr) {
		for (const auto& enemy : enemies) {
			if (!enemy || enemy->isReachedEnd()) continue;
			glm::vec2 enemyCenter = enemy->getPixelPos();
			float dist = glm::distance(towerCenter, enemyCenter);
			if (dist <= m_range) {
				m_currentTarget = enemy.get();
				break;
			}
		}
	}
	if (m_currentTarget && m_shotTimer <= 0.0f) {
		m_currentTarget->takeDamage(m_damage);
		m_shotTimer = m_fireRate;
	}
}