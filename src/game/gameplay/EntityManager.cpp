#include "EntityManager.h"
#include "world/Grid.h"
#include "renderer/SpriteRenderer.h"
#include "textures/Texture2D.h"
#include <algorithm>
#include "gameplay/PlayerStats.h"

void EntityManager::update(float dt, Grid& gameGrid, PlayerStats& stats) {
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

    // если враг убит добавляем игроку деняк иначе отминаем от базі хп
    for (const auto& enemy : m_enemies) {
        if (enemy->isDead()) {
            stats.money += enemy->getReward();
            stats.score += 10;
        }
        if (enemy->isReachedEnd()) {
            stats.baseHealth -= 1;
        }
    }


    // обновляем пули
    for (const auto& proj : m_projectiles) {
        if (proj) {
            proj->update(dt, m_enemies, gameGrid);
        }
    }

    // удаляем уничтоженіе пули
    m_projectiles.erase(
        std::remove_if(m_projectiles.begin(), m_projectiles.end(),
            [](const std::unique_ptr<Projectile>& proj) { return proj->isDestroyed(); }),
        m_projectiles.end()
    );

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
    for (const auto& proj : m_projectiles) {
        if (proj) {
            proj->render(renderer, cellTex);
        }
    }
}