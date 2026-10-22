#pragma once
#include <memory>
#include <string>
#include <glm/glm.hpp>

class SpriteRenderer;
class Texture2D;
class Grid;

class BackgroundRenderer {
public:
    // СЛОЙ 1: Базовая подложка под фон (заливка земли/травы и горизонтальные пласты породы/склона)
    static void renderSubstrate(
        SpriteRenderer* renderer,
        const std::shared_ptr<Texture2D>& whiteTexture,
        const std::string& backgroundType,
        const Grid* grid,
        float screenWidth,
        float screenHeight
    );

    // СЛОЙ 2: Фоновое окружение (деревянный настил, здание копра, рельсы, забор, трубы)
    static void renderEnvironment(
        SpriteRenderer* renderer,
        const std::shared_ptr<Texture2D>& whiteTexture,
        const std::string& backgroundType,
        const Grid* grid,
        float screenWidth,
        float screenHeight
    );

    // Полная отрисовка обоих слоев (обратная совместимость)
    static void render(
        SpriteRenderer* renderer,
        const std::shared_ptr<Texture2D>& whiteTexture,
        const std::string& backgroundType,
        const Grid* grid,
        float screenWidth,
        float screenHeight
    );

private:
    static void renderPipesCanalSubstrate(
        SpriteRenderer* renderer,
        const std::shared_ptr<Texture2D>& whiteTexture,
        const Grid* grid,
        float screenWidth,
        float screenHeight
    );

    static void renderPipesCanalEnvironment(
        SpriteRenderer* renderer,
        const std::shared_ptr<Texture2D>& whiteTexture,
        const Grid* grid,
        float screenWidth,
        float screenHeight
    );

    static void renderMineEntranceSubstrate(
        SpriteRenderer* renderer,
        const std::shared_ptr<Texture2D>& whiteTexture,
        const Grid* grid,
        float screenWidth,
        float screenHeight
    );

    static void renderMineEntranceEnvironment(
        SpriteRenderer* renderer,
        const std::shared_ptr<Texture2D>& whiteTexture,
        const Grid* grid,
        float screenWidth,
        float screenHeight
    );

    static void renderPipe(
        SpriteRenderer* renderer,
        const std::shared_ptr<Texture2D>& whiteTexture,
        float startX,
        float endX,
        float pipeY,
        float pipeThickness,
        float cellSize,
        float gridOffsetX
    );
};

