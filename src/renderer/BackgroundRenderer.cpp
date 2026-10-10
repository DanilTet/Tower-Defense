#include "BackgroundRenderer.h"
#include "SpriteRenderer.h"
#include "../textures/Texture2D.h"
#include "../game/world/Grid.h"
#include <algorithm>
#include <cmath>

void BackgroundRenderer::render(
    SpriteRenderer* renderer,
    const std::shared_ptr<Texture2D>& whiteTexture,
    const std::string& backgroundType,
    const Grid* grid,
    float screenWidth,
    float screenHeight
) {
    if (!renderer || !whiteTexture) return;

    if (backgroundType == "pipes_canal") {
        renderPipesCanal(renderer, whiteTexture, grid, screenWidth, screenHeight);
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

void BackgroundRenderer::renderPipesCanal(
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

    if (!grid) return;

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

