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

class BuildManager {
public:
    void tryBuildOrUpgrade(
        glm::vec2 mousePos, // позиция мыши
        TowerType selectedType,// выбраный тип башни
        int& playerMoney, // деньки игрока
        std::vector<std::unique_ptr<Tower>>& towers,
        std::vector<std::unique_ptr<Enemy>>& enemies,// башни
        Grid& gameGrid, // сетка
        Pathfinder& pathfinder, // поиск пути
        const std::vector<glm::ivec2>& spawners, // спавны
        const std::vector<glm::ivec2>& bases,// базы блять
        std::vector<std::vector<glm::ivec2>>& paths,
        std::vector<glm::ivec2>& levelPath
    );
};