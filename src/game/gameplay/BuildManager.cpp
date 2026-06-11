#include "BuildManager.h"
#include "audio/AudioManager.h"
#include <iostream>
#include "entities/Tower.h"
#include "entities/Enemy.h"
#include "world/Grid.h"
#include "world/Pathfinder.h"
#include "gameplay/PlayerStats.h"

void BuildManager::tryBuildOrUpgrade(
    glm::vec2 mousePos, // позиция мыши
    TowerType selectedType,// выбраный тип башни
    PlayerStats& stats, // деньки игрока
    std::vector<std::unique_ptr<Tower>>& towers,
    std::vector<std::unique_ptr<Enemy>>& enemies,// башни
    Grid& gameGrid, // сетка
    Pathfinder& pathfinder, // поиск пути
    const std::vector<glm::ivec2>& spawners, // спавны
    const std::vector<glm::ivec2>& bases,// базы блять
    std::vector<std::vector<glm::ivec2>>& paths,
    std::vector<glm::ivec2>& levelPath) { 


    glm::ivec2 clickedCell = gameGrid.pixelToGrid(mousePos);

    // если клик поза поля
    if (clickedCell.x < 0 || clickedCell.x >= gameGrid.getWidth() ||
        clickedCell.y < 0 || clickedCell.y >= gameGrid.getHeight()) {
        return;
    }

    // логика апгрейда
    if (selectedType == TowerType::None) {
        CellType clickedCellType = gameGrid.getCellType(clickedCell.x, clickedCell.y);

        if (clickedCellType == CellType::Tower) {
            // ищем в нашем векторе башню с такими же координатами YX
            for (auto& tower : towers) {
                if (tower && tower->getGridX() == clickedCell.x && tower->getGridY() == clickedCell.y) {
                    int cost = tower->getUpgradeCost();
                    if (tower->upgrade(stats.money)) {
                        std::cout << "Tower upgraded to level " << tower->getLevel() << "!" << std::endl;
                    }
                    else {
                        std::cout << "Upgrade failed! Max level or check money ($" << cost << ")" << std::endl;
                    }
                    break; // выходим
                }
            }
        }
        return; // ОБЯЗАТЕЛЬНО выходим из метода
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
            std::vector<glm::ivec2> testPath = pathfinder.findPath(gameGrid, spawners[i], bases[0]);

            if (testPath.empty()) {
                isPathBlocked = true; // если хоть один спавнер заблокирован то строить нельзя
                break;
            }

            testPath.insert(testPath.begin(), spawners[i]); // добавляем точку старта
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
                    enemy->recalculatePath(&pathfinder, gameGrid, bases[0]);
                }
            }
        }
    }
}