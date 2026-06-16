#include "ConfigManager.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// инициализация деревьев которіе как словари работают
std::map<std::string, std::vector<TowerStats>> ConfigManager::s_towerStats;
std::map<EnemyType, EnemyStats> ConfigManager::s_enemyStats;

bool ConfigManager::loadConfigs(const std::string& towerPath, const std::string& enemiesPath) {
	s_towerStats.clear();
	s_enemyStats.clear();
	
	//загрузка башен
	std::ifstream tFile(towerPath); // откріваем файл
	if (tFile.is_open()) {
		json j;
		tFile >> j;

		// парсим все башни из json
		for (auto& element : j.items()) {
			std::string towerName = element.key(); // имя башни
			json towerData = element.value(); // так называемые внутрености башни

			// cчитываем визуальные настройки из корня башни
			std::string tex = towerData.value("textureId", "towerTexture");
			glm::vec3 col = glm::vec3(1.0f); // по умолчанию белый

			if (towerData.contains("color") && towerData["color"].is_array() && towerData["color"].size() >= 3) {
				col = glm::vec3(
					towerData["color"][0].get<float>(),
					towerData["color"][1].get<float>(),
					towerData["color"][2].get<float>()
				);
			}

			// если есть уровни
			if (towerData.contains("levels") && towerData["levels"].is_array()) {
				for (const auto& lvlJson : towerData["levels"]) {
					TowerStats stats;
					stats.range = lvlJson["range"];
					stats.fireRate = lvlJson["fireRate"];
					stats.damage = lvlJson["damage"];
					stats.cost = lvlJson["cost"];
					stats.splashRadius = lvlJson["splashRadius"];
					stats.rotationSpeed = lvlJson.value("rotationSpeed", 300.0f);
					stats.buildSound = lvlJson["buildSound"];
					stats.attackSound = lvlJson["attackSound"];

					// присваиваем визуал
					stats.textureId = tex;
					stats.color = col;

					// записываем статы в словарь по имени башни
					s_towerStats[towerName].push_back(stats);
				}
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

TowerStats ConfigManager::getTowerStats(const std::string& type, int level) {
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

std::vector<std::string> ConfigManager::getAllTowerTypes() {
	std::vector<std::string> keys;
	for (const auto& pair : s_towerStats) {
		keys.push_back(pair.first);
	}
	return keys;
}