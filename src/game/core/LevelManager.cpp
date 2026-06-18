#include "LevelManager.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

LevelMapData LevelManager::loadLevelMap(const std::string& filepath) {
    LevelMapData data;
    std::ifstream file(filepath);

    if (!file.is_open()) {
        std::cerr << "ERROR::LEVELMANAGER: Could not open level file: " << filepath << std::endl;
        return data;
    }

    try {
        json j;
        file >> j;

        // если есть блок map в json то читаем его 
        if (j.contains("map")) {
            data.gridWidth = j["map"].value("width", 10);
            data.gridHeight = j["map"].value("height", 7);
            data.cellSize = j["map"].value("cellSize", 64.0f);
            data.offsetX = j["map"].value("offsetX", 20.0f);
            data.offsetY = j["map"].value("offsetY", 20.0f);

            // читаем спавнеры
            for (const auto& spawner : j["map"]["spawners"]) {
                SpawnerData sd;
                sd.pos = { spawner["x"], spawner["y"] };
                sd.targetBaseIndex = spawner.value("targetBaseIndex", -1);
                data.spawners.push_back(sd);
            }

            // читаем базы
            for (const auto& base : j["map"]["bases"]) {
                data.bases.push_back({ base["x"], base["y"] });
            }
            // парсинг сетки
            //0 = Земля(Ground)
            //1 = Дорога(Path)
            //2 = Платформа(Platform)
            //3 = Вода / Скала(Scenery)
            if (j["map"].contains("layout")) {
                for (const auto& row : j["map"]["layout"]) {
                    std::vector<int> rowData;
                    for (const auto& cell : row) {
                        rowData.push_back(cell.get<int>());
                    }
                    data.layout.push_back(rowData);
                }
            }
        }
        else {
            std::cerr << "WARNING::LEVELMANAGER: No 'map' section found in " << filepath << std::endl;
        }
    }
    catch (json::parse_error& e) {
        std::cerr << "ERROR::LEVELMANAGER: JSON parse error: " << e.what() << std::endl;
    }
    return data;
}