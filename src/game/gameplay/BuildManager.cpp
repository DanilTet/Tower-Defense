#include "BuildManager.h"
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
    glm::vec2 mousePos, // позиция мыши
    const std::string& selectedType,// выбраный тип башни
    PlayerStats& stats, // деньки игрока
    std::vector<std::unique_ptr<Tower>>& towers,
    std::vector<std::unique_ptr<Enemy>>& enemies,// башни
    Grid& gameGrid, // сетка
    Pathfinder& pathfinder, // поиск пути
    const std::vector<SpawnerData>& spawners, // спавны
    const std::vector<glm::ivec2>& bases,// базы блять
    std::vector<std::vector<glm::ivec2>>& paths,
    std::vector<glm::ivec2>& levelPath) {

    glm::ivec2 clickedCell = gameGrid.pixelToGrid(mousePos);

    // если клик поза поля
    if (clickedCell.x < 0 || clickedCell.x >= gameGrid.getWidth() ||
        clickedCell.y < 0 || clickedCell.y >= gameGrid.getHeight()) {
        return;
    }

    // если в руке ниче нету то просто клик по карте
    if (selectedType.empty()) {
        return;
    }

    int currentCost = ConfigManager::getTowerStats(selectedType).cost;

    if (stats.money >= currentCost && gameGrid.canBuildAt(clickedCell.x, clickedCell.y)) {

        // запоминаем какая клетка была до клика
        CellType oldCellType = gameGrid.getCellType(clickedCell.x, clickedCell.y);

        // виртуально ставим башню
        gameGrid.setCellType(clickedCell.x, clickedCell.y, CellType::Tower);

        // проверяем пути для всех спавнеров
        bool isPathBlocked = false;
        std::vector<std::vector<glm::ivec2>> newPaths; // временное хранилище для новых путей

        for (size_t i = 0; i < spawners.size(); ++i) {
            int baseIdx = spawners[i].targetBaseIndex;
            std::vector<glm::ivec2> bestPath;

            if (baseIdx == -1) {
                int minCost = 999999;
                float minEuclideanDist = 999999.0f;

                for (const auto& base : bases) {
                    int currentCost = 0;
                    auto path = pathfinder.findPath(gameGrid, spawners[i].pos, base, currentCost);

                    if (!path.empty()) {
                        float euclideanDist = glm::distance(glm::vec2(spawners[i].pos), glm::vec2(base));

                        if (currentCost < minCost || (currentCost == minCost && euclideanDist < minEuclideanDist)) {
                            minCost = currentCost;
                            minEuclideanDist = euclideanDist;
                            bestPath = path;
                        }
                    }
                }
            }
            else {
                if (baseIdx < 0 || baseIdx >= bases.size()) baseIdx = 0;
                int dummyCost = 0;
                bestPath = pathfinder.findPath(gameGrid, spawners[i].pos, bases[baseIdx], dummyCost);
            }

            if (bestPath.empty()) {
                isPathBlocked = true; // если хоть один спавнер заблокирован то строить нельзя
                break;
            }

            bestPath.insert(bestPath.begin(), spawners[i].pos); // добавляем точку старта
            newPaths.push_back(bestPath); // сохраняем успешный путь
        }

        // если пути нету значит игрок заблокировал маршрут
        if (isPathBlocked) {
            gameGrid.setCellType(clickedCell.x, clickedCell.y, oldCellType); // Откатываем сетку
            std::cout << "Path Blocked! Cannot build here." << std::endl;
        }
        else { // иначе пути свободны
            stats.money -= currentCost;

            // спавним башню
            auto newTower = std::make_unique<Tower>(clickedCell.x, clickedCell.y, selectedType);
            towers.push_back(std::move(newTower));

            // звук постройки
            TowerStats towerstats = Tower::getStatsfromTowerType(selectedType);
            Event e;
            e.type = EventType::TowerBuilt;
            e.textData = towerstats.buildSound;
            EventBus::publish(e);

            // обновляем все маршруты
            paths = newPaths;
            levelPath = paths[0];

            // даем врагам новый путь
            for (auto& enemy : enemies) {
                if (enemy) {
                    enemy->recalculatePath(&pathfinder, gameGrid, bases);
                }
            }
        }
    }
}

void BuildManager::sellTower(
    Tower* tower,
    PlayerStats& stats,
    EntityManager& entityManager,
    Grid& gameGrid,
    Pathfinder& pathfinder,
    const std::vector<SpawnerData>& spawners,
    const std::vector<glm::ivec2>& bases,
    std::vector<std::vector<glm::ivec2>>& paths,
    std::vector<glm::ivec2>& levelPath) {

    if (!tower) return;

    int tx = tower->getGridX();
    int ty = tower->getGridY();

    stats.money += 50;

    // освобождаем клетку на сетке
    gameGrid.setCellType(tx, ty, CellType::Ground);

    // уничтожаем объект башни
    entityManager.removeTower(tx, ty);

    // считаем заново пути с учетом геометрической близости
    std::vector<std::vector<glm::ivec2>> newPaths;
    for (size_t i = 0; i < spawners.size(); ++i) {
        int baseIdx = spawners[i].targetBaseIndex;
        std::vector<glm::ivec2> testPath;

        if (baseIdx == -1) {
            int minCost = 999999;
            float minEuclideanDist = 999999.0f;

            for (const auto& base : bases) {
                int currentCost = 0;
                auto path = pathfinder.findPath(gameGrid, spawners[i].pos, base, currentCost);

                if (!path.empty()) {
                    float euclideanDist = glm::distance(glm::vec2(spawners[i].pos), glm::vec2(base));

                    if (currentCost < minCost || (currentCost == minCost && euclideanDist < minEuclideanDist)) {
                        minCost = currentCost;
                        minEuclideanDist = euclideanDist;
                        testPath = path;;
                    }
                }
            }
        }
        else {
            if (baseIdx < 0 || baseIdx >= bases.size()) baseIdx = 0;
            int dummyCost = 0;
            testPath = pathfinder.findPath(gameGrid, spawners[i].pos, bases[baseIdx], dummyCost);
        }

        testPath.insert(testPath.begin(), spawners[i].pos);
        newPaths.push_back(testPath);
    }

    // обновляем пути
    paths = newPaths;
    levelPath = paths[0];

    // перенаправляем врагов по новому маршруту
    for (auto& enemy : entityManager.getEnemies()) {
        if (enemy) {
            enemy->recalculatePath(&pathfinder, gameGrid, bases);
        }
    }
}