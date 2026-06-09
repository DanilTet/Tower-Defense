#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>

// четенькая структура где храниться структура левела
struct LevelMapData {
    int gridWidth = 10;
    int gridHeight = 7;
    float cellSize = 64.0f;
    float offsetX = 20.0f;
    float offsetY = 20.0f;
    std::vector<glm::ivec2> spawners;
    std::vector<glm::ivec2> bases;
};

class LevelManager {
public:
    static LevelMapData loadLevelMap(const std::string& filepath);
};