#include "EntityManager.h"
#include "world/Grid.h"
#include "renderer/SpriteRenderer.h"
#include "textures/Texture2D.h"
#include <algorithm>
#include "../core/EventBus.h"
#include "../../particles/ParticleSystem.h"

// сразу выделяем память под 500 пуль
EntityManager::EntityManager() {
    m_projectiles.resize(500);
    m_particleSystem = std::make_unique<ParticleSystem>();
}

void EntityManager::update(float dt, Grid& gameGrid) {
    // пробегаемся по всему вектору активных башен на карте
    for (const auto& tower : m_towers) {
        if (tower) {
            tower->update(dt, m_enemies, m_projectiles, gameGrid);
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
            proj.update(dt, m_enemies, gameGrid);

            // если пуля долетела или время ВСЁ то выключаем её
            if (proj.isDestroyed()) {
                proj.setActive(false);
            }
        }
    }

    // если враг убит добавляем игроку деняк иначе отминаем от базі хп
    for (const auto& enemy : m_enemies) {
        if (enemy->isDead()) {
            EventBus::publish({ EventType::EnemyDied, enemy->getReward(), 10, enemy->getCollider(gameGrid).center.x, enemy->getCollider(gameGrid).center.y, enemy->getDeathSound() });

            // ГОВНОКОД ДЛЯ ТЕСТА ПАРТИКЛОВ ОТ ГЕМИНЬКИ ПОТОМ УБРАТЬ
            // --- СПАВНИМ КРОВИЩУ! ---
            ParticleEmitterProps bloodProps;
            bloodProps.position = enemy->getCollider(gameGrid).center; // Прямо в центре врага
            bloodProps.velocityDir = glm::vec2(0.0f); // Разлет во все стороны
            bloodProps.velocityVariation = 150.0f;    // Резкий взрывной выброс
            bloodProps.startColor = glm::vec3(0.8f, 0.1f, 0.1f); // Свежая яркая кровь
            bloodProps.endColor = glm::vec3(0.3f, 0.0f, 0.0f);   // Темнеет со временем
            bloodProps.startSize = 12.0f; // Крупные ошметки
            bloodProps.endSize = 2.0f;    // Растворяются в пыль
            bloodProps.lifeTime = 0.4f;   // Быстро исчезают

            // Запускаем 30 частиц за один раз
            m_particleSystem->emit(bloodProps, 30);
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
}

void EntityManager::render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> cellTex, std::shared_ptr<Texture2D> radiusTex, std::shared_ptr<Texture2D> arrowTex, Grid& gameGrid) {
    // проходим по всем врагам, башням, пулям и рисуем их
    for (const auto& enemy : m_enemies) {
        if (enemy) { // Если враг существует
            enemy->render(renderer, cellTex, radiusTex, gameGrid.getOffset(), gameGrid); // Вызываем его метод отрисовки
        }
    }
    for (const auto& tower : m_towers) {
        if (tower) {
            tower->render(renderer, cellTex, radiusTex, arrowTex, gameGrid);
        }
    }
    for (auto& proj : m_projectiles) {
        if (proj.isActive()) {
            proj.render(renderer, cellTex);
        }
    }

    m_particleSystem->render(renderer, cellTex);
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