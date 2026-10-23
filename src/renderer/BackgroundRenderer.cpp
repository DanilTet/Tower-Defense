#include "BackgroundRenderer.h"
#include "SpriteRenderer.h"
#include "../textures/Texture2D.h"
#include "../game/world/Grid.h"
#include <algorithm>
#include <cmath>

void BackgroundRenderer::renderSubstrate(
    SpriteRenderer* renderer,
    const std::shared_ptr<Texture2D>& whiteTexture,
    const std::string& backgroundType,
    const Grid* grid,
    float screenWidth,
    float screenHeight
) {
    if (!renderer || !whiteTexture) return;

    if (backgroundType == "pipes_canal" || backgroundType == "channel") {
        renderPipesCanalSubstrate(renderer, whiteTexture, grid, screenWidth, screenHeight);
    } else if (backgroundType == "mine_entrance" || backgroundType == "mine") {
        renderMineEntranceSubstrate(renderer, whiteTexture, grid, screenWidth, screenHeight);
    } else {
        // Стандартная минималистичная темная подложка (с широким охватом для зума и панорамирования)
        glm::vec2 gridOffset = grid ? grid->getOffset() : glm::vec2(0.0f);
        float cellSize = grid ? grid->getCellSize() : 64.0f;
        float gridW = grid ? static_cast<float>(grid->getWidth()) * cellSize : screenWidth;
        float gridH = grid ? static_cast<float>(grid->getHeight()) * cellSize : screenHeight;
        float marginX = std::max(1600.0f, screenWidth * 2.5f);
        float marginY = std::max(1200.0f, screenHeight * 2.0f);
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(gridOffset.x - marginX, gridOffset.y - marginY),
            glm::vec2(gridW + marginX * 2.0f, gridH + marginY * 2.0f),
            0.0f,
            glm::vec3(0.12f, 0.13f, 0.16f)
        );
    }
}

void BackgroundRenderer::renderEnvironment(
    SpriteRenderer* renderer,
    const std::shared_ptr<Texture2D>& whiteTexture,
    const std::string& backgroundType,
    const Grid* grid,
    float screenWidth,
    float screenHeight
) {
    if (!renderer || !whiteTexture) return;

    if (backgroundType == "pipes_canal" || backgroundType == "channel") {
        renderPipesCanalEnvironment(renderer, whiteTexture, grid, screenWidth, screenHeight);
    } else if (backgroundType == "mine_entrance" || backgroundType == "mine") {
        renderMineEntranceEnvironment(renderer, whiteTexture, grid, screenWidth, screenHeight);
    }
}

void BackgroundRenderer::render(
    SpriteRenderer* renderer,
    const std::shared_ptr<Texture2D>& whiteTexture,
    const std::string& backgroundType,
    const Grid* grid,
    float screenWidth,
    float screenHeight
) {
    renderSubstrate(renderer, whiteTexture, backgroundType, grid, screenWidth, screenHeight);
    renderEnvironment(renderer, whiteTexture, backgroundType, grid, screenWidth, screenHeight);
}

void BackgroundRenderer::renderPipesCanalSubstrate(
    SpriteRenderer* renderer,
    const std::shared_ptr<Texture2D>& whiteTexture,
    const Grid* grid,
    float screenWidth,
    float screenHeight
) {
    // Параметры мира и сетки
    glm::vec2 gridOffset = grid ? grid->getOffset() : glm::vec2(0.0f);
    float cellSize = grid ? grid->getCellSize() : 64.0f;
    float gridW = grid ? static_cast<float>(grid->getWidth()) * cellSize : screenWidth;
    float gridH = grid ? static_cast<float>(grid->getHeight()) * cellSize : screenHeight;

    // В координатах мира: широкий охват вокруг сетки для зума и панорамирования
    float marginX = std::max(1600.0f, screenWidth * 2.5f);
    float marginY = std::max(1200.0f, screenHeight * 2.0f);
    float startX = gridOffset.x - marginX;
    float endX = gridOffset.x + gridW + marginX;
    float totalW = endX - startX;

    float startY = gridOffset.y - marginY;
    float endY = gridOffset.y + gridH + marginY;
    float totalH = endY - startY;

    // 1. Базовый сочно-зеленый склон холма / трава
    renderer->drawSprite(
        whiteTexture,
        glm::vec2(startX, startY),
        glm::vec2(totalW, totalH),
        0.0f,
        glm::vec3(0.18f, 0.40f, 0.16f)
    );

    // Плавные горизонтальные градиентные полосы травяного склона
    float slopeBandH = std::max(36.0f, cellSize * 0.75f);
    int slopeBands = static_cast<int>(totalH / slopeBandH) + 1;
    for (int i = 0; i < slopeBands; ++i) {
        float bY = startY + static_cast<float>(i) * slopeBandH;
        float factor = static_cast<float>(i % 12) / 12.0f;
        float wave = std::sin(factor * 3.14159f * 2.0f) * 0.03f;
        glm::vec3 bandCol = glm::vec3(
            0.17f + 0.04f * factor + wave,
            0.37f + 0.06f * factor + wave * 1.5f,
            0.15f + 0.03f * factor + wave * 0.8f
        );
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(startX, bY),
            glm::vec2(totalW, slopeBandH + 1.0f),
            0.0f,
            bandCol
        );
    }
}

