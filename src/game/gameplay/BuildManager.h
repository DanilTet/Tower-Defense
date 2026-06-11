#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include "entities/Tower.h"

class Grid;
class Pathfinder;
class Tower;
struct PlayerStats;
class EntityManager;

class BuildManager {
public:
    void tryBuildOrUpgrade(
        glm::vec2 mousePos, // позиция мыши
        TowerType selectedType,// выбраный тип башни
        PlayerStats& stats, // деньки игрока
        std::vector<std::unique_ptr<Tower>>& towers,
        std::vector<std::unique_ptr<Enemy>>& enemies,// башни
        Grid& gameGrid, // сетка
        Pathfinder& pathfinder, // поиск пути
        const std::vector<glm::ivec2>& spawners, // спавны
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
        const std::vector<glm::ivec2>& spawners,
        const std::vector<glm::ivec2>& bases,
        std::vector<std::vector<glm::ivec2>>& paths,
        std::vector<glm::ivec2>& levelPath
    );

};