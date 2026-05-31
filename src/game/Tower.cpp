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

void Tower::render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> texture, std::shared_ptr<Texture2D> radiusTexture, const Grid& grid) {
	float cellSize = grid.getCellSize(); //подтягиваем рязмер чтобы в клетку попала башня
	glm::vec2 size(cellSize, cellSize); // создаем вектор чтобы башня идеально стала в клетку
	glm::vec2 pixelPos = grid.gridToPixel(m_gridX, m_gridY); // получаем пиксели клетки
	glm::vec2 towerCenter = pixelPos + glm::vec2(cellSize / 2.0f);

	// РАДИУС АТАКИ
	float currentPixelRange = m_range * cellSize; //считаем реальный радиус атаки от размера текущего окна
	glm::vec2 radiusSize(currentPixelRange * 2.0f, currentPixelRange * 2.0f); // размер спрайта радиуса
	glm::vec2 radiusPos = towerCenter - glm::vec2(currentPixelRange); // сдвиг по левому верхнему уголу
	// рисуем радиус атаки
	renderer->drawSprite(radiusTexture, radiusPos, radiusSize, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f));

	// БАШНЯ
	glm::vec3 color(1.0f);
	if (m_type == TowerType::Basic) color = glm::vec3(0.5f, 0.5f, 0.5f);
	else if (m_type == TowerType::Sniper) color = glm::vec3(0.8f, 0.2f, 0.2f);
	else if (m_type == TowerType::Cannon) color = glm::vec3(0.2f, 0.2f, 0.8f);
	renderer->drawSprite(texture, pixelPos, size, 0.0f, color);
}

void Tower::update(float dt, const std::vector<std::unique_ptr<Enemy>>& enemies, const Grid& grid) {
	//если башня еще не перезарядилась, перезаряжаем
	if (m_shotTimer > 0.0f) {
		m_shotTimer -= dt;
	}
	float cellSize = grid.getCellSize(); // достаем размер клетки для центров
	glm::vec2 towerCenter = grid.gridToPixel(m_gridX, m_gridY) + glm::vec2(cellSize / 2.0f);

	float currentPixelRange = m_range * cellSize; // пересчитываем дальность стрельбы

	if (m_currentTarget != nullptr) {
		if (!m_currentTarget->isReachedEnd() && !m_currentTarget->isDead()) {
			glm::vec2 enemyCenter = m_currentTarget->getPixelPos();

			float dist = glm::distance(towerCenter, enemyCenter);
			if (dist > currentPixelRange) {
				m_currentTarget = nullptr;
			}
		}
		else {
			m_currentTarget = nullptr;
		}
	}
	if (m_currentTarget == nullptr) {
		for (const auto& enemy : enemies) {
			if (!enemy || enemy->isReachedEnd() || enemy->isDead()) continue;
			glm::vec2 enemyCenter = enemy->getPixelPos();
			float dist = glm::distance(towerCenter, enemyCenter);
			if (dist <= currentPixelRange) {
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