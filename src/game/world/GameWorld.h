#pragma once
#include <memory>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "../core/LevelManager.h"
#include "../core/WaveManager.h"
#include "../gameplay/PlayerStats.h"
#include "../gameplay/EntityManager.h"
#include "Grid.h"
#include "Pathfinder.h"

struct GameWorld {
    std::unique_ptr<Grid> grid;
    std::unique_ptr<Pathfinder> pathfinder;
    std::unique_ptr<WaveManager> waveManager;
    std::unique_ptr<EntityManager> entityManager;

    PlayerStats playerStats;

    std::vector<SpawnerData> spawners;
    std::vector<glm::ivec2> bases;
    std::vector<std::vector<glm::ivec2>> paths;
    std::vector<glm::ivec2> levelPath;

    std::string currentLevelPath;

    GameWorld();
    ~GameWorld() = default;

    bool loadLevel(const std::string& levelPath, int windowWidth, int windowHeight);
    void recalculateAllPaths();
    void update(float dt);
    void resize(int windowWidth, int windowHeight);
    void spawnEnemy(const std::string& type, int spawnerIndex = 0);
};
