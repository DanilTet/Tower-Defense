#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "../core/LevelManager.h"

class Grid;
class Pathfinder;

class PathService {
public:
    static std::vector<std::vector<glm::ivec2>> calculateAllPaths(
        const Grid& grid,
        Pathfinder& pathfinder,
        const std::vector<SpawnerData>& spawners,
        const std::vector<glm::ivec2>& bases
    );

    static bool isPlacementValid(
        Grid& grid,
        Pathfinder& pathfinder,
        const std::vector<SpawnerData>& spawners,
        const std::vector<glm::ivec2>& bases,
        int gridX,
        int gridY,
        std::vector<std::vector<glm::ivec2>>& outNewPaths
    );
};

