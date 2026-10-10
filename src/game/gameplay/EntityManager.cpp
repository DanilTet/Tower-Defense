#include "EntityManager.h"
#include "world/Grid.h"
#include "renderer/SpriteRenderer.h"
#include "textures/Texture2D.h"
#include <algorithm>
#include <cstdlib>
#include "../core/EventBus.h"
#include "../../particles/ParticleSystem.h"
#include "../core/ConfigManager.h"

// сразу выделяем память под 500 пуль
EntityManager::EntityManager() {
    m_projectiles.resize(500);
    m_particleSystem = std::make_unique<ParticleSystem>();
}

void EntityManager::update(float dt, Grid& gameGrid) {
    // пробегаемся по всему вектору активных башен на карте
    for (const auto& tower : m_towers) {
        if (tower) {
            tower->update(dt, m_enemies, *this, gameGrid, *m_particleSystem);
        }
    }
    // Пробегаемся по всему вектору активных врагов на карте
    for (const auto& enemy : m_enemies) {
        if (enemy) { // Если указатель на врага живой
            enemy->update(dt, gameGrid); // враг пересчитывает свою позицию с учетом deltaTime
        }
    }

    // обновляем пули из пула
    for (auto& proj : m_projectiles) {
        if (proj.isActive()) {
            proj.update(dt, m_enemies, gameGrid, *m_particleSystem);
            // если пуля долетела или время ВСЁ то выключаем её
            if (proj.isDestroyed()) {
                proj.setActive(false);
            }
        }
    }

    std::vector<std::unique_ptr<Enemy>> spawnedChildren;

    // если враг убит добавляем игроку деняк иначе отминаем от базі хп
    for (const auto& enemy : m_enemies) {
        if (enemy->isDead()) {
            EventBus::publish({ EventType::EnemyDied, enemy->getReward(), 10, enemy->getCollider(gameGrid).center.x, enemy->getCollider(gameGrid).center.y, enemy->getDeathSound() });

            std::string deathParticle = enemy->getDeathParticle();
            if (deathParticle.empty()) deathParticle = "BloodSplatter";
            ParticleEmitterProps deathProps = ConfigManager::getParticleProps(deathParticle);
            deathProps.position = enemy->getCollider(gameGrid).center;
            m_particleSystem->emit(deathProps, deathProps.spawnCount);

            // Механика распада (разделения) при гибели врага
            if (!enemy->isFalling() && enemy->getSplitCount() > 0 && !enemy->getSplitChildType().empty()) {
                float scatterRadius = enemy->getSplitScatterRadius();
                for (int i = 0; i < enemy->getSplitCount(); ++i) {
                    float randX = (((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f) * scatterRadius;
                    float randY = (((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f) * scatterRadius;
                    glm::vec2 childPos = enemy->getPixelPos() + glm::vec2(randX, randY);

                    auto child = std::make_unique<Enemy>(
                        enemy->getPath(),
                        gameGrid,
                        enemy->getSplitChildType(),
                        enemy->getTargetBaseIndex(),
                        enemy->getCurrentWaypoint(),
                        childPos,
                        enemy->getEnableFadeIn(),
                        enemy->getEnableFadeOut()
                    );
                    child->setDistanceTraveled(enemy->getDistanceTraveled());
                    spawnedChildren.push_back(std::move(child));
                }
            }
        }
        if (enemy->isReachedEnd()) {
            EventBus::publish({ EventType::EnemyReachedBase, 1 });
        }
    }

    m_particleSystem->update(dt);

    // Удаляем врагов, которые достигли конца пути или умерли
    m_enemies.erase(
        std::remove_if(m_enemies.begin(), m_enemies.end(),
            [](const std::unique_ptr<Enemy>& enemy) { return enemy->isReachedEnd() || enemy->isDead(); }),
        m_enemies.end()
    );

    // Добавляем созданных дочерних врагов в общий пул
    for (auto& child : spawnedChildren) {
        m_enemies.push_back(std::move(child));
    }
}

void EntityManager::render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> mainAtlas, std::shared_ptr<Texture2D> enemyAtlas, std::shared_ptr<Texture2D> radiusTex, std::shared_ptr<Texture2D> arrowTex, std::shared_ptr<Texture2D> particleTex, Grid& gameGrid, Tower* selectedTower) {
    for (const auto& tower : m_towers) {
        if (tower) {
            bool isSelected = (tower.get() == selectedTower);
            tower->render(renderer, mainAtlas, radiusTex, arrowTex, gameGrid, isSelected);
        }
    }
    for (const auto& enemy : m_enemies) {
        if (enemy) {
            enemy->render(renderer, enemyAtlas, radiusTex, gameGrid.getOffset(), gameGrid);
        }
    }
    for (auto& proj : m_projectiles) {
        if (proj.isActive()) {
            proj.render(renderer, mainAtlas, gameGrid);
        }
    }

    m_particleSystem->render(renderer, particleTex, gameGrid);
}

Tower* EntityManager::getTowerAt(int gridX, int gridY) {
    // бам бам бим бим по башням и ищем по кордам ее
    for (const auto& tower : m_towers) {
        if (tower && tower->getGridX() == gridX && tower->getGridY() == gridY) {
            return tower.get();
        }
    }
    return nullptr;
}

void EntityManager::removeTower(int gridX, int gridY) {
    m_towers.erase(
        std::remove_if(m_towers.begin(), m_towers.end(),
            [gridX, gridY](const std::unique_ptr<Tower>& tower) {
                return tower->getGridX() == gridX && tower->getGridY() == gridY;
            }),
        m_towers.end()
    );// короче круто просто убираем через КРУТУЮ лямбда функцию егор тетеря курсач
}

Projectile* EntityManager::getFreeProjectile() {
    for (auto& proj : m_projectiles) {
        if (!proj.isActive()) {
            return &proj;
        }
    }
    // если 500 пуль не хватило то расширяем массив
    m_projectiles.emplace_back();
    return &m_projectiles.back();
}