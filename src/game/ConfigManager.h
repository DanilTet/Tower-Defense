#pragma once
#include <map>
#include <string>
#include "Tower.h"
#include "Enemy.h"

class ConfigManager {
public:
    // загрузка всех конфигов при старте игры
    static bool loadConfigs(const std::string& towersPath, const std::string& enemiesPath);

    // получение готовых стат
    static TowerStats getTowerStats(TowerType type);
    static EnemyStats getEnemyStats(EnemyType type);

private:
    static std::map<TowerType, TowerStats> s_towerStats; // виды статы башен
    static std::map<EnemyType, EnemyStats> s_enemyStats; // виды статы врагов
};