void BackgroundRenderer::renderPipesCanalEnvironment(
    SpriteRenderer* renderer,
    const std::shared_ptr<Texture2D>& whiteTexture,
    const Grid* grid,
    float screenWidth,
    float screenHeight
) {
    if (!grid) return;

    glm::vec2 gridOffset = grid->getOffset();
    float cellSize = grid->getCellSize();
    float gridW = static_cast<float>(grid->getWidth()) * cellSize;
    float gridH = static_cast<float>(grid->getHeight()) * cellSize;

    float marginX = std::max(1600.0f, screenWidth * 2.5f);
    float startX = gridOffset.x - marginX;
    float endX = gridOffset.x + gridW + marginX;

    // 2. Монументальные трубы дюкеров вдоль верхней и нижней границ поля
    // Толщина трубы и зазор строго пропорциональны cellSize, чтобы координаты тайлов декора и эмиттеров
    // оставались строго совмещены с геометрией труб при любом изменении размера сетки
    float pipeThickness = 1.15f * cellSize;
    float pipeGap = 0.15f * cellSize;

    float topPipeY = gridOffset.y - pipeThickness - pipeGap;
    float botPipeY = gridOffset.y + gridH + pipeGap;

    renderPipe(renderer, whiteTexture, startX, endX, topPipeY, pipeThickness, cellSize, gridOffset.x);
    renderPipe(renderer, whiteTexture, startX, endX, botPipeY, pipeThickness, cellSize, gridOffset.x);
}

