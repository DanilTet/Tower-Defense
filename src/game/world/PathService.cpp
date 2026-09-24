#include "PathService.h"
#include "Grid.h"
#include "Pathfinder.h"
#include "../entities/Enemy.h"
#include <unordered_map>
#include <cmath>
#include <glm/geometric.hpp>

// Проверяет, пересекает ли маршрут клетку с учетом диагональных срезов углов
static bool doesPathIntersectCell(const std::vector<glm::ivec2>& path, int gridX, int gridY) {
    if (path.empty()) return false;
    for (size_t i = 0; i < path.size(); ++i) {
        if (path[i].x == gridX && path[i].y == gridY) {
            return true;
        }
        if (i + 1 < path.size()) {
            int dx = path[i + 1].x - path[i].x;
            int dy = path[i + 1].y - path[i].y;
            if (std::abs(dx) == 1 && std::abs(dy) == 1) {
                // Если новая башня ставится на любой из двух углов диагонального шага
                if ((gridX == path[i].x + dx && gridY == path[i].y) ||
                    (gridX == path[i].x && gridY == path[i].y + dy)) {
                    return true;
                }
            }
        }
    }
    return false;
}

std::vector<std::vector<glm::ivec2>> PathService::calculateAllPaths(
    const Grid& grid,
    Pathfinder& pathfinder,
    const std::vector<SpawnerData>& spawners,
    const std::vector<glm::ivec2>& bases)
{
    std::vector<std::vector<glm::ivec2>> paths;
    if (spawners.empty() || bases.empty()) return paths;

    std::unordered_map<int, DijkstraMap> dmapCache;

    for (size_t i = 0; i < spawners.size(); ++i) {
        int baseIdx = spawners[i].targetBaseIndex;
        if (baseIdx < -1 || baseIdx >= static_cast<int>(bases.size())) {
            baseIdx = -1;
        }

        if (dmapCache.find(baseIdx) == dmapCache.end()) {
            if (baseIdx == -1) {
                dmapCache[baseIdx] = pathfinder.generateDijkstraMap(grid, bases);
            } else {
                dmapCache[baseIdx] = pathfinder.generateDijkstraMap(grid, {bases[baseIdx]});
            }
        }

        const auto& dmap = dmapCache[baseIdx];
        std::vector<glm::ivec2> calculatedPath = pathfinder.tracePath(dmap, grid, spawners[i].pos);

        if (!calculatedPath.empty() || (dmap.isReachable(spawners[i].pos.x, spawners[i].pos.y) && dmap.getDistance(spawners[i].pos.x, spawners[i].pos.y) == 0)) {
            calculatedPath.insert(calculatedPath.begin(), spawners[i].pos);
            paths.push_back(calculatedPath);
        } else {
            paths.push_back({});
        }
    }

    return paths;
}

