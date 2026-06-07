#pragma once

#include <vector>
#include <string>
#include "entities/Enemy.h"

class Game;

struct WavePart {
	EnemyType type; // тип врага для спавна
	int count; // сколько врагов заспавнить
	float spawnInterwal; // пауза между спавном внутри этой пачки
	float delayAfter; // пауза между спавном после этой пачки
};

struct WaveConfig {
	std::vector<WavePart> parts;
};

class WaveManager {
public:
	//конструктор
	WaveManager();

	bool loadLevel(const std::string& filepath); // считываем левел с файла

	void startNextWave(); // начинаем новую волну
	void update(float dt, Game& game); // обновляем менеджер врагов

	// гетері для дебага потом убрать
	int getCurrentWaveNumber() const { return m_currentWaveIndex + 1; }
	bool isWaveActive() const { return m_isWaveActive; }

private:
	std::vector<WaveConfig> m_waves; // Вектор со всеми волнами игры
	bool m_isWaveActive; // флаг активна ли волна
	int m_currentWaveIndex; // индекс текущей волны

	size_t m_currentPartIndex; // какая пачка щас идет
	int m_enemiesSpawnedInCurrentPart;; // количество врагов заспавненых из текущей пачки
	float m_spawnTimer; // время спавна между врагами
};