void BackgroundRenderer::renderPipe(
    SpriteRenderer* renderer,
    const std::shared_ptr<Texture2D>& whiteTexture,
    float startX,
    float endX,
    float pipeY,
    float pipeThickness,
    float cellSize,
    float gridOffsetX
) {
    float totalW = endX - startX;

    // 1. Мягкая тень от трубы на траву (свет падает сверху)
    float shadowY = pipeY + pipeThickness;
    float shadowThick1 = std::max(16.0f, pipeThickness * 0.22f);
    float shadowThick2 = std::max(12.0f, pipeThickness * 0.16f);
    renderer->drawSpriteRGBA(
        whiteTexture,
        glm::vec2(startX, shadowY),
        glm::vec2(totalW, shadowThick1),
        0.0f,
        glm::vec4(0.04f, 0.10f, 0.03f, 0.55f)
    );
    renderer->drawSpriteRGBA(
        whiteTexture,
        glm::vec2(startX, shadowY + shadowThick1 * 0.5f),
        glm::vec2(totalW, shadowThick2),
        0.0f,
        glm::vec4(0.06f, 0.14f, 0.05f, 0.28f)
    );

    // 2. Опорные бетонные блоки (компенсаторы):
    // Привязаны СТРОГО к целым клеткам сетки (каждые 4 клетки), чтобы декорации и эмиттеры на трубах
    // никогда не съезжали относительно геометрии трубы при изменении размеров сетки
    float cubeSpacing = 4.0f * cellSize;
    float cubeW = pipeThickness * 1.30f; // Ширина больше диаметра трубы
    float cubeH = pipeThickness * 1.35f; // Высота больше диаметра трубы
    float cubeY = pipeY - (cubeH - pipeThickness) * 0.5f;

    // Привязываем сетку компенсаторов к gridOffsetX
    float firstCubeX = gridOffsetX - std::floor((gridOffsetX - startX) / cubeSpacing) * cubeSpacing;

    // Рисуем заднюю тень и основания кубов
    for (float cx = firstCubeX; cx <= endX; cx += cubeSpacing) {
        // Тень от массивного бетонного блока на траву
        float shadowH = std::max(4.0f, 0.20f * cellSize);
        renderer->drawSpriteRGBA(
            whiteTexture,
            glm::vec2(cx - 4.0f, cubeY + cubeH),
            glm::vec2(cubeW + 8.0f, shadowH),
            0.0f,
            glm::vec4(0.03f, 0.08f, 0.02f, 0.65f)
        );

        // Внешний контур бетонного куба
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(cx, cubeY),
            glm::vec2(cubeW, cubeH),
            0.0f,
            glm::vec3(0.20f, 0.22f, 0.24f)
        );

        // Основной объем бетона
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(cx + 2.0f, cubeY + 2.0f),
            glm::vec2(cubeW - 4.0f, cubeH - 4.0f),
            0.0f,
            glm::vec3(0.48f, 0.50f, 0.53f)
        );

        // Верхняя грань (светлая фаска)
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(cx + 2.0f, cubeY + 2.0f),
            glm::vec2(cubeW - 4.0f, 5.0f),
            0.0f,
            glm::vec3(0.72f, 0.74f, 0.77f)
        );

        // Левая грань (освещенная фаска)
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(cx + 2.0f, cubeY + 7.0f),
            glm::vec2(4.0f, cubeH - 12.0f),
            0.0f,
            glm::vec3(0.60f, 0.62f, 0.65f)
        );

        // Нижняя теневая грань
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(cx + 2.0f, cubeY + cubeH - 8.0f),
            glm::vec2(cubeW - 4.0f, 6.0f),
            0.0f,
            glm::vec3(0.30f, 0.32f, 0.34f)
        );

        // Правая теневая фаска
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(cx + cubeW - 6.0f, cubeY + 7.0f),
            glm::vec2(4.0f, cubeH - 15.0f),
            0.0f,
            glm::vec3(0.38f, 0.40f, 0.42f)
        );

        // Вертикальный компенсаторный шов в центре
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(cx + cubeW * 0.5f - 1.0f, cubeY + 4.0f),
            glm::vec2(2.0f, cubeH - 10.0f),
            0.0f,
            glm::vec3(0.22f, 0.24f, 0.26f)
        );
    }

    // 3. Объемный горизонтальный цилиндр трубы (проходит сквозь компенсаторы)
    struct PipeBand {
        float relY;
        float relH;
        glm::vec3 color;
    };

    static const PipeBand bands[] = {
        { 0.00f, 0.07f, glm::vec3(0.16f, 0.14f, 0.13f) }, // Верхний темный контур
        { 0.07f, 0.11f, glm::vec3(0.36f, 0.32f, 0.28f) }, // Затененная сталь
        { 0.18f, 0.14f, glm::vec3(0.56f, 0.52f, 0.48f) }, // Освещенный металл
        { 0.32f, 0.20f, glm::vec3(0.88f, 0.90f, 0.94f) }, // Мощный цилиндрический блик
        { 0.52f, 0.18f, glm::vec3(0.48f, 0.32f, 0.24f) }, // Темно-ржавый металл
        { 0.70f, 0.18f, glm::vec3(0.28f, 0.18f, 0.14f) }, // Глубокая ржавчина
        { 0.88f, 0.12f, glm::vec3(0.12f, 0.09f, 0.08f) }  // Нижний контур тени
    };

    for (const auto& b : bands) {
        float bY = pipeY + b.relY * pipeThickness;
        float bH = b.relH * pipeThickness + 0.5f;
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(startX, bY),
            glm::vec2(totalW, bH),
            0.0f,
            b.color
        );
    }

    // 4. Манжеты / сальники герметизации на входе и выходе трубы из куба
    float collarExtra = pipeThickness * 0.12f;
    float collarH = pipeThickness + collarExtra;
    float collarY = pipeY - collarExtra * 0.5f;
    float collarW = pipeThickness * 0.10f;
    for (float cx = firstCubeX; cx <= endX; cx += cubeSpacing) {
        // Левая манжета (вход трубы в блок)
        renderer->drawSprite(whiteTexture, glm::vec2(cx - collarW * 0.5f, collarY), glm::vec2(collarW, collarH), 0.0f, glm::vec3(0.25f, 0.27f, 0.30f));
        renderer->drawSprite(whiteTexture, glm::vec2(cx - collarW * 0.5f + 2.0f, collarY + 3.0f), glm::vec2(std::max(2.0f, collarW - 4.0f), collarH - 6.0f), 0.0f, glm::vec3(0.75f, 0.78f, 0.82f));

        // Правая манжета (выход трубы из блока)
        renderer->drawSprite(whiteTexture, glm::vec2(cx + cubeW - collarW * 0.5f, collarY), glm::vec2(collarW, collarH), 0.0f, glm::vec3(0.25f, 0.27f, 0.30f));
        renderer->drawSprite(whiteTexture, glm::vec2(cx + cubeW - collarW * 0.5f + 2.0f, collarY + 3.0f), glm::vec2(std::max(2.0f, collarW - 4.0f), collarH - 6.0f), 0.0f, glm::vec3(0.75f, 0.78f, 0.82f));
    }

    // 5. Поперечные стальные кольца/швы (фланцы) через каждые 2 клетки сетки
    float ringSpacing = 2.0f * cellSize;
    float ringW = pipeThickness * 0.14f;
    float ringExtra = pipeThickness * 0.12f;
    float ringH = pipeThickness + ringExtra;
    float ringY = pipeY - ringExtra * 0.5f;

    float firstRingX = gridOffsetX - std::floor((gridOffsetX - startX) / ringSpacing) * ringSpacing;
    for (float rx = firstRingX; rx <= endX; rx += ringSpacing) {
        // Темный шов слева
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(rx - 2.0f, ringY),
            glm::vec2(2.0f, ringH),
            0.0f,
            glm::vec3(0.10f, 0.08f, 0.08f)
        );

        // Рельефное кольцо / фланец
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(rx, ringY),
            glm::vec2(ringW, ringH),
            0.0f,
            glm::vec3(0.42f, 0.38f, 0.36f)
        );

        // Блик по центру кольца
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(rx + 1.5f, pipeY + 0.32f * pipeThickness),
            glm::vec2(ringW - 3.0f, 0.20f * pipeThickness),
            0.0f,
            glm::vec3(0.92f, 0.94f, 0.96f)
        );

        // Двойные шляпки болтов сверху и снизу кольца
        renderer->drawSprite(whiteTexture, glm::vec2(rx + 2.0f, ringY + 2.0f), glm::vec2(3.0f, 3.0f), 0.0f, glm::vec3(0.85f, 0.85f, 0.85f));
        renderer->drawSprite(whiteTexture, glm::vec2(rx + 2.0f, ringY + ringH - 5.0f), glm::vec2(3.0f, 3.0f), 0.0f, glm::vec3(0.85f, 0.85f, 0.85f));
    }
}