bool PathService::isPlacementValid(
    Grid& grid,
    Pathfinder& pathfinder,
    const std::vector<SpawnerData>& spawners,
    const std::vector<glm::ivec2>& bases,
    int gridX,
    int gridY,
    std::vector<std::vector<glm::ivec2>>& outNewPaths,
    const std::vector<std::vector<glm::ivec2>>* currentPaths,
    const std::vector<std::unique_ptr<Enemy>>* activeEnemies)
{
    // --- 0. ПРОВЕРКА: Не стоит ли на клетке живой враг? ---
    if (activeEnemies) {
        for (const auto& enemy : *activeEnemies) {
            if (!enemy || enemy->isDead() || enemy->isReachedEnd()) continue;
            glm::ivec2 ePos = grid.pixelToGrid(enemy->getPixelPos());
            if (ePos.x == gridX && ePos.y == gridY) {
                return false; // Запрещено строить прямо поверх моба
            }
        }
    }

    // --- 1. DIRTY CHECK: Быстрый тест на пересечение ---
    if (currentPaths && currentPaths->size() == spawners.size()) {
        bool intersectsAny = false;
        for (const auto& path : *currentPaths) {
            if (doesPathIntersectCell(path, gridX, gridY)) {
                intersectsAny = true;
                break;
            }
        }

        if (!intersectsAny && activeEnemies) {
            for (const auto& enemy : *activeEnemies) {
                if (!enemy || enemy->isDead() || enemy->isReachedEnd()) continue;
                if (enemy->isPathIntersecting(glm::ivec2(gridX, gridY))) {
                    intersectsAny = true;
                    break;
                }
            }
        }

        if (!intersectsAny) {
            grid.setCellType(gridX, gridY, CellType::Tower);
            outNewPaths = *currentPaths;
            return true; // 0 ms instant exit!
        }
    }

    // --- 2. ПРОВЕРКА С ПОСТАНОВКОЙ БАШНИ ЧЕРЕЗ БЫСТРЫЙ FLOW FIELD ---
    CellType oldCellType = grid.getCellType(gridX, gridY);
    grid.setCellType(gridX, gridY, CellType::Tower);

    outNewPaths.clear();
    outNewPaths.resize(spawners.size());
    bool isPathBlocked = false;

    std::unordered_map<int, DijkstraMap> dmapCache;

    for (size_t i = 0; i < spawners.size(); ++i) {
        // Если путь этого спавнера не пересекал башню, сохраняем его старый маршрут
        if (currentPaths && i < currentPaths->size() && !(*currentPaths)[i].empty()) {
            if (!doesPathIntersectCell((*currentPaths)[i], gridX, gridY)) {
                outNewPaths[i] = (*currentPaths)[i];
                continue;
            }
        }

        // Путь спавнера заблокирован — рассчитываем новый маршрут
        int baseIdx = spawners[i].targetBaseIndex;
        if (baseIdx < -1 || baseIdx >= static_cast<int>(bases.size())) {
            baseIdx = -1;
        }

        if (dmapCache.find(baseIdx) == dmapCache.end()) {
            if (baseIdx == -1) {
                dmapCache[baseIdx] = pathfinder.generateDijkstraMap(grid, bases);
            } else {
                dmapCache[baseIdx] = pathfinder.generateDijkstraMap(grid, {bases[baseIdx]});
            }
        }

        const auto& dmap = dmapCache[baseIdx];
        if (!dmap.isReachable(spawners[i].pos.x, spawners[i].pos.y)) {
            isPathBlocked = true;
            break;
        }

        std::vector<glm::ivec2> bestPath = pathfinder.tracePath(dmap, grid, spawners[i].pos);
        if (bestPath.empty() && dmap.getDistance(spawners[i].pos.x, spawners[i].pos.y) > 0) {
            isPathBlocked = true;
            break;
        }

        bestPath.insert(bestPath.begin(), spawners[i].pos);
        outNewPaths[i] = bestPath;
    }

    // --- 3. ПРОВЕРКА ЖИВЫХ ВРАГОВ: Не заперт ли хотя бы один моб в тупике? ---
    if (!isPathBlocked && activeEnemies) {
        for (const auto& enemy : *activeEnemies) {
            if (!enemy || enemy->isDead() || enemy->isReachedEnd()) continue;

            glm::ivec2 ePos = grid.pixelToGrid(enemy->getPixelPos());

            int baseIdx = enemy->getTargetBaseIndex();
            if (baseIdx < -1 || baseIdx >= static_cast<int>(bases.size())) {
                baseIdx = -1;
            }

            if (dmapCache.find(baseIdx) == dmapCache.end()) {
                if (baseIdx == -1) {
                    dmapCache[baseIdx] = pathfinder.generateDijkstraMap(grid, bases);
                } else {
                    dmapCache[baseIdx] = pathfinder.generateDijkstraMap(grid, {bases[baseIdx]});
                }
            }

            const auto& dmap = dmapCache[baseIdx];
            if (!dmap.isReachable(ePos.x, ePos.y)) {
                // Враг заперт в тупике без выхода к базе!
                isPathBlocked = true;
                break;
            }
        }
    }

    if (isPathBlocked) {
        grid.setCellType(gridX, gridY, oldCellType);
        outNewPaths.clear();
        return false;
    }

    grid.setCellType(gridX, gridY, CellType::Tower);
    return true;
}

