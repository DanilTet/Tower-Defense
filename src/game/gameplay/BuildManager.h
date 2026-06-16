#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include "entities/Tower.h"
#include "core/LevelManager.h"
#include "gameplay/EntityManager.h"
#include <string>

class Tower;
class Enemy;
class Grid;
class Pathfinder;
struct PlayerStats;
struct SpawnerData;

class BuildManager {
public:
    void tryBuildOrUpgrade(
        glm::vec2 mousePos, // позиция мыши
        const std::string& selectedType,// выбраный тип башни
        PlayerStats& stats, // деньки игрока
        std::vector<std::unique_ptr<Tower>>& towers,
        std::vector<std::unique_ptr<Enemy>>& enemies,// башни
        Grid& gameGrid, // сетка
        Pathfinder& pathfinder, // поиск пути
        const std::vector<SpawnerData>& spawners, // спавны
        const std::vector<glm::ivec2>& bases,// базы блять
        std::vector<std::vector<glm::ivec2>>& paths,
        std::vector<glm::ivec2>& levelPath
    );

    void sellTower(
        Tower* tower,
        PlayerStats& stats,
        EntityManager& entityManager,
        Grid& gameGrid,
        Pathfinder& pathfinder,
        const std::vector<SpawnerData>& spawners,
        const std::vector<glm::ivec2>& bases,
        std::vector<std::vector<glm::ivec2>>& paths,
        std::vector<glm::ivec2>& levelPath
    );

};