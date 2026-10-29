#pragma once
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "../core/LevelManager.h"

class SpriteRenderer;
class Texture2D;
class Grid;

class EditorEmitterRenderer {
public:
    static void render(
        SpriteRenderer* renderer,
        const std::shared_ptr<Texture2D>& texture,
        const Grid* grid,
        const std::vector<ParticleEmitterConfig>& emitters,
        int selectedEmitterIndex
    );
};

