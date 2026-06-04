#include "ConfigManager.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// инициализация деревьев которіе как словари работают
std::map<TowerType, TowerStats> ConfigManager::s_towerStats;
std::map<EnemyType, EnemyStats> ConfigManager::s_enemyStats;

bool ConfigManager::loadConfigs(const std::string& towerPath, const std::string& enemiesPath) {
	//загрузка башен
	std::ifstream tFile(towerPath); // откріваем файл
	if (tFile.is_open()) {
		json j;
		tFile >> j;
		// мапим json на Enum
		// Порядок  range, fireRate, damage, cost
		s_towerStats[TowerType::Basic] = { j["Basic"]["range"],  j["Basic"]["fireRate"],  j["Basic"]["damage"],  j["Basic"]["cost"], j["Basic"]["splashRadius"] };
		s_towerStats[TowerType::Sniper] = { j["Sniper"]["range"], j["Sniper"]["fireRate"], j["Sniper"]["damage"], j["Sniper"]["cost"], j["Sniper"]["splashRadius"] };
		s_towerStats[TowerType::Cannon] = { j["Cannon"]["range"], j["Cannon"]["fireRate"], j["Cannon"]["damage"], j["Cannon"]["cost"], j["Cannon"]["splashRadius"]};
		tFile.close(); // закріваем файл
	}
	else {
		std::cerr << "Failed to load:" << towerPath << std::endl;
		return false;
	}

	// загрузка врагов
	std::ifstream eFile(enemiesPath); // откріваем файл
	if (eFile.is_open()) {
		json j;
		eFile >> j;
		// мапим json на Enum
		s_enemyStats[EnemyType::Basic] = { j["Basic"]["speed"], j["Basic"]["Maxhealth"], j["Basic"]["sizeScale"], j["Basic"]["reward"] };
		s_enemyStats[EnemyType::Fast] = { j["Fast"]["speed"],  j["Fast"]["Maxhealth"],  j["Fast"]["sizeScale"],  j["Fast"]["reward"] };
		s_enemyStats[EnemyType::Tank] = { j["Tank"]["speed"],  j["Tank"]["Maxhealth"],  j["Tank"]["sizeScale"],  j["Tank"]["reward"] };
		eFile.close(); // закріваем файл
	}
	else {
		std::cerr << "Failed to load:" << enemiesPath << std::endl;
		return false;
	}

	std::cout << "Configs loaded successfully!" << std::endl;
	return true;
}

TowerStats ConfigManager::getTowerStats(TowerType type) {
	return s_towerStats[type];
}

EnemyStats ConfigManager::getEnemyStats(EnemyType type) {
	return s_enemyStats[type];
}