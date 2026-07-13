#include "BuildManager.h"
#include "world/GameWorld.h"
#include "audio/AudioManager.h"
#include <iostream>
#include "entities/Tower.h"
#include "entities/Enemy.h"
#include "world/Grid.h"
#include "world/Pathfinder.h"
#include "gameplay/PlayerStats.h"
#include "gameplay/EntityManager.h"
#include "../core/ConfigManager.h"
#include "../core/EventBus.h"

void BuildManager::tryBuildOrUpgrade(
    glm::vec2 mousePos,
    const std::string& selectedType,
    GameWorld& world) {

    glm::ivec2 clickedCell = world.grid->pixelToGrid(mousePos);

    // если клик поза поля
    if (clickedCell.x < 0 || clickedCell.x >= world.grid->getWidth() ||
        clickedCell.y < 0 || clickedCell.y >= world.grid->getHeight()) {
        return;
    }

    // если в руке ниче нету то просто клик по карте
    if (selectedType.empty()) {
        return;
    }

    int currentCost = ConfigManager::getTowerStats(selectedType).cost;

    if (world.playerStats.money >= currentCost && world.grid->canBuildAt(clickedCell.x, clickedCell.y)) {

        // запоминаем какая клетка была до клика
        CellType oldCellType = world.grid->getCellType(clickedCell.x, clickedCell.y);

        // виртуально ставим башню
        world.grid->setCellType(clickedCell.x, clickedCell.y, CellType::Tower);

        // проверяем пути для всех спавнеров
        bool isPathBlocked = false;
        std::vector<std::vector<glm::ivec2>> newPaths; // временное хранилище для новых путей

        for (size_t i = 0; i < world.spawners.size(); ++i) {
            int baseIdx = world.spawners[i].targetBaseIndex;
            std::vector<glm::ivec2> bestPath;

            if (baseIdx == -1) {
                int minCost = 999999;
                float minEuclideanDist = 999999.0f;

                for (const auto& base : world.bases) {
                    int currentCost = 0;
                    auto path = world.pathfinder->findPath(*world.grid, world.spawners[i].pos, base, currentCost);

                    if (!path.empty()) {
                        float euclideanDist = glm::distance(glm::vec2(world.spawners[i].pos), glm::vec2(base));

                        if (currentCost < minCost || (currentCost == minCost && euclideanDist < minEuclideanDist)) {
                            minCost = currentCost;
                            minEuclideanDist = euclideanDist;
                            bestPath = path;
                        }
                    }
                }
            }
            else {
                if (baseIdx < 0 || baseIdx >= world.bases.size()) baseIdx = 0;
                int dummyCost = 0;
                bestPath = world.pathfinder->findPath(*world.grid, world.spawners[i].pos, world.bases[baseIdx], dummyCost);
            }

            if (bestPath.empty()) {
                isPathBlocked = true; // если хоть один спавнер заблокирован то строить нельзя
                break;
            }

            bestPath.insert(bestPath.begin(), world.spawners[i].pos); // добавляем точку старта
            newPaths.push_back(bestPath); // сохраняем успешный путь
        }

        // если пути нету значит игрок заблокировал маршрут
        if (isPathBlocked) {
            world.grid->setCellType(clickedCell.x, clickedCell.y, oldCellType); // Откатываем сетку
            std::cout << "Path Blocked! Cannot build here." << std::endl;
        }
        else { // иначе пути свободны
            world.playerStats.money -= currentCost;

            // спавнить башню через entityManager
            auto newTower = std::make_unique<Tower>(clickedCell.x, clickedCell.y, selectedType);
            world.entityManager->addTower(std::move(newTower));

            // звук постройки
            TowerStats towerstats = Tower::getStatsfromTowerType(selectedType);
            Event e;
            e.type = EventType::TowerBuilt;
            e.textData = towerstats.buildSound;
            EventBus::publish(e);

            // обновляем все маршруты
            world.paths = newPaths;
            world.levelPath = world.paths[0];

            // даем врагам новый путь
            for (auto& enemy : world.entityManager->getEnemies()) {
                if (enemy) {
                    enemy->recalculatePath(world.pathfinder.get(), *world.grid, world.bases);
                }
            }
        }
    }
}

void BuildManager::sellTower(
    Tower* tower,
    GameWorld& world) {

    if (!tower) return;

    int tx = tower->getGridX();
    int ty = tower->getGridY();

    world.playerStats.money += 50;

    // освобождаем клетку на сетке
    world.grid->setCellType(tx, ty, CellType::Ground);

    // уничтожаем объект башни
    world.entityManager->removeTower(tx, ty);

    // считаем заново пути с учетом геометрической близости
    std::vector<std::vector<glm::ivec2>> newPaths;
    for (size_t i = 0; i < world.spawners.size(); ++i) {
        int baseIdx = world.spawners[i].targetBaseIndex;
        std::vector<glm::ivec2> testPath;

        if (baseIdx == -1) {
            int minCost = 999999;
            float minEuclideanDist = 999999.0f;

            for (const auto& base : world.bases) {
                int currentCost = 0;
                auto path = world.pathfinder->findPath(*world.grid, world.spawners[i].pos, base, currentCost);

                if (!path.empty()) {
                    float euclideanDist = glm::distance(glm::vec2(world.spawners[i].pos), glm::vec2(base));

                    if (currentCost < minCost || (currentCost == minCost && euclideanDist < minEuclideanDist)) {
                        minCost = currentCost;
                        minEuclideanDist = euclideanDist;
                        testPath = path;
                    }
                }
            }
        }
        else {
            if (baseIdx < 0 || baseIdx >= world.bases.size()) baseIdx = 0;
            int dummyCost = 0;
            testPath = world.pathfinder->findPath(*world.grid, world.spawners[i].pos, world.bases[baseIdx], dummyCost);
        }

        testPath.insert(testPath.begin(), world.spawners[i].pos);
        newPaths.push_back(testPath);
    }

    // обновляем пути
    world.paths = newPaths;
    world.levelPath = world.paths[0];

    // перенаправляем врагов по новому маршруту
    for (auto& enemy : world.entityManager->getEnemies()) {
        if (enemy) {
            enemy->recalculatePath(world.pathfinder.get(), *world.grid, world.bases);
        }
    }
}