#include "ConfigManager.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// инициализация деревьев которіе как словари работают
std::map<TowerType, std::vector<TowerStats>> ConfigManager::s_towerStats;
std::map<EnemyType, EnemyStats> ConfigManager::s_enemyStats;

bool ConfigManager::loadConfigs(const std::string& towerPath, const std::string& enemiesPath) {
	s_towerStats.clear();
	s_enemyStats.clear();
	
	//загрузка башен
	std::ifstream tFile(towerPath); // откріваем файл
	if (tFile.is_open()) {
		json j;
		tFile >> j;

		// список типов для автоматизации парсинга уровней
		std::vector<std::pair<std::string, TowerType>> types = {
			{"Basic", TowerType::Basic},
			{"Sniper", TowerType::Sniper},
			{"Cannon", TowerType::Cannon}
		};

		for (const auto& pair : types) {
			std::string name = pair.first;
			TowerType type = pair.second;

			// если в json конфиг файлі и если это массив обьэктов 
			if (j[name].contains("levels") && j[name]["levels"].is_array()) {
				for (const auto& lvlJson : j[name]["levels"]) {
					TowerStats stats = {
						lvlJson["range"],
						lvlJson["fireRate"],
						lvlJson["damage"],
						lvlJson["cost"],
						lvlJson["splashRadius"],
						lvlJson["buildSound"],
						lvlJson["attackSound"]
					};
					s_towerStats[type].push_back(stats);
				}
			}
			else {
				// обратная совместимость если уровней нету то один базовый уровень
				TowerStats stats = {
					j[name]["range"],
					j[name]["fireRate"],
					j[name]["damage"],
					j[name]["cost"],
					j[name]["splashRadius"],
					j[name]["buildSound"],
					j[name]["attackSound"]
				};
				s_towerStats[type].push_back(stats);
			}
		}
		tFile.close();
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

TowerStats ConfigManager::getTowerStats(TowerType type, int level) {
	auto it = s_towerStats.find(type);
	if (it != s_towerStats.end() && !it->second.empty()) {
		// защита от выхода за границы вектора уровней
		int idx = level - 1;
		if (idx >= (int)it->second.size()) {
			return it->second.back(); // если запросили уровень выше максимального то возвращаем последний
		}
		return it->second[idx];
	}
	return TowerStats{}; // возвращаем пустую структуру в случае ошибки
}

EnemyStats ConfigManager::getEnemyStats(EnemyType type) {
	return s_enemyStats[type];
}