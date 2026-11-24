#pragma once

#include <cstdint>
#include <string>
#include <glm/glm.hpp>
#include "BackgroundRenderer.h"
#include "../textures/Texture2D.h"

class SpriteRenderer;
class Grid;

/// RAII-кэш статических слоев темы cinnabar_mine в world-space OpenGL FBO.
class CinnabarStaticCache {
public:
    CinnabarStaticCache() = default;
    ~CinnabarStaticCache();

    CinnabarStaticCache(const CinnabarStaticCache&) = delete;
    CinnabarStaticCache& operator=(const CinnabarStaticCache&) = delete;

    CinnabarStaticCache(CinnabarStaticCache&& other) noexcept;
    CinnabarStaticCache& operator=(CinnabarStaticCache&& other) noexcept;

    void release();
    void invalidate();
    bool isValid() const { return m_valid; }

    /// Запекает статику при изменении карты/темы/области видимости и рисует 1 квад + динамические оверлеи.
    bool renderOrBake(
        SpriteRenderer* renderer,
        const Texture2D* whiteTexture,
        const std::string& backgroundType,
        const Grid* grid,
        const glm::mat4& currentWorldProj,
        float screenWidth,
        float screenHeight,
        const BackgroundViewRect& viewRect
    );

private:
    bool ensureFramebuffer(int width, int height);
    bool needsRebake(
        const std::string& backgroundType,
        const Grid* grid,
        float desiredTierScale,
        const glm::vec2& vMin,
        const glm::vec2& vMax
    ) const;

    unsigned int m_fbo = 0;
    Texture2D m_texture;
    bool m_valid = false;

    uint64_t m_cachedGridId = 0;
    uint64_t m_cachedVisualVersion = 0;
    int m_cachedGridW = 0;
    int m_cachedGridH = 0;
    float m_cachedCellSize = 0.0f;
    glm::vec2 m_cachedGridOffset = glm::vec2(0.0f);
    bool m_cachedSidePockets = false;
    std::string m_cachedBackground;
    float m_cachedTierScale = 0.0f;

    glm::vec2 m_worldMin = glm::vec2(0.0f);
    glm::vec2 m_worldMax = glm::vec2(0.0f);
};