void BackgroundRenderer::renderMineEntranceSubstrate(
    SpriteRenderer* renderer,
    const std::shared_ptr<Texture2D>& whiteTexture,
    const Grid* grid,
    float screenWidth,
    float screenHeight
) {
    if (!renderer || !whiteTexture) return;

    // Параметры мира и сетки
    glm::vec2 gridOffset = grid ? grid->getOffset() : glm::vec2(0.0f);
    float cellSize = grid ? grid->getCellSize() : 64.0f;
    int gridW_tiles = grid ? grid->getWidth() : 10;
    int gridH_tiles = grid ? grid->getHeight() : 7;
    float gridW = static_cast<float>(gridW_tiles) * cellSize;
    float gridH = static_cast<float>(gridH_tiles) * cellSize;

    // В координатах мира: широкий охват вокруг сетки для свободного панорамирования и зума
    float marginX = std::max(1600.0f, screenWidth * 2.5f);
    float marginY = std::max(1200.0f, screenHeight * 2.0f);
    float startX = gridOffset.x - marginX;
    float endX = gridOffset.x + gridW + marginX;
    float totalW = endX - startX;

    float startY = gridOffset.y - marginY;
    float endY = gridOffset.y + gridH + marginY;
    float totalH = endY - startY;

    // 1. БАЗОВАЯ ЗЕМЛЯ ОКРУЖЕНИЯ: вытоптанный темно-рыжий каменистый грунт рудника
    renderer->drawSprite(
        whiteTexture,
        glm::vec2(startX, startY),
        glm::vec2(totalW, totalH),
        0.0f,
        glm::vec3(0.22f, 0.15f, 0.10f)
    );

    // Плавные полосы горизонтальных пластов породы и высохшей глины
    float bandH = std::max(40.0f, cellSize * 0.8f);
    int numBands = static_cast<int>(totalH / bandH) + 1;
    for (int i = 0; i < numBands; ++i) {
        float bY = startY + static_cast<float>(i) * bandH;
        float factor = static_cast<float>(i % 10) / 10.0f;
        float wave = std::sin(factor * 3.14159f * 2.0f) * 0.02f;
        glm::vec3 bandCol = glm::vec3(
            0.20f + 0.035f * factor + wave,
            0.13f + 0.025f * factor + wave * 0.8f,
            0.09f + 0.020f * factor + wave * 0.6f
        );
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(startX, bY),
            glm::vec2(totalW, bandH + 1.0f),
            0.0f,
            bandCol
        );
    }
}

