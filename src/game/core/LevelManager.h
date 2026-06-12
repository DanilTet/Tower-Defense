#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>

// новая структура для спавнера чтобі он помнил свою базу
struct SpawnerData {
    glm::ivec2 pos;
    int targetBaseIndex = -1; // по умолчанию искать динамически ближайшую базу
};

// четенькая структура где храниться структура левела
struct LevelMapData {
    int gridWidth = 10;
    int gridHeight = 7;
    float cellSize = 64.0f;
    float offsetX = 20.0f;
    float offsetY = 20.0f;
    std::vector<SpawnerData> spawners;
    std::vector<glm::ivec2> bases;
};

class LevelManager {
public:
    static LevelMapData loadLevelMap(const std::string& filepath);
};