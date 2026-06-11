#pragma once
#include <memory>
#include <glm/glm.hpp>

class Tower;
class SpriteRenderer;
class TextRenderer;
class Texture2D;
class BuildManager;
class EntityManager;
class Grid;
class Pathfinder;
struct PlayerStats;

class TowerMenuUI {
public:
    // обработка клика: возвращает true если клик пришелся по кнопке меню
    bool processClick(
        double mouseX, double mouseY,
        Tower* selectedTower,
        BuildManager* buildManager,
        PlayerStats& stats,
        EntityManager& entityManager,
        Grid& gameGrid,
        Pathfinder& pathfinder,
        const std::vector<glm::ivec2>& spawners,
        const std::vector<glm::ivec2>& bases,
        std::vector<std::vector<glm::ivec2>>& paths,
        std::vector<glm::ivec2>& levelPath,
        Tower*& outSelectedTower
    );

    // отрисовка меню
    void render(
        Tower* selectedTower,
        SpriteRenderer* renderer,
        TextRenderer* textRenderer,
        std::shared_ptr<Texture2D> cellTexture, // используем базовую текстуру как заглушку
        const Grid& gameGrid
    );
};