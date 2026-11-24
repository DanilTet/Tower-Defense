#include "CinnabarStaticCache.h"
#include "SpriteRenderer.h"
#include "../game/world/Grid.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <utility>

CinnabarStaticCache::~CinnabarStaticCache() {
    release();
}

CinnabarStaticCache::CinnabarStaticCache(CinnabarStaticCache&& other) noexcept
    : m_fbo(other.m_fbo),
      m_texture(std::move(other.m_texture)),
      m_valid(other.m_valid),
      m_cachedGridId(other.m_cachedGridId),
      m_cachedVisualVersion(other.m_cachedVisualVersion),
      m_cachedGridW(other.m_cachedGridW),
      m_cachedGridH(other.m_cachedGridH),
      m_cachedCellSize(other.m_cachedCellSize),
      m_cachedGridOffset(other.m_cachedGridOffset),
      m_cachedSidePockets(other.m_cachedSidePockets),
      m_cachedBackground(std::move(other.m_cachedBackground)),
      m_cachedTierScale(other.m_cachedTierScale),
      m_worldMin(other.m_worldMin),
      m_worldMax(other.m_worldMax)
{
    other.m_fbo = 0;
    other.m_valid = false;
    other.m_cachedGridId = 0;
    other.m_cachedVisualVersion = 0;
}

CinnabarStaticCache& CinnabarStaticCache::operator=(CinnabarStaticCache&& other) noexcept {
    if (this != &other) {
        release();
        m_fbo = other.m_fbo;
        m_texture = std::move(other.m_texture);
        m_valid = other.m_valid;
        m_cachedGridId = other.m_cachedGridId;
        m_cachedVisualVersion = other.m_cachedVisualVersion;
        m_cachedGridW = other.m_cachedGridW;
        m_cachedGridH = other.m_cachedGridH;
        m_cachedCellSize = other.m_cachedCellSize;
        m_cachedGridOffset = other.m_cachedGridOffset;
        m_cachedSidePockets = other.m_cachedSidePockets;
        m_cachedBackground = std::move(other.m_cachedBackground);
        m_cachedTierScale = other.m_cachedTierScale;
        m_worldMin = other.m_worldMin;
        m_worldMax = other.m_worldMax;

        other.m_fbo = 0;
        other.m_valid = false;
        other.m_cachedGridId = 0;
        other.m_cachedVisualVersion = 0;
    }
    return *this;
}

void CinnabarStaticCache::release() {
    if (m_fbo != 0) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
    m_texture = Texture2D();
    m_valid = false;
    m_cachedGridId = 0;
    m_cachedVisualVersion = 0;
    m_cachedBackground.clear();
}

void CinnabarStaticCache::invalidate() {
    m_valid = false;
}

bool CinnabarStaticCache::ensureFramebuffer(int width, int height) {
    if (width <= 0 || height <= 0) {
        return false;
    }

    GLint prevFbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);

    if (m_fbo == 0) {
        glGenFramebuffers(1, &m_fbo);
        if (m_fbo == 0) {
            return false;
        }
    }

    const bool needNewTexture =
        (m_texture.getId() == 0) ||
        (m_texture.getWidth() != width) ||
        (m_texture.getHeight() != height);

    if (needNewTexture) {
        if (!m_texture.createEmpty(width, height, GL_LINEAR, GL_RGBA8, GL_RGBA)) {
            glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFbo));
            release();
            return false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture.getId(), 0);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    }

    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFbo));
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        release();
        return false;
    }
    return true;
}

bool CinnabarStaticCache::needsRebake(
    const std::string& backgroundType,
    const Grid* grid,
    float desiredTierScale,
    const glm::vec2& vMin,
    const glm::vec2& vMax
) const {
    if (!m_valid || m_fbo == 0 || m_texture.getId() == 0 || !grid) {
        return true;
    }
    if (grid->getInstanceId() != m_cachedGridId ||
        grid->getVisualVersion() != m_cachedVisualVersion ||
        grid->getWidth() != m_cachedGridW ||
        grid->getHeight() != m_cachedGridH ||
        std::abs(grid->getCellSize() - m_cachedCellSize) > 1e-4f ||
        glm::length(grid->getOffset() - m_cachedGridOffset) > 1e-4f ||
        grid->hasSidePockets() != m_cachedSidePockets ||
        backgroundType != m_cachedBackground ||
        std::abs(desiredTierScale - m_cachedTierScale) > 0.01f) {
        return true;
    }

    constexpr float kContainEps = 0.5f;
    if (vMin.x < m_worldMin.x - kContainEps ||
        vMin.y < m_worldMin.y - kContainEps ||
        vMax.x > m_worldMax.x + kContainEps ||
        vMax.y > m_worldMax.y + kContainEps) {
        return true;
    }
    return false;
}

