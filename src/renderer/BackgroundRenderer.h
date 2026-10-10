#pragma once
#include <memory>
#include <string>
#include <glm/glm.hpp>

class SpriteRenderer;
class Texture2D;
class Grid;

class BackgroundRenderer {
public:
    static void render(
        SpriteRenderer* renderer,
        const std::shared_ptr<Texture2D>& whiteTexture,
        const std::string& backgroundType,
        const Grid* grid,
        float screenWidth,
        float screenHeight
    );

private:
    static void renderPipesCanal(
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

