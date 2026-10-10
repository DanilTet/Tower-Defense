#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <string>
#include "../game/core/LevelManager.h"

class SpriteRenderer;
class Texture2D;
class Grid;

class DecorationRenderer {
public:
    static void renderDecoration(
        SpriteRenderer* renderer,
        std::shared_ptr<Texture2D> whiteTexture,
        const DecorationConfig& decor,
        bool isSelected = false,
        float alpha = 1.0f,
        float cellSize = 64.0f
    );

    static void renderDecoration(
        SpriteRenderer* renderer,
        std::shared_ptr<Texture2D> whiteTexture,
        glm::vec2 renderPos,
        const DecorationConfig& decor,
        bool isSelected = false,
        float alpha = 1.0f,
        float cellSize = 64.0f
    );

    static void renderDecorations(
        SpriteRenderer* renderer,
        std::shared_ptr<Texture2D> whiteTexture,
        const std::vector<DecorationConfig>& decorations,
        int selectedIndex = -1,
        float cellSize = 64.0f,
        bool onlyFog = false
    );

    static void renderDecorations(
        SpriteRenderer* renderer,
        std::shared_ptr<Texture2D> whiteTexture,
        const std::vector<DecorationConfig>& decorations,
        glm::vec2 gridOffset,
        float cellSize,
        int selectedIndex = -1,
        bool onlyFog = false
    );

    static void renderDecorations(
        SpriteRenderer* renderer,
        std::shared_ptr<Texture2D> whiteTexture,
        const std::vector<DecorationConfig>& decorations,
        const Grid& grid,
        int selectedIndex = -1,
        bool onlyFog = false
    );

    static void renderPreview(
        SpriteRenderer* renderer,
        std::shared_ptr<Texture2D> whiteTexture,
        const std::string& type,
        glm::vec2 worldPos,
        float rotation = 0.0f,
        float scale = 1.0f,
        float scaleX = 1.0f,
        float scaleY = 1.0f,
        float opacity = 0.5f,
        float cellSize = 64.0f
    );
};

