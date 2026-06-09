#include "BuildManager.h"
#include "audio/AudioManager.h"
#include <iostream>
#include "entities/Tower.h"
#include "entities/Enemy.h"
#include "world/Grid.h"
#include "world/Pathfinder.h"

void BuildManager::tryBuildOrUpgrade(
    glm::vec2 mousePos, // позиция мыши
    TowerType selectedType,// выбраный тип башни
    int& playerMoney, // деньки игрока
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
                    if (tower->upgrade(playerMoney)) {
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

    if (playerMoney >= currentCost && gameGrid.canBuildAt(clickedCell.x, clickedCell.y)) {

        // запоминаем какая клетка была до клика
        CellType oldCellType = gameGrid.getCellType(clickedCell.x, clickedCell.y);

        // виртуально ставим башню
        gameGrid.setCellType(clickedCell.x, clickedCell.y, CellType::Tower);
        //проверка маршрута
        std::vector<glm::ivec2> testPath = pathfinder.findPath(gameGrid, spawners[0], bases[0]);

        // если пути нету значит игрок заблокировал маршрут
        if (testPath.empty()) {
            gameGrid.setCellType(clickedCell.x, clickedCell.y, oldCellType);
            std::cout << "Path Blocked! Cannot build here." << std::endl;
        }
        else {
            // если путь есть
            // забираем деньги
            playerMoney -= currentCost;
            // спавним башню
            auto newTower = std::make_unique<Tower>(clickedCell.x, clickedCell.y, selectedType);
            towers.push_back(std::move(newTower));
            // звук постройки
            TowerStats stats = Tower::getStatsfromTowerType(selectedType);
            AudioManager::playSound(stats.buildSound.c_str());

            // обновляем путь
            testPath.insert(testPath.begin(), spawners[0]);
            paths[0] = testPath;
            levelPath = testPath;

            // убираем старій путь
            for (int x = 0; x < gameGrid.getWidth(); ++x) {
                for (int y = 0; y < gameGrid.getHeight(); ++y) {
                    if (gameGrid.getCellType(x, y) == CellType::Path) {
                        gameGrid.setCellType(x, y, CellType::Empty);
                    }
                }
            }
            // записуем новій путь
            for (const auto& p : paths[0]) {
                if (gameGrid.getCellType(p.x, p.y) == CellType::Empty) {
                    gameGrid.setCellType(p.x, p.y, CellType::Path);
                }
            }

            // даем врагам новій путь
            for (auto& enemy : enemies) {
                if (enemy) {
                    enemy->recalculatePath(&pathfinder, gameGrid, bases[0]);
                }
            }
        }
    }
}