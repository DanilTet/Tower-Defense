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
                BaseData bd;
                bd.x = base["x"];
                bd.y = base["y"];
                bd.id = base.value("id", 0);
                data.bases.push_back(bd);
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

        // читаем волны если есть
        if (j.contains("waves") && j["waves"].is_array()) {
            for (const auto& waveJson : j["waves"]) {
                WaveConfig wave;
                if (waveJson.contains("parts") && waveJson["parts"].is_array()) {
                    for (const auto& partJson : waveJson["parts"]) {
                        WavePart part;
                        part.type = partJson.value("type", "Basic");
                        part.count = partJson.value("count", 10);
                        if (partJson.contains("interval")) {
                            part.spawnInterwal = partJson["interval"].get<float>();
                        } else if (partJson.contains("spawnInterwal")) {
                            part.spawnInterwal = partJson["spawnInterwal"].get<float>();
                        }
                        part.delayAfter = partJson.value("delayAfter", 2.0f);
                        wave.parts.push_back(part);
                    }
                }
                data.waves.push_back(wave);
            }
        }
    }
    catch (json::parse_error& e) {
        std::cerr << "ERROR::LEVELMANAGER: JSON parse error: " << e.what() << std::endl;
    }
    return data;
}

bool LevelManager::saveLevelMap(const std::string& filepath, const LevelMapData& data) {
    json j;
    // Попытаться прочесть существующий файл чтобы сохранить прочие секции (например waves)
    std::ifstream inFile(filepath);
    if (inFile.is_open()) {
        try {
            inFile >> j;
        }
        catch (...) {
            j = json::object();
        }
        inFile.close();
    }

    if (!j.is_object()) {
        j = json::object();
    }

    j["map"]["width"] = data.gridWidth;
    j["map"]["height"] = data.gridHeight;
    j["map"]["cellSize"] = data.cellSize;
    j["map"]["offsetX"] = data.offsetX;
    j["map"]["offsetY"] = data.offsetY;
    j["map"]["layout"] = data.layout;

    json spawnersJson = json::array();
    for (const auto& spawner : data.spawners) {
        spawnersJson.push_back({
            { "x", spawner.pos.x },
            { "y", spawner.pos.y },
            { "targetBaseIndex", spawner.targetBaseIndex }
        });
    }
    j["map"]["spawners"] = spawnersJson;

    json basesJson = json::array();
    for (const auto& base : data.bases) {
        basesJson.push_back({
            { "x", base.x },
            { "y", base.y },
            { "id", base.id }
        });
    }
    j["map"]["bases"] = basesJson;

    // Сохраняем волны
    if (!data.waves.empty()) {
        json wavesJson = json::array();
        for (const auto& wave : data.waves) {
            json partsJson = json::array();
            for (const auto& part : wave.parts) {
                partsJson.push_back({
                    { "type", part.type },
                    { "count", part.count },
                    { "interval", part.spawnInterwal },
                    { "delayAfter", part.delayAfter }
                });
            }
            wavesJson.push_back({
                { "parts", partsJson }
            });
        }
        j["waves"] = wavesJson;
    } else if (!j.contains("waves") || !j["waves"].is_array() || j["waves"].empty()) {
        // Если нет секции waves, добавим базовую волну для тестирования
        j["waves"] = json::array({
            {
                { "parts", json::array({
                    {
                        { "type", "Basic" },
                        { "count", 10 },
                        { "interval", 1.0f },
                        { "delayAfter", 2.0f }
                    }
                }) }
            }
        });
    }

    std::ofstream outFile(filepath);
    if (!outFile.is_open()) {
        std::cerr << "ERROR::LEVELMANAGER: Could not open level file for writing: " << filepath << std::endl;
        return false;
    }

    outFile << j.dump(2) << std::endl;
    outFile.close();
    std::cout << "SUCCESS::LEVELMANAGER: Level map saved to " << filepath << std::endl;
    return true;
}