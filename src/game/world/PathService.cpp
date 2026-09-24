#include "PathService.h"
#include "Grid.h"
#include "Pathfinder.h"
#include <glm/geometric.hpp>

std::vector<std::vector<glm::ivec2>> PathService::calculateAllPaths(
    const Grid& grid,
    Pathfinder& pathfinder,
    const std::vector<SpawnerData>& spawners,
    const std::vector<glm::ivec2>& bases)
{
    std::vector<std::vector<glm::ivec2>> paths;
    for (size_t i = 0; i < spawners.size(); ++i) {
        int baseIdx = spawners[i].targetBaseIndex;
        std::vector<glm::ivec2> calculatedPath;

        if (baseIdx == -1) {
            int minCost = 999999;
            float minEuclideanDist = 999999.0f;

            for (const auto& base : bases) {
                int currentCost = 0;
                auto path = pathfinder.findPath(grid, spawners[i].pos, base, currentCost);

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
            if (baseIdx < 0 || baseIdx >= static_cast<int>(bases.size())) baseIdx = 0;
            int dummyCost = 0;
            calculatedPath = pathfinder.findPath(grid, spawners[i].pos, bases[baseIdx], dummyCost);
        }

        if (!calculatedPath.empty()) {
            calculatedPath.insert(calculatedPath.begin(), spawners[i].pos);
            paths.push_back(calculatedPath);
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
    std::vector<std::vector<glm::ivec2>>& outNewPaths)
{
    CellType oldCellType = grid.getCellType(gridX, gridY);
    grid.setCellType(gridX, gridY, CellType::Tower);

    outNewPaths.clear();
    bool isPathBlocked = false;

    for (size_t i = 0; i < spawners.size(); ++i) {
        int baseIdx = spawners[i].targetBaseIndex;
        std::vector<glm::ivec2> bestPath;

        if (baseIdx == -1) {
            int minCost = 999999;
            float minEuclideanDist = 999999.0f;

            for (const auto& base : bases) {
                int currentCost = 0;
                auto path = pathfinder.findPath(grid, spawners[i].pos, base, currentCost);

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
            if (baseIdx < 0 || baseIdx >= static_cast<int>(bases.size())) baseIdx = 0;
            int dummyCost = 0;
            bestPath = pathfinder.findPath(grid, spawners[i].pos, bases[baseIdx], dummyCost);
        }

        if (bestPath.empty()) {
            isPathBlocked = true;
            break;
        }

        bestPath.insert(bestPath.begin(), spawners[i].pos);
        outNewPaths.push_back(bestPath);
    }

    if (isPathBlocked) {
        grid.setCellType(gridX, gridY, oldCellType);
        outNewPaths.clear();
        return false;
    }

    return true;
}

