#pragma once
#include <map>
#include <string>
#include <vector>
#include "entities/Tower.h"
#include "entities/Enemy.h"

class ConfigManager {
public:
    // загрузка всех конфигов при старте игры
    static bool loadConfigs(const std::string& towersPath, const std::string& enemiesPath);

    // получение готовых стат
    static TowerStats getTowerStats(const std::string& type, int level = 1);
    static EnemyStats getEnemyStats(EnemyType type);
    // метод для получения всех загруженых имен
    static std::vector<std::string> getAllTowerTypes();

private:
    static std::map<std::string, std::vector<TowerStats>> s_towerStats; // виды статы башен
    static std::map<EnemyType, EnemyStats> s_enemyStats; // виды статы врагов
};