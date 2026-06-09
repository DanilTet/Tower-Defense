#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>
#include "entities/Tower.h"

class SpriteRenderer;
class Texture2D;
class Grid;

class PlacementUI {
private:

public:
    void renderHologram(
        SpriteRenderer* renderer,
        std::shared_ptr<Texture2D> cellTexture,// текстурка башни
        std::shared_ptr<Texture2D> radiusTexture, // текстурка радиуса атаки
        const Grid& gameGrid, // ссылка на сетку поля
        glm::vec2 currentMousePos, // курсор позиция
        TowerType selectedTower, // выбраная башня
        int playerMoney, // деняк
        glm::vec2 panelPos, // позиция менбшки
        bool hasValidPath // есть ли путь чтобы пк не взорвался
    );
};