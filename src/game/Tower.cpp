#include "Tower.h"
#include "Grid.h"
#include "Enemy.h"
#include "renderer/SpriteRenderer.h"
#include "../audio/AudioManager.h"
#include "Projectile.h"
#include "ConfigManager.h"

TowerStats Tower::getStatsfromTowerType(TowerType type) {
	return ConfigManager::getTowerStats(type);
}

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
	
	//m_currentTarget = nullptr; // текущая цель это нулпоинтер

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

void Tower::update(float dt, const std::vector<std::unique_ptr<Enemy>>& enemies, std::vector<std::unique_ptr<Projectile>>& projectiles, const Grid& grid) {
	//если башня еще не перезарядилась, перезаряжаем
	if (m_shotTimer > 0.0f) {
		m_shotTimer -= dt;
	}
	
	// если башня заряжена
	if (m_shotTimer <= 0.0f) {
		// считаем центр башни
		float cellSize = grid.getCellSize();
		glm::vec2 towerCenter = grid.gridToPixel(m_gridX, m_gridY) + glm::vec2(cellSize / 2.0f);
		
		float currentPixelRange = m_range * cellSize; // считаем радиус атаки в пикселях

		// создание хитбокса башни в понимании зоны поражения
		CircleCollider towerCollider = { towerCenter, currentPixelRange };

		// ПЕРЕМЕННІЕ ДЛЯ ПОИСКА ЛУЧШЕЙ ЦЕЛИ
		Enemy* bestTarget = nullptr; // указатели на лучшую цель
		float maxDistance = -1.0f; // рекорд пройденого пути


		// проходим по врагам
		for (const auto& enemy : enemies) {
			// пропускаем мертвіх или тех кто дошел до финала
			if (!enemy || enemy->isReachedEnd() || enemy->isDead()) continue;

			// берем хитбокс врага и проверяем столкновение с зоной башни
			if (towerCollider.intersects(enemy->getCollider(grid))) {
				
				if (enemy->getDistanceTraveled() > maxDistance) {
					maxDistance = enemy->getDistanceTraveled(); // обновляем рекорд пути
					bestTarget = enemy.get(); // запоминаем врага
				}
			}
		}

		if (bestTarget != nullptr) {
			// получаем центр врага
			glm::vec2 enemyCenter = bestTarget->getCollider(grid).center;

			// достаем радиус сплеша
			float currentSplash = Tower::getStatsfromTowerType(m_type).splashRadius;

			// Создаем пулю
			auto newProj = std::make_unique<Projectile>(towerCenter, enemyCenter, 800.0f, m_damage, bestTarget->getId(), currentSplash);
			projectiles.push_back(std::move(newProj));

			AudioManager::playSound("res/sounds/shoot.wav", 0.1f); // играем звук

			m_shotTimer = m_fireRate;
		}
	}

}