#include "Tower.h"
#include "world/Grid.h"
#include "Enemy.h"
#include "renderer/SpriteRenderer.h"
#include "Projectile.h"
#include "core/ConfigManager.h"
#include "../core/EventBus.h"

TowerStats Tower::getStatsfromTowerType(const std::string& type) {
	return ConfigManager::getTowerStats(type);
}

// конструктор
Tower::Tower(int gridX, int gridY, const std::string& type)
	: m_gridX(gridX),
	m_gridY(gridY),
	m_type(type),
	m_currentLevel(1), // бам бам с первого уровня
	m_maxLevel(3) // макс левел башни
{
	// считываем статистику
	TowerStats stats = Tower::getStatsfromTowerType(type);
	m_range = stats.range; // присваеваем радиус атаки
	m_damage = stats.damage; // присваиваем урон
	m_fireRate = stats.fireRate; // присваеваем скорость атаки
	m_rotationSpeed = stats.rotationSpeed; // присвоение скорости поворота башни
	//m_currentTarget = nullptr; // текущая цель это нулпоинтер

	// кеш
	m_splashRadius = stats.splashRadius;
	m_attackSound = stats.attackSound;
	m_buildSound = stats.buildSound;

	m_textureId = stats.textureId;
	m_color = stats.color;

	m_shotTimer = 0.0f; // переменная таймер
}

void Tower::render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> texture, std::shared_ptr<Texture2D> radiusTexture, std::shared_ptr<Texture2D> arrowTexture, const Grid& grid) {
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
	glm::vec3 color = m_color;

	// зависимо от левела башни меняем цвет
	if (m_currentLevel == 2) color += glm::vec3(0.2f, 0.2f, 0.2f);
	if (m_currentLevel == 3) color += glm::vec3(0.4f, 0.4f, 0.4f);

	renderer->drawSprite(texture, pixelPos, size, 0.0f, color);


	// debug стрелочка
	if (m_showDebugArrow && arrowTexture != nullptr) {
		glm::vec2 arrowSize = size * 0.8f;
		glm::vec2 arrowPos = pixelPos + (size - arrowSize) * 0.5f;
		renderer->drawSprite(arrowTexture, arrowPos, arrowSize, m_angle, glm::vec3(0.5f, 1.0f, 0.5f));
	}

}

void Tower::update(float dt, const std::vector<std::unique_ptr<Enemy>>& enemies, std::vector<Projectile>& projectiles, const Grid& grid) {
	//если башня еще не перезарядилась, перезаряжаем
	if (m_shotTimer > 0.0f) {
		m_shotTimer -= dt;
	}
	
	
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
		
	// если нашли врага то наводимся на него
	if (bestTarget != nullptr) {
		// получаем центр врага
		glm::vec2 enemyCenter = bestTarget->getCollider(grid).center;
		// вектор направления к цели от башни до врага
		glm::vec2 dir = enemyCenter - towerCenter;
		// получаем градусы на которые надо повернуться
		float targetAngle = glm::degrees(atan2(dir.y, dir.x));

		// вычисляем разницу между текущим углом башни и нужным углом -180 до 180
		float angleDiff = targetAngle - m_angle;
		while (angleDiff > 180.0f) angleDiff -= 360.0f;
		while (angleDiff < -180.0f) angleDiff += 360.0f;

		// если дуло почти смотрит на врага и разница меньше того что мы проверяем за этот кадр
		if (abs(angleDiff) <= m_rotationSpeed * dt) {
			
			float currentSplash = ConfigManager::getTowerStats(m_type, m_currentLevel).splashRadius;

			m_angle = targetAngle; // фиксируем прицел на враге
			// проверяем перезарядку
			if (m_shotTimer <= 0.0f) {
				// ищем свободную пулю в масиве
				Projectile* freeProj = nullptr;
				for (auto& proj : projectiles) {
					if (!proj.isActive()) {
						freeProj = &proj;
						break;
					}
				}

				// если нашли пулю то будим этого бизнесмена
				if (freeProj) {
					freeProj->init(towerCenter, m_angle, 800.0f, m_damage, bestTarget->getId(), m_splashRadius, currentPixelRange);
				}

				// формируем поссылку
				Event e;
				e.type = EventType::TowerFired;
				e.textData = m_attackSound;
				EventBus::publish(e);

				m_shotTimer = m_fireRate;

			}
		}
		else {
			float direction = 0.0f;
			if (angleDiff > 0) {
				direction = 1.0f;
			}
			else {
				direction = -1.0f;
			}

			float rotationStep = direction * m_rotationSpeed * dt;

			m_angle += rotationStep;
			if (m_angle > 180.0f) m_angle -= 360.0f;
			if (m_angle < -180.0f) m_angle += 360.0f;
		}	
	}
}

// функция улучшения башни
bool Tower::upgrade(int& playerMoney) {
	if (m_currentLevel >= m_maxLevel) return false; // достигнут максимум

	// увеличиваем лвл
	int nextLevel = m_currentLevel + 1;
	TowerStats nextStats = ConfigManager::getTowerStats(m_type, nextLevel);

	if (playerMoney >= nextStats.cost) {
		playerMoney -= nextStats.cost; // заберяем деняк
		m_currentLevel = nextLevel;

		// теперь новые статы
		m_range = nextStats.range;
		m_damage = nextStats.damage;
		m_fireRate = nextStats.fireRate;
		m_rotationSpeed = nextStats.rotationSpeed;

		// обновляем кеш
		m_splashRadius = nextStats.splashRadius;
		m_attackSound = nextStats.attackSound;
		m_buildSound = nextStats.buildSound;
		m_textureId = nextStats.textureId;
		m_color = nextStats.color;

		// формируем посылку с кастомным звуком апгрейда
		Event e;
		e.type = EventType::TowerBuilt;
		e.textData = nextStats.buildSound;
		EventBus::publish(e);

		return true;
	}

	return false; // Не хватило денег
}

int Tower::getUpgradeCost() const {
	if (m_currentLevel >= m_maxLevel) return 0;
	return ConfigManager::getTowerStats(m_type, m_currentLevel + 1).cost;
}