bool CinnabarStaticCache::renderOrBake(
    SpriteRenderer* renderer,
    const Texture2D* whiteTexture,
    const std::string& backgroundType,
    const Grid* grid,
    const glm::mat4& currentWorldProj,
    float screenWidth,
    float screenHeight,
    const BackgroundViewRect& viewRect
) {
    if (!renderer || !whiteTexture || !grid || screenWidth <= 1.0f || screenHeight <= 1.0f) {
        return false;
    }
    if (!BackgroundRenderer::isCinnabarMine(backgroundType)) {
        return false;
    }

    glm::vec2 vMin(0.0f, 0.0f);
    glm::vec2 vMax(screenWidth, screenHeight);
    if (viewRect.valid) {
        vMin = glm::min(viewRect.min, viewRect.max);
        vMax = glm::max(viewRect.min, viewRect.max);
    }

    const float viewWorldW = std::max(1.0f, vMax.x - vMin.x);
    const float pixelsPerWorld = screenWidth / viewWorldW;
    const float desiredTierScale = (pixelsPerWorld <= 1.25f) ? 1.0f : ((pixelsPerWorld <= 1.75f) ? 1.5f : 2.0f);

    if (needsRebake(backgroundType, grid, desiredTierScale, vMin, vMax)) {
        GLint glMaxTex = 2048;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &glMaxTex);
        const int maxTexDim = std::clamp(static_cast<int>(glMaxTex), 1024, 4096);

        const float cellSize = std::max(1.0f, grid->getCellSize());
        const glm::vec2 offset = grid->getOffset();

        glm::vec2 mapMin = offset;
        glm::vec2 mapMax = offset + glm::vec2(
            static_cast<float>(grid->getWidth()) * cellSize,
            static_cast<float>(grid->getHeight()) * cellSize
        );
        for (const auto& [pos, type] : grid->getOuterCells()) {
            if (type == CellType::Empty) continue;
            glm::vec2 cellMin = grid->gridToPixel(pos.first, pos.second);
            glm::vec2 cellMax = cellMin + glm::vec2(cellSize);
            mapMin = glm::min(mapMin, cellMin);
            mapMax = glm::max(mapMax, cellMax);
        }
        const glm::vec2 mapPad(cellSize * 6.0f, cellSize * 5.0f);
        mapMin -= mapPad;
        mapMax += mapPad;

        const glm::vec2 viewPad = glm::max(glm::vec2(cellSize * 4.0f), (vMax - vMin) * 0.35f);
        const glm::vec2 paddedViewMin = vMin - viewPad;
        const glm::vec2 paddedViewMax = vMax + viewPad;

        const glm::vec2 unionMin = glm::min(mapMin, paddedViewMin);
        const glm::vec2 unionMax = glm::max(mapMax, paddedViewMax);
        const glm::vec2 unionSize = unionMax - unionMin;

        glm::vec2 newMin = unionMin;
        glm::vec2 newMax = unionMax;
        if (unionSize.x * desiredTierScale > static_cast<float>(maxTexDim) ||
            unionSize.y * desiredTierScale > static_cast<float>(maxTexDim)) {
            newMin = paddedViewMin;
            newMax = paddedViewMax;
        }

        const float worldW = std::max(1.0f, newMax.x - newMin.x);
        const float worldH = std::max(1.0f, newMax.y - newMin.y);
        const float effectiveScale = std::min(
            desiredTierScale,
            std::min(static_cast<float>(maxTexDim) / worldW, static_cast<float>(maxTexDim) / worldH)
        );
        const int texW = std::clamp(static_cast<int>(std::ceil(worldW * effectiveScale)), 1, maxTexDim);
        const int texH = std::clamp(static_cast<int>(std::ceil(worldH * effectiveScale)), 1, maxTexDim);

        renderer->setAdditiveBlending(false);
        renderer->flush();

        if (!ensureFramebuffer(texW, texH)) {
            return false;
        }

        GLint prevFbo = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
        GLint prevViewport[4] = { 0, 0, 0, 0 };
        glGetIntegerv(GL_VIEWPORT, prevViewport);
        GLfloat prevClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        glGetFloatv(GL_COLOR_CLEAR_VALUE, prevClearColor);

        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
        glViewport(0, 0, texW, texH);
        glClearColor(0.055f, 0.060f, 0.078f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        const glm::mat4 bakeProj = glm::ortho(newMin.x, newMax.x, newMax.y, newMin.y, -1.0f, 1.0f);
        renderer->setProjection(bakeProj);
        renderer->beginBatch();

        BackgroundViewRect bakeView;
        bakeView.min = newMin;
        bakeView.max = newMax;
        bakeView.valid = true;

        BackgroundRenderer::renderCinnabarMineSubstrateStatic(renderer, whiteTexture, grid, worldW, worldH, bakeView);
        grid->drawBaseCells(renderer, whiteTexture, glm::vec3(1.0f), backgroundType);
        BackgroundRenderer::renderCinnabarMineEnvironmentStatic(renderer, whiteTexture, grid, worldW, worldH, bakeView);
        grid->drawObstacleCells(renderer, whiteTexture, glm::vec3(1.0f), backgroundType, false);

        renderer->setAdditiveBlending(false);
        renderer->endBatch();

        glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFbo));
        glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
        glClearColor(prevClearColor[0], prevClearColor[1], prevClearColor[2], prevClearColor[3]);
        renderer->setProjection(currentWorldProj);
        renderer->beginBatch();

        m_cachedGridId = grid->getInstanceId();
        m_cachedVisualVersion = grid->getVisualVersion();
        m_cachedGridW = grid->getWidth();
        m_cachedGridH = grid->getHeight();
        m_cachedCellSize = grid->getCellSize();
        m_cachedGridOffset = grid->getOffset();
        m_cachedSidePockets = grid->hasSidePockets();
        m_cachedBackground = backgroundType;
        m_cachedTierScale = desiredTierScale;
        m_worldMin = newMin;
        m_worldMax = newMax;
        m_valid = true;
    }

    // 1. Запеченная статическая текстура одним квадом (Y-flip для OpenGL FBO)
    const SpriteUV fboUV{ glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 0.0f) };
    renderer->drawSprite(&m_texture, m_worldMin, m_worldMax - m_worldMin, 0.0f, glm::vec3(1.0f), fboUV);

    // 2. Поверх кэша — только анимированные лампы и пульсирующие свечения киновари
    BackgroundRenderer::renderCinnabarMineAnimatedOverlay(renderer, whiteTexture, grid, screenWidth, screenHeight, viewRect);
    grid->drawCinnabarObstacleAnimated(renderer, whiteTexture, glm::vec3(1.0f), vMin, vMax, true);

    return true;
}

