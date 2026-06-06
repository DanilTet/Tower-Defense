#pragma once
#include <map>
#include <string>
#include <vector>
#include "Tower.h"
#include "Enemy.h"

class ConfigManager {
public:
    // загрузка всех конфигов при старте игры
    static bool loadConfigs(const std::string& towersPath, const std::string& enemiesPath);

    // получение готовых стат
    static TowerStats getTowerStats(TowerType type, int level = 1);
    static EnemyStats getEnemyStats(EnemyType type);

private:
    static std::map<TowerType, std::vector<TowerStats>> s_towerStats; // виды статы башен
    static std::map<EnemyType, EnemyStats> s_enemyStats; // виды статы врагов
};