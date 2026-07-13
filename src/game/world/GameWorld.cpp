#include "GameWorld.h"
#include "../entities/Enemy.h"
#include <iostream>

GameWorld::GameWorld() {
}

bool GameWorld::loadLevel(const std::string& levelPath, int windowWidth, int windowHeight) {
    LevelMapData levelData = LevelManager::loadLevelMap(levelPath);
    currentLevelPath = levelPath;

    grid = std::make_unique<Grid>(
        levelData.gridWidth,
        levelData.gridHeight,
        levelData.cellSize,
        glm::vec2(levelData.offsetX, levelData.offsetY)
    );
    grid->updateCellSize(windowWidth, windowHeight);

    if (!levelData.layout.empty()) {
        for (int y = 0; y < levelData.gridHeight; ++y) {
            for (int x = 0; x < levelData.gridWidth; ++x) {
                if (levelData.layout[y][x] == 1) {
                    grid->setCellType(x, y, CellType::Path);
                }
                else if (levelData.layout[y][x] == 2) {
                    grid->setCellType(x, y, CellType::Platform);
                }
                else if (levelData.layout[y][x] == 3) {
                    grid->setCellType(x, y, CellType::Scenery);
                }
            }
        }
    }

    spawners = levelData.spawners;
    bases = levelData.bases;

    for (const auto& spawner : spawners) {
        grid->setCellType(spawner.pos.x, spawner.pos.y, CellType::Spawner);
    }

    for (const auto& base : bases) {
        grid->setCellType(base.x, base.y, CellType::Base);
    }

    grid->saveOriginalGrid();

    pathfinder = std::make_unique<Pathfinder>(levelData.gridWidth, levelData.gridHeight);
    waveManager = std::make_unique<WaveManager>();
    waveManager->loadLevel(currentLevelPath);
    entityManager = std::make_unique<EntityManager>();

    recalculateAllPaths();
    return true;
}

void GameWorld::recalculateAllPaths() {
    paths.clear();
    for (size_t i = 0; i < spawners.size(); ++i) {
        int baseIdx = spawners[i].targetBaseIndex;
        std::vector<glm::ivec2> calculatedPath;

        if (baseIdx == -1) {
            int minCost = 999999;
            float minEuclideanDist = 999999.0f;

            for (const auto& base : bases) {
                int currentCost = 0;
                auto path = pathfinder->findPath(*grid, spawners[i].pos, base, currentCost);

                if (!path.empty()) {
                    float euclideanDist = glm::distance(glm::vec2(spawners[i].pos), glm::vec2(base));

                    if (currentCost < minCost || (currentCost == minCost && euclideanDist < minEuclideanDist)) {
                        minCost = currentCost;
                        minEuclideanDist = euclideanDist;
                        calculatedPath = path;
                    }
                }
            }
        }
        else {
            if (baseIdx < 0 || baseIdx >= bases.size()) baseIdx = 0;
            int dummyCost = 0;
            calculatedPath = pathfinder->findPath(*grid, spawners[i].pos, bases[baseIdx], dummyCost);
        }

        if (!calculatedPath.empty()) {
            calculatedPath.insert(calculatedPath.begin(), spawners[i].pos);
            paths.push_back(calculatedPath);
        }
    }

    if (!paths.empty()) {
        levelPath = paths[0];
    }
}

void GameWorld::update(float dt) {
    if (waveManager) {
        auto requests = waveManager->update(dt, paths.size());
        for (const auto& req : requests) {
            spawnEnemy(req.type, req.spawnerIndex);
        }
    }
    entityManager->update(dt, *grid);
}

void GameWorld::resize(int windowWidth, int windowHeight) {
    if (grid) {
        Grid oldGrid = *grid;
        grid->updateCellSize(windowWidth, windowHeight);

        for (const auto& enemy : entityManager->getEnemies()) {
            if (enemy) {
                enemy->recalculatePosition(oldGrid, *grid);
            }
        }

        for (auto& proj : entityManager->getProjectilePool()) {
            proj.recalculatePosition(oldGrid, *grid);
        }
    }
}

void GameWorld::spawnEnemy(const std::string& type, int spawnerIndex) {
    if (spawnerIndex >= paths.size() || paths[spawnerIndex].empty()) return;

    int targetIdx = spawners[spawnerIndex].targetBaseIndex;
    auto newEnemy = std::make_unique<Enemy>(paths[spawnerIndex], *grid, type, targetIdx);
    entityManager->addEnemy(std::move(newEnemy));
}
