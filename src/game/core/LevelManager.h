#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "WaveData.h"

// новая структура для базы с поддержкой ID для связи со спавнерами
struct BaseData {
    int x = 0;
    int y = 0;
    int id = 0; // ID базы (0, 1, 2...). Спавнеры с таким же targetBaseIndex направляются сюда

    BaseData() = default;
    BaseData(int _x, int _y, int _id = 0) : x(_x), y(_y), id(_id) {}
    BaseData(glm::ivec2 p, int _id = 0) : x(p.x), y(p.y), id(_id) {}

    glm::ivec2 pos() const { return glm::ivec2(x, y); }
    operator glm::ivec2() const { return glm::ivec2(x, y); }
    bool operator==(const BaseData& other) const {
        return x == other.x && y == other.y && id == other.id;
    }
};

// новая структура для спавнера чтобі он помнил свою базу
struct SpawnerData {
    glm::ivec2 pos;
    int targetBaseIndex = -1; // -1 = искать ближайшую базу, >= 0 = конкретный ID базы
};

// четенькая структура где храниться структура левела
struct LevelMapData {
    int gridWidth = 10;
    int gridHeight = 7;
    float cellSize = 64.0f;
    float offsetX = 20.0f;
    float offsetY = 20.0f;
    std::vector<SpawnerData> spawners;
    std::vector<BaseData> bases;
    std::vector<std::vector<int>> layout;
    std::vector<WaveConfig> waves;
};

// Информация об уровне для отображения в меню и редакторе
struct LevelInfo {
    std::string filename; // например: "level_1.json", "custom_map.json"
    std::string name;     // отображаемое имя, например: "LEVEL 1", "My Defense"
    std::string fullPath; // путь для запуска: "res/levels/level_1.json"
    bool isBuiltIn = false; // true для сюжетных уровней
};

class LevelManager {
public:
    static LevelMapData loadLevelMap(const std::string& filepath);
    static bool saveLevelMap(const std::string& filepath, const LevelMapData& data);

    // Управление и синхронизация уровней
    static std::vector<std::string> getLevelDirectories();
    static void syncLevelsBetweenSourceAndBuild();
    static std::vector<LevelInfo> getAvailableLevels();
    static bool saveLevel(const std::string& levelFileName, const LevelMapData& data);
    static bool renameLevel(const std::string& oldFileName, const std::string& newFileName);
    static bool deleteLevel(const std::string& levelFileName);
    static std::string createNewLevel(const std::string& baseName = "custom_map");
    static std::string sanitizeLevelFileName(const std::string& name);
};