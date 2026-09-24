#include "Tower.h"
#include "world/Grid.h"
#include "Enemy.h"
#include "renderer/SpriteRenderer.h"
#include "Projectile.h"
#include "core/ConfigManager.h"
#include "../core/EventBus.h"
#include "../../particles/ParticleSystem.h"
#include "../gameplay/EntityManager.h"
#include "TowerTargetingSystem.h"

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
	applyStats(stats);

	m_shotTimer = 0.0f; // переменная таймер
}

void Tower::render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> atlasTexture, std::shared_ptr<Texture2D> radiusTexture, std::shared_ptr<Texture2D> arrowTexture, const Grid& grid, bool isSelected) {
	float cellSize = grid.getCellSize(); //подтягиваем рязмер чтобы в клетку попала башня
	glm::vec2 size(cellSize, cellSize); // создаем вектор чтобы башня идеально стала в клетку
	glm::vec2 pixelPos = grid.gridToPixel(m_gridX, m_gridY); // получаем пиксели клетки
	glm::vec2 towerCenter = pixelPos + glm::vec2(cellSize / 2.0f);

	// БАШНЯ
	glm::vec3 color = m_color;
	if (m_currentLevel == 2) color += glm::vec3(0.2f, 0.2f, 0.2f);
	if (m_currentLevel == 3) color += glm::vec3(0.4f, 0.4f, 0.4f);

	// режем башню
	SpriteUV towerUV = ConfigManager::getUV("main_atlas", "tower_basic");

	// рисуем (добавляем 90 градусов, чтобы выровнять текстуру дула с направлением выстрела)
	renderer->drawSprite(atlasTexture, pixelPos, size, m_angle + 90.0f, color, towerUV);

	if (isSelected) {
		float currentPixelRange = m_range * cellSize;
		glm::vec2 radiusSize(currentPixelRange * 2.0f, currentPixelRange * 2.0f);
		glm::vec2 radiusPos = towerCenter - glm::vec2(currentPixelRange);

		renderer->drawSprite(radiusTexture, radiusPos, radiusSize, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	}

	// debug стрелочка
	if (m_showDebugArrow && arrowTexture != nullptr) {
		glm::vec2 arrowSize = size * 0.8f;
		glm::vec2 arrowPos = pixelPos + (size - arrowSize) * 0.5f;
		renderer->drawSprite(arrowTexture, arrowPos, arrowSize, m_angle, glm::vec3(0.5f, 1.0f, 0.5f));
	}

}

void Tower::update(float dt, const std::vector<std::unique_ptr<Enemy>>& enemies, EntityManager& entityManager, const Grid& grid, ParticleSystem& particleSystem) {
	//если башня еще не перезарядилась, перезаряжаем
	if (m_shotTimer > 0.0f) {
		m_shotTimer -= dt;
	}
	
	
	// считаем центр башни
	float cellSize = grid.getCellSize();
	glm::vec2 towerCenter = grid.gridToPixel(m_gridX, m_gridY) + glm::vec2(cellSize / 2.0f);
	float currentPixelRange = m_range * cellSize;

	Enemy* bestTarget = TowerTargetingSystem::selectTarget(towerCenter, currentPixelRange, m_targetMode, enemies, grid);

	if (bestTarget != nullptr) {
		glm::vec2 enemyCenter = bestTarget->getCollider(grid).center;
		bool isAimed = TowerTargetingSystem::updateAim(m_angle, towerCenter, enemyCenter, m_rotationSpeed, dt);

		if (isAimed) {
			float currentSplash = ConfigManager::getTowerStats(m_type, m_currentLevel).splashRadius;

			if (m_shotTimer <= 0.0f) {
				Projectile* freeProj = entityManager.getFreeProjectile();
				if (freeProj) {
					freeProj->setParticleEffects(m_trailParticle, m_impactParticle);
					freeProj->setVisuals(m_bulletTextureId, m_bulletBaseSize);
					freeProj->init(towerCenter, m_angle, m_bulletSpeed, m_damage, bestTarget->getId(), m_splashRadius, currentPixelRange);
				}

				if (!m_muzzleParticle.empty()) {
					ParticleEmitterProps muzzle = ConfigManager::getParticleProps(m_muzzleParticle);
					glm::vec2 shootDirection = glm::vec2(cos(glm::radians(m_angle)), sin(glm::radians(m_angle)));
					muzzle.position = towerCenter + shootDirection * (cellSize * 0.4f);
					float force = glm::length(muzzle.velocityDir);
					muzzle.velocityDir = shootDirection * force;
					particleSystem.emit(muzzle, muzzle.spawnCount);

					if (m_muzzleParticle == "SniperMuzzle") {
						ParticleEmitterProps smoke = ConfigManager::getParticleProps("SniperSmoke");
						smoke.position = muzzle.position;
						float smokeForce = glm::length(smoke.velocityDir);

						glm::vec2 leftDir(-shootDirection.y, shootDirection.x);
						smoke.velocityDir = leftDir * smokeForce;
						particleSystem.emit(smoke, smoke.spawnCount);

						glm::vec2 rightDir(shootDirection.y, -shootDirection.x);
						smoke.velocityDir = rightDir * smokeForce;
						particleSystem.emit(smoke, smoke.spawnCount);
					}
				}

				Event e;
				e.type = EventType::TowerFired;
				e.textData = m_attackSound;
				EventBus::publish(e);

				m_shotTimer = m_fireRate;
			}
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

		applyStats(nextStats);

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

void Tower::forceLevel(int level) {
	if (level < 1 || level > m_maxLevel) return;
	m_currentLevel = level;
	TowerStats stats = ConfigManager::getTowerStats(m_type, level);// получаем статы для этого типа

	applyStats(stats);
}

void Tower::applyStats(const TowerStats& stats) {
	m_range = stats.range;
	m_damage = stats.damage;
	m_fireRate = stats.fireRate;
	m_rotationSpeed = stats.rotationSpeed;
	m_splashRadius = stats.splashRadius;
	m_attackSound = stats.attackSound;
	m_buildSound = stats.buildSound;
	m_textureId = stats.textureId;
	m_color = stats.color;
	m_muzzleParticle = stats.muzzleParticle;
	m_trailParticle = stats.trailParticle;
	m_impactParticle = stats.impactParticle;
	m_bulletTextureId = stats.bulletTextureId;
	m_bulletBaseSize = stats.bulletBaseSize;
	m_bulletSpeed = stats.bulletSpeed;
}