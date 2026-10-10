#pragma once

#include <vector>
#include <string>
#include "entities/Enemy.h"
#include "WaveData.h"
#include "LevelManager.h"

struct SpawnRequest {
	std::string type;
	int spawnerIndex;
};

struct PlayerStats;

class WaveManager {
public:
	//конструктор
	WaveManager();

	bool loadLevel(const std::string& filepath); // считываем левел с файла

	void startNextWave(); // начинаем новую волну
	std::vector<SpawnRequest> update(float dt, const std::vector<SpawnerData>& spawners); // обновляем менеджер врагов по логическим ID
	std::vector<SpawnRequest> update(float dt, int totalSpawners); // совместимость

	// Ранний вызов волны (Фаза 4.2)
	bool canCallEarlyWave() const;
	int getEarlyCallBonus() const;
	void triggerEarlyWave(PlayerStats* player = nullptr);

	// геттеры состояния
	int getCurrentWaveNumber() const { return m_currentWaveIndex + 1; }
	int getTotalWaves() const { return static_cast<int>(m_waves.size()); }
	bool isWaveActive() const { return m_isWaveActive; }
	bool isGameStarted() const { return m_gameStarted; }
	float getWaveCountdown() const { return m_waveCountdown; }
	float getTimeBetweenWaves() const { return m_timeBetweenWaves; }

	bool isAllWavesCompleted() const {
		return !m_isWaveActive && m_currentWaveIndex >= static_cast<int>(m_waves.size());
	}
	// Реестр поддерживаемых типов врагов
	static const std::vector<std::string>& getRegisteredEnemyTypes() {
		static const std::vector<std::string> s_types = {
			"Coal", "Basic", "Fast", "Tank", "Mercury_Large", "Mercury_Medium", "Mercury_Small"
		};
		return s_types;
	}

	// сеттер волны
	void setCurrentWaveIndex(int index);

private:
	std::vector<WaveConfig> m_waves; // Вектор со всеми волнами игры
	bool m_isWaveActive; // флаг активна ли волна
	int m_currentWaveIndex; // индекс текущей волны

	size_t m_currentPartIndex; // какая пачка щас идет
	int m_enemiesSpawnedInCurrentPart; // количество врагов заспавненых из текущей пачки
	float m_spawnTimer; // время спавна между врагами

	int m_currentSpawnerIndex = 0;

	// Межволновая пауза и ранний старт (Фаза 4.2)
	float m_timeBetweenWaves = 15.0f; // базовое время паузы между волнами (сек)
	float m_waveCountdown = 0.0f;    // оставшиеся секунды до следующей волны
	bool m_gameStarted = false;      // запущена ли игра (первая волна)
};