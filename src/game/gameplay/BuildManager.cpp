#include "BuildManager.h"
#include "audio/AudioManager.h"
#include <iostream>
#include "entities/Tower.h"
#include "entities/Enemy.h"
#include "world/Grid.h"
#include "world/Pathfinder.h"
#include "gameplay/PlayerStats.h"
#include "gameplay/EntityManager.h"

void BuildManager::tryBuildOrUpgrade(
    glm::vec2 mousePos, // позиция мыши
    TowerType selectedType,// выбраный тип башни
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
    if (selectedType == TowerType::None) {
        return;
    }


    int currentCost = Tower::getStatsfromTowerType(selectedType).cost;

    if (stats.money >= currentCost && gameGrid.canBuildAt(clickedCell.x, clickedCell.y)) {

        // запоминаем какая клетка была до клика
        CellType oldCellType = gameGrid.getCellType(clickedCell.x, clickedCell.y);

        // виртуально ставим башню
        gameGrid.setCellType(clickedCell.x, clickedCell.y, CellType::Tower);
        //проверка маршрута
        
        // проверяем пути для всех спавнеров
        bool isPathBlocked = false;
        std::vector<std::vector<glm::ivec2>> newPaths; // временное хранилице для новых путей

        for (size_t i = 0; i < spawners.size(); ++i) {
            int baseIdx = spawners[i].targetBaseIndex;
            if (baseIdx < 0 || baseIdx >= bases.size()) baseIdx = 0;

            std::vector<glm::ivec2> testPath = pathfinder.findPath(gameGrid, spawners[i].pos, bases[baseIdx]);

            if (testPath.empty()) {
                isPathBlocked = true; // если хоть один спавнер заблокирован то строить нельзя
                break;
            }

            testPath.insert(testPath.begin(), spawners[i].pos); // добавляем точку старта
            newPaths.push_back(testPath); // сохраняем успешный путь
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
            AudioManager::playSound(towerstats.buildSound.c_str());

            // обновляем все маршруты
            paths = newPaths; // заменяем старые пути на новые
            levelPath = paths[0]; // оставляем нулевой для обратной совместимости

            // убираем старый путь с сетки
            for (int x = 0; x < gameGrid.getWidth(); ++x) {
                for (int y = 0; y < gameGrid.getHeight(); ++y) {
                    if (gameGrid.getCellType(x, y) == CellType::Path) {
                        gameGrid.setCellType(x, y, CellType::Empty);
                    }
                }
            }
            // записуем новые пути на сетку
            for (const auto& path : paths) {
                for (const auto& p : path) {
                    // Ставим CellType::Path, только если клетка пустая чтобы не затереть спавнер или базу
                    if (gameGrid.getCellType(p.x, p.y) == CellType::Empty) {
                        gameGrid.setCellType(p.x, p.y, CellType::Path);
                    }
                }
            }

            // даем врагам новый путь
            for (auto& enemy : enemies) {
                if (enemy) {
                    // враг пересчитывает путь именно к СВОЕЙ базе которую он запомнил
                    enemy->recalculatePath(&pathfinder, gameGrid, enemy->getTargetBase());
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

    // заглушка даем 50 деньков позже сделаем расчет возврата в зависимости от левела
    stats.money += 50;

    // освобождаем клетку на сетке
    gameGrid.setCellType(tx, ty, CellType::Empty);

    // уничтожаем объект башни
    entityManager.removeTower(tx, ty);

    // считаем заново путь но тут 1000% будет путь так как же освободили путь клянусь!!!
    std::vector<std::vector<glm::ivec2>> newPaths;
    for (size_t i = 0; i < spawners.size(); ++i) {
        int baseIdx = spawners[i].targetBaseIndex;
        if (baseIdx < 0 || baseIdx >= bases.size()) baseIdx = 0;

        std::vector<glm::ivec2> testPath = pathfinder.findPath(gameGrid, spawners[i].pos, bases[baseIdx]);
        testPath.insert(testPath.begin(), spawners[i].pos);
        newPaths.push_back(testPath);
    }

    // обновляем пути
    paths = newPaths;
    levelPath = paths[0];

    // очищаем старые пути и рисуем новые на сетке
    for (int x = 0; x < gameGrid.getWidth(); ++x) {
        for (int y = 0; y < gameGrid.getHeight(); ++y) {
            if (gameGrid.getCellType(x, y) == CellType::Path) {
                gameGrid.setCellType(x, y, CellType::Empty);
            }
        }
    }
    for (const auto& path : paths) {
        for (const auto& p : path) {
            if (gameGrid.getCellType(p.x, p.y) == CellType::Empty) {
                gameGrid.setCellType(p.x, p.y, CellType::Path);
            }
        }
    }

    // перенаправляем врагов по новому маршруту
    for (auto& enemy : entityManager.getEnemies()) {
        if (enemy) {
            enemy->recalculatePath(&pathfinder, gameGrid, enemy->getTargetBase());
        }
    }
}