void BackgroundRenderer::renderMineEntranceEnvironment(
    SpriteRenderer* renderer,
    const std::shared_ptr<Texture2D>& whiteTexture,
    const Grid* grid,
    float screenWidth,
    float screenHeight
) {
    if (!renderer || !whiteTexture || !grid) return;

    // Параметры мира и сетки
    glm::vec2 gridOffset = grid->getOffset();
    float cellSize = grid->getCellSize();
    int gridW_tiles = grid->getWidth();
    int gridH_tiles = grid->getHeight();
    float gridW = static_cast<float>(gridW_tiles) * cellSize;
    float gridH = static_cast<float>(gridH_tiles) * cellSize;

    float marginX = std::max(1600.0f, screenWidth * 2.5f);
    float marginY = std::max(1200.0f, screenHeight * 2.0f);
    float startX = gridOffset.x - marginX;
    float endX = gridOffset.x + gridW + marginX;
    float totalW = endX - startX;

    float startY = gridOffset.y - marginY;
    float endY = gridOffset.y + gridH + marginY;
    float totalH = endY - startY;

    float yardX = gridOffset.x;
    float yardY = gridOffset.y;
    float yardW = gridW;
    float yardH = gridH;

    // Вспомогательный детерминированный генератор псевдослучайных чисел
    auto hash2D = [](int x, int y, uint32_t seed) -> uint32_t {
        uint32_t h = seed ^ (static_cast<uint32_t>(x) * 374761393u) ^ (static_cast<uint32_t>(y) * 668265263u);
        h = (h ^ (h >> 13)) * 1274126177u;
        return h ^ (h >> 16);
    };

    auto rand01 = [](uint32_t val) -> float {
        return static_cast<float>(val & 0xFFFF) / 65535.0f;
    };

    const auto& outerCells = grid->getOuterCells();
    auto isOuterTile = [&](int tx, int ty) -> bool {
        return outerCells.find({tx, ty}) != outerCells.end();
    };

    // =========================================================================
    // 1. ДЕТАЛИЗИРОВАННАЯ УЛИЦА: РАЗНОРОДНАЯ ТЕКСТУРА ПОЧВЫ ЗА ПРЕДЕЛАМИ СЕТКИ
    // (естественные мягкие пятна суглинка, выжженной бурой земли, пыли и сланца)
    // =========================================================================
    float patchStep = cellSize * 1.5f;
    for (float py = startY; py < endY; py += patchStep) {
        for (float px = startX; px < endX; px += patchStep) {
            int tx = static_cast<int>(std::floor((px - yardX) / cellSize));
            int ty = static_cast<int>(std::floor((py - yardY) / cellSize));

            // Не рисуем пятна поверх клеток двора и внешних клеток
            if (tx >= 0 && tx < gridW_tiles && ty >= 0 && ty < gridH_tiles) continue;
            if (isOuterTile(tx, ty)) continue;

            uint32_t h = hash2D(tx, ty, 908209u);
            float rType = rand01(h);
            float rShiftX = (rand01(hash2D(tx, ty, 1103515245u)) - 0.5f) * patchStep * 0.4f;
            float rShiftY = (rand01(hash2D(tx, ty, 123456789u)) - 0.5f) * patchStep * 0.4f;
            float pW = patchStep * (0.85f + 0.50f * rand01(h >> 3));
            float pH = patchStep * (0.80f + 0.45f * rand01(h >> 7));

            glm::vec4 col(0.0f);
            if (rType < 0.28f) {
                // Суглинок (warm loam / глинистая охра)
                col = glm::vec4(0.36f, 0.24f, 0.15f, 0.40f);
            } else if (rType < 0.55f) {
                // Выжженная бурая земля (scorched brown earth)
                col = glm::vec4(0.17f, 0.11f, 0.08f, 0.38f);
            } else if (rType < 0.78f) {
                // Каменистая пыль (stone dust / супесь)
                col = glm::vec4(0.40f, 0.36f, 0.30f, 0.30f);
            } else {
                // Вкрапления темного сланца (dark shale / порода)
                col = glm::vec4(0.18f, 0.19f, 0.21f, 0.35f);
            }

            glm::vec2 pos(px + rShiftX, py + rShiftY);
            renderer->drawSpriteRGBA(whiteTexture, pos, glm::vec2(pW, pH), 0.0f, col);
            renderer->drawSpriteRGBA(whiteTexture, pos - glm::vec2(pW * 0.1f, pH * 0.1f), glm::vec2(pW * 1.2f, pH * 1.2f), 0.0f, glm::vec4(col.r, col.g, col.b, col.a * 0.45f));
        }
    }

    // =========================================================================
    // 2. ПРОЦЕДУРНЫЕ ДЕТАЛИ: КАМУШКИ С 2.5D ОБЪЕМОМ И СУХОЙ БУРЬЯН
    // =========================================================================
    float detailStep = cellSize * 0.85f;
    for (float py = startY; py < endY; py += detailStep) {
        for (float px = startX; px < endX; px += detailStep) {
            int tx = static_cast<int>(std::floor((px - yardX) / cellSize));
            int ty = static_cast<int>(std::floor((py - yardY) / cellSize));

            if (tx >= 0 && tx < gridW_tiles && ty >= 0 && ty < gridH_tiles) continue;
            if (isOuterTile(tx, ty)) continue;

            uint32_t hDet = hash2D(tx, ty, 521471u);
            uint32_t detType = hDet % 100;

            // Мелкие камушки с объемом (светлая грань сверху, теневая подсечка снизу)
            if (detType < 40) {
                float kx = px + rand01(hDet) * detailStep * 0.7f;
                float ky = py + rand01(hDet >> 4) * detailStep * 0.7f;
                float kw = std::max(4.0f, (cellSize / 64.0f) * (4.0f + 5.0f * rand01(hDet >> 8)));
                float kh = std::max(3.0f, kw * (0.60f + 0.25f * rand01(hDet >> 12)));

                glm::vec3 stoneCol = (detType % 2 == 0)
                    ? glm::vec3(0.33f, 0.31f, 0.29f)
                    : glm::vec3(0.24f, 0.25f, 0.27f);

                // Падающая тень на землю снизу
                renderer->drawSpriteRGBA(whiteTexture, glm::vec2(kx - 1.0f, ky + kh * 0.7f), glm::vec2(kw + 2.0f, kh * 0.6f), 0.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.40f));
                // Тело камня
                renderer->drawSprite(whiteTexture, glm::vec2(kx, ky), glm::vec2(kw, kh), 0.0f, stoneCol);
                // Светлая грань сверху (освещенная фаска)
                renderer->drawSprite(whiteTexture, glm::vec2(kx, ky), glm::vec2(kw, std::max(1.0f, kh * 0.35f)), 0.0f, stoneCol + glm::vec3(0.18f, 0.17f, 0.15f));
                // Теневой низ камня
                renderer->drawSprite(whiteTexture, glm::vec2(kx, ky + kh - std::max(1.0f, kh * 0.28f)), glm::vec2(kw, std::max(1.0f, kh * 0.28f)), 0.0f, stoneCol * 0.60f);
            }
            // Редкие пучки сухой жухлой травы/бурьяна по краям локации
            else if (detType >= 82 && detType < 94) {
                float distToYard = std::min(
                    std::min(std::abs(px - yardX), std::abs(px - (yardX + yardW))),
                    std::min(std::abs(py - yardY), std::abs(py - (yardY + yardH)))
                );
                if (distToYard > cellSize * 0.5f) {
                    float gx = px + rand01(hDet) * detailStep * 0.7f;
                    float gy = py + rand01(hDet >> 3) * detailStep * 0.7f;
                    float sc = cellSize / 64.0f;

                    // Тень у корней травы
                    renderer->drawSpriteRGBA(whiteTexture, glm::vec2(gx - 2.0f * sc, gy + 8.0f * sc), glm::vec2(12.0f * sc, 3.0f * sc), 0.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.30f));
                    // Темное основание бурьяна
                    renderer->drawSprite(whiteTexture, glm::vec2(gx + 1.0f * sc, gy + 5.0f * sc), glm::vec2(6.0f * sc, 4.0f * sc), 0.0f, glm::vec3(0.24f, 0.20f, 0.11f));
                    // Стебли жухлой травы
                    glm::vec3 strawCol(0.46f, 0.40f, 0.20f);
                    renderer->drawSprite(whiteTexture, glm::vec2(gx - 1.0f * sc, gy), glm::vec2(std::max(1.5f, 2.0f * sc), 8.0f * sc), 0.0f, strawCol * 0.90f);
                    renderer->drawSprite(whiteTexture, glm::vec2(gx + 2.5f * sc, gy - 3.0f * sc), glm::vec2(std::max(1.5f, 2.0f * sc), 11.0f * sc), 0.0f, strawCol * 1.10f);
                    renderer->drawSprite(whiteTexture, glm::vec2(gx + 5.5f * sc, gy + 1.0f * sc), glm::vec2(std::max(1.5f, 2.0f * sc), 7.0f * sc), 0.0f, strawCol * 0.85f);
                }
            }
        }
    }

    // =========================================================================
    // 3. ПЛАВНЫЙ ПЕРЕХОД ВЫТОПТАННОГО ГРУНТА ВОКРУГ ДВОРА (КРОМКА, КОЛЕИ, САПОГИ)
    // =========================================================================
    auto drawTrampledRimVertical = [&](float edgeX, bool isLeftEdge) {
        float segH = cellSize * 0.45f;
        int numSegs = static_cast<int>(yardH / segH) + 1;
        for (int s = 0; s < numSegs; ++s) {
            float sy = yardY + static_cast<float>(s) * segH;
            uint32_t hRim = hash2D(static_cast<int>(edgeX), s, 442117u);
            float rimThickness = cellSize * (0.20f + 0.22f * rand01(hRim));
            float rx = isLeftEdge ? (edgeX - rimThickness * 0.75f) : (edgeX - rimThickness * 0.25f);

            glm::vec4 rimCol(0.24f, 0.16f, 0.10f, 0.40f + 0.12f * rand01(hRim >> 4));
            renderer->drawSpriteRGBA(whiteTexture, glm::vec2(rx, sy), glm::vec2(rimThickness, segH + 1.0f), 0.0f, rimCol);

            float edgeLineX = isLeftEdge ? edgeX - 2.0f : edgeX;
            renderer->drawSpriteRGBA(whiteTexture, glm::vec2(edgeLineX, sy), glm::vec2(3.0f, segH + 1.0f), 0.0f, glm::vec4(0.14f, 0.09f, 0.06f, 0.38f));
        }
    };
    drawTrampledRimVertical(yardX, true);
    drawTrampledRimVertical(yardX + yardW, false);

    // Следы колес и сапог через боковые границы двора
    auto drawWheelRuts = [&](float startRutX, float rutY, float rutLen) {
        float trackGap = cellSize * 0.42f;
        float rutWidth = std::max(3.0f, cellSize * 0.06f);

        for (int track = 0; track < 2; ++track) {
            float ty = rutY + static_cast<float>(track) * trackGap;
            renderer->drawSpriteRGBA(whiteTexture, glm::vec2(startRutX, ty), glm::vec2(rutLen, rutWidth), 0.0f, glm::vec4(0.11f, 0.08f, 0.05f, 0.50f));
            renderer->drawSpriteRGBA(whiteTexture, glm::vec2(startRutX, ty - 1.0f), glm::vec2(rutLen, 1.0f), 0.0f, glm::vec4(0.44f, 0.38f, 0.30f, 0.35f));
            renderer->drawSpriteRGBA(whiteTexture, glm::vec2(startRutX, ty + rutWidth), glm::vec2(rutLen, 1.0f), 0.0f, glm::vec4(0.44f, 0.38f, 0.30f, 0.30f));
        }

        int numFootprints = static_cast<int>(rutLen / (cellSize * 0.35f));
        for (int fp = 0; fp < numFootprints; ++fp) {
            float fx = startRutX + static_cast<float>(fp) * (cellSize * 0.35f) + 6.0f;
            float fy = rutY + trackGap * 0.5f + ((fp % 2 == 0) ? -4.0f : 4.0f);
            renderer->drawSpriteRGBA(whiteTexture, glm::vec2(fx, fy), glm::vec2(std::max(3.0f, cellSize * 0.05f), std::max(2.0f, cellSize * 0.035f)), 0.0f, glm::vec4(0.14f, 0.10f, 0.07f, 0.40f));
        }
    };

    float midYardY = yardY + yardH * 0.5f - cellSize * 0.25f;
    drawWheelRuts(yardX - cellSize * 1.8f, midYardY, cellSize * 2.2f);
    drawWheelRuts(yardX + yardW - cellSize * 0.4f, midYardY, cellSize * 2.2f);

    // =========================================================================
    // 4. ОБЪЕМНЫЙ 2.5D ЗАБОР (СВЕРХУ И СНИЗУ РАБОЧЕГО ДВОРА)
    // =========================================================================
    auto drawFence25D = [&](float groundY, bool isTop) {
        float fenceH = cellSize * 0.46f;
        float postSpacing = cellSize * 2.0f;
        float postW = std::max(11.0f, cellSize * 0.20f);
        float postH = fenceH + std::max(4.0f, cellSize * 0.07f);
        float postCapH = std::max(4.0f, cellSize * 0.08f);

        float fenceStartX = yardX;
        float fenceEndX = yardX + yardW;
        float fenceLen = yardW;
        float facadeTopY = groundY - fenceH;

        // 4.1. Мягкая падающая тень на земле
        float shadowH = std::max(10.0f, cellSize * 0.20f);
        renderer->drawSpriteRGBA(
            whiteTexture,
            glm::vec2(fenceStartX - postW * 0.5f, groundY),
            glm::vec2(fenceLen + postW, shadowH),
            0.0f,
            glm::vec4(0.0f, 0.0f, 0.0f, isTop ? 0.42f : 0.36f)
        );
        // Темный контактный кант
        renderer->drawSpriteRGBA(
            whiteTexture,
            glm::vec2(fenceStartX - postW * 0.5f, groundY),
            glm::vec2(fenceLen + postW, 2.0f),
            0.0f,
            glm::vec4(0.0f, 0.0f, 0.0f, 0.58f)
        );

        // 4.2. Каменный/бетонный цоколь под пролетами
        float plinthH = std::max(4.0f, cellSize * 0.08f);
        float plinthY = groundY - plinthH;
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(fenceStartX - postW * 0.5f, plinthY),
            glm::vec2(fenceLen + postW, plinthH),
            0.0f,
            glm::vec3(0.38f, 0.37f, 0.35f)
        );
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(fenceStartX - postW * 0.5f, plinthY),
            glm::vec2(fenceLen + postW, 1.5f),
            0.0f,
            glm::vec3(0.52f, 0.51f, 0.48f)
        );
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(fenceStartX - postW * 0.5f, groundY - 1.5f),
            glm::vec2(fenceLen + postW, 1.5f),
            0.0f,
            glm::vec3(0.20f, 0.19f, 0.18f)
        );

        // 4.3. Пролеты забора: вертикальный штакетник и горизонтальные балки-слеги
        int numIntervals = static_cast<int>(std::ceil(fenceLen / postSpacing));
        if (numIntervals < 1) numIntervals = 1;

        float railH = std::max(4.0f, cellSize * 0.09f);
        float topRailY = facadeTopY + fenceH * 0.20f;
        float botRailY = facadeTopY + fenceH * 0.65f;

        float plankW = std::max(5.0f, cellSize * 0.09f);
        float plankGap = std::max(3.0f, cellSize * 0.05f);
        float plankH = fenceH * 0.82f;
        float plankTopY = facadeTopY + fenceH * 0.08f;

        // Вертикальный штакетник между столбами
        for (int p = 0; p < numIntervals; ++p) {
            float p1 = fenceStartX + static_cast<float>(p) * postSpacing;
            float p2 = fenceStartX + static_cast<float>(p + 1) * postSpacing;
            if (p2 > fenceEndX) p2 = fenceEndX;
            if (p1 >= p2) break;

            float spanStartX = p1 + postW * 0.5f;
            float spanEndX = p2 - postW * 0.5f;
            float spanW = spanEndX - spanStartX;
            if (spanW <= 0.0f) continue;

            int numPlanks = static_cast<int>(spanW / (plankW + plankGap));
            float actualGap = (numPlanks > 1) ? ((spanW - numPlanks * plankW) / static_cast<float>(numPlanks)) : plankGap;

            for (int k = 0; k < numPlanks; ++k) {
                float pkX = spanStartX + actualGap * 0.5f + static_cast<float>(k) * (plankW + actualGap);

                uint32_t pHash = hash2D(static_cast<int>(p1), k, 38911u);
                float hueVar = 0.90f + 0.20f * rand01(pHash);
                glm::vec3 woodCol = glm::vec3(0.50f, 0.33f, 0.18f) * hueVar;

                // Падающая микротень справа от доски
                renderer->drawSpriteRGBA(
                    whiteTexture,
                    glm::vec2(pkX + plankW, plankTopY + 1.0f),
                    glm::vec2(1.5f, plankH - 1.0f),
                    0.0f,
                    glm::vec4(0.0f, 0.0f, 0.0f, 0.28f)
                );
                // Тело доски
                renderer->drawSprite(
                    whiteTexture,
                    glm::vec2(pkX, plankTopY),
                    glm::vec2(plankW, plankH),
                    0.0f,
                    woodCol
                );
                // Светлая фаска/верх доски
                renderer->drawSprite(
                    whiteTexture,
                    glm::vec2(pkX, plankTopY),
                    glm::vec2(plankW, 1.5f),
                    0.0f,
                    woodCol + glm::vec3(0.14f, 0.12f, 0.08f)
                );
                // Темный низ доски
                renderer->drawSprite(
                    whiteTexture,
                    glm::vec2(pkX, plankTopY + plankH - 1.5f),
                    glm::vec2(plankW, 1.5f),
                    0.0f,
                    woodCol * 0.60f
                );
            }
        }

        // Две горизонтальные толстые деревянные балки-слеги (светлая верхняя фаска 1-2px, темный низ)
        auto drawRail = [&](float ry) {
            renderer->drawSprite(
                whiteTexture,
                glm::vec2(fenceStartX - postW * 0.5f, ry),
                glm::vec2(fenceLen + postW, railH),
                0.0f,
                glm::vec3(0.42f, 0.28f, 0.16f)
            );
            renderer->drawSprite(
                whiteTexture,
                glm::vec2(fenceStartX - postW * 0.5f, ry),
                glm::vec2(fenceLen + postW, 1.5f),
                0.0f,
                glm::vec3(0.64f, 0.46f, 0.28f)
            );
            renderer->drawSprite(
                whiteTexture,
                glm::vec2(fenceStartX - postW * 0.5f, ry + railH - 1.5f),
                glm::vec2(fenceLen + postW, 1.5f),
                0.0f,
                glm::vec3(0.20f, 0.13f, 0.07f)
            );
        };
        drawRail(topRailY);
        drawRail(botRailY);

        // 4.4. Массивные столбы (каждые 2 тайла)
        auto drawSinglePost = [&](float px) {
            float postX = px - postW * 0.5f;
            float postTopY = groundY - postH;

            // Тень под столбом
            renderer->drawSpriteRGBA(
                whiteTexture,
                glm::vec2(postX - 2.0f, groundY),
                glm::vec2(postW + 6.0f, shadowH * 1.35f),
                0.0f,
                glm::vec4(0.0f, 0.0f, 0.0f, 0.45f)
            );

            // Разделение на освещенную (слева) и теневую (справа) сторону
            float halfW = postW * 0.5f;
            renderer->drawSprite(
                whiteTexture,
                glm::vec2(postX, postTopY),
                glm::vec2(halfW, postH),
                0.0f,
                glm::vec3(0.58f, 0.56f, 0.52f)
            );
            renderer->drawSprite(
                whiteTexture,
                glm::vec2(postX, postTopY),
                glm::vec2(1.5f, postH),
                0.0f,
                glm::vec3(0.74f, 0.72f, 0.68f)
            );
            renderer->drawSprite(
                whiteTexture,
                glm::vec2(postX + halfW, postTopY),
                glm::vec2(halfW, postH),
                0.0f,
                glm::vec3(0.36f, 0.35f, 0.33f)
            );
            renderer->drawSprite(
                whiteTexture,
                glm::vec2(postX + postW - 1.5f, postTopY),
                glm::vec2(1.5f, postH),
                0.0f,
                glm::vec3(0.22f, 0.21f, 0.20f)
            );

            // Верхняя горизонтальная грань / шапка столба (более светлая, объемная фаска)
            float capW = postW + 4.0f;
            float capX = px - capW * 0.5f;
            float capY = postTopY - postCapH;

            renderer->drawSprite(
                whiteTexture,
                glm::vec2(capX, capY),
                glm::vec2(capW, postCapH),
                0.0f,
                glm::vec3(0.68f, 0.66f, 0.62f)
            );
            renderer->drawSprite(
                whiteTexture,
                glm::vec2(capX, capY),
                glm::vec2(capW, 2.0f),
                0.0f,
                glm::vec3(0.84f, 0.82f, 0.78f)
            );
            renderer->drawSprite(
                whiteTexture,
                glm::vec2(capX, capY + postCapH - 1.5f),
                glm::vec2(capW, 1.5f),
                0.0f,
                glm::vec3(0.26f, 0.25f, 0.24f)
            );

            // Болты/заклепки на стыках со столбами
            auto drawRivetPlate = [&](float ry) {
                float plateW = std::max(6.0f, postW * 0.42f);
                float plateH = railH + 2.0f;
                // Левая монтажная пластина
                renderer->drawSprite(whiteTexture, glm::vec2(postX - plateW * 0.5f, ry - 1.0f), glm::vec2(plateW, plateH), 0.0f, glm::vec3(0.28f, 0.30f, 0.32f));
                renderer->drawSprite(whiteTexture, glm::vec2(postX - plateW * 0.5f + 1.5f, ry + 1.5f), glm::vec2(2.5f, 2.5f), 0.0f, glm::vec3(0.82f, 0.84f, 0.88f));
                // Правая монтажная пластина
                renderer->drawSprite(whiteTexture, glm::vec2(postX + postW - plateW * 0.5f, ry - 1.0f), glm::vec2(plateW, plateH), 0.0f, glm::vec3(0.26f, 0.27f, 0.29f));
                renderer->drawSprite(whiteTexture, glm::vec2(postX + postW - plateW * 0.5f + 1.5f, ry + 1.5f), glm::vec2(2.5f, 2.5f), 0.0f, glm::vec3(0.80f, 0.82f, 0.85f));
            };
            drawRivetPlate(topRailY);
            drawRivetPlate(botRailY);
        };

        for (int p = 0; p <= numIntervals; ++p) {
            float px = fenceStartX + static_cast<float>(p) * postSpacing;
            if (px > fenceEndX) px = fenceEndX;
            drawSinglePost(px);
            if (px >= fenceEndX) break;
        }
    };

    // Верхний забор: основание на верхней границе двора (Y = yardY)
    drawFence25D(yardY, true);
    // Нижний забор: фасад расположен под нижней границей двора (Y = yardY + yardH + fenceH)
    drawFence25D(yardY + yardH + cellSize * 0.46f, false);
}


