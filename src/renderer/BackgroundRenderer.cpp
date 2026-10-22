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

    // Расчет центральных 3 клеток по вертикали (Y)
    int startTileY = std::max(0, (gridH_tiles - 3) / 2);
    float entranceY = gridOffset.y + static_cast<float>(startTileY) * cellSize;
    float entranceH = 3.0f * cellSize;

    // Сдвиг боковых карманов внутрь валидной сетки (0 .. W-1):
    // Первые 3 столбца (0, 1, 2) - настил спавнеров, последние 3 столбца (W-3, W-2, W-1) - здание и портал
    float yardX = gridOffset.x;
    float yardW = gridW;

    // 2. СВЕРХУ И СНИЗУ: ИНДУСТРИАЛЬНЫЙ ЗАБОР (вдоль центральной зоны двора x = 3 .. W-4)
    float fenceH = std::max(22.0f, cellSize * 0.40f);
    float postSpacing = cellSize * 1.25f;
    float postW = std::max(7.0f, cellSize * 0.13f);

    auto drawFenceSection = [&](float fenceY, bool isTop) {
        // Тень от забора на центральный двор
        float shadowH = std::max(12.0f, cellSize * 0.22f);
        float shadowY = isTop ? gridOffset.y : (gridOffset.y + gridH - shadowH);
        renderer->drawSpriteRGBA(
            whiteTexture,
            glm::vec2(yardX, shadowY),
            glm::vec2(yardW, shadowH),
            0.0f,
            glm::vec4(0.0f, 0.0f, 0.0f, isTop ? 0.42f : 0.35f)
        );

        // Основное полотно забора: ржавые металлические секции
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(yardX - postW * 0.5f, fenceY),
            glm::vec2(yardW + postW, fenceH),
            0.0f,
            glm::vec3(0.32f, 0.21f, 0.15f)
        );

        // Горизонтальные ребра жесткости (верхняя и нижняя перекладины)
        float railThick = std::max(2.5f, fenceH * 0.14f);
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(yardX - postW * 0.5f, fenceY + 1.0f),
            glm::vec2(yardW + postW, railThick),
            0.0f,
            glm::vec3(0.20f, 0.18f, 0.17f)
        );
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(yardX - postW * 0.5f, fenceY + fenceH - railThick - 1.0f),
            glm::vec2(yardW + postW, railThick),
            0.0f,
            glm::vec3(0.20f, 0.18f, 0.17f)
        );

        // Бетонные столбики с колпаками
        int numPosts = static_cast<int>(yardW / postSpacing) + 2;
        for (int p = 0; p < numPosts; ++p) {
            float px = yardX + static_cast<float>(p) * postSpacing;
            if (px > yardX + yardW + postW * 0.5f) break;

            // Тело столбика (бетон)
            renderer->drawSprite(
                whiteTexture,
                glm::vec2(px - postW * 0.5f, fenceY - 2.0f),
                glm::vec2(postW, fenceH + 4.0f),
                0.0f,
                glm::vec3(0.48f, 0.47f, 0.45f)
            );
            // Темный теневой кант столбика
            renderer->drawSprite(
                whiteTexture,
                glm::vec2(px - postW * 0.5f, fenceY - 2.0f),
                glm::vec2(2.0f, fenceH + 4.0f),
                0.0f,
                glm::vec3(0.28f, 0.27f, 0.26f)
            );
            // Колпак столбика
            renderer->drawSprite(
                whiteTexture,
                glm::vec2(px - postW * 0.7f, fenceY - 4.0f),
                glm::vec2(postW * 1.4f, 3.5f),
                0.0f,
                glm::vec3(0.60f, 0.59f, 0.56f)
            );
        }
    };

    drawFenceSection(gridOffset.y - fenceH, true);
    drawFenceSection(gridOffset.y + gridH, false);

    // 3. СПРАВА: МАССИВНОЕ БЕТОННОЕ ЗДАНИЕ ШАХТНОГО КОПРА + ВХОД В ШАХТУ (3 КЛЕТКИ В ЦЕНТРЕ)
    // Начинается с колонки W-3 и уходит вправо за экран
    float bldX = yardX + yardW;
    float bldW = endX - bldX;

    // 3.1. Тень от западной стены здания на игровое поле
    float bldShadowW = std::max(20.0f, cellSize * 0.38f);
    renderer->drawSpriteRGBA(
        whiteTexture,
        glm::vec2(bldX - bldShadowW, startY),
        glm::vec2(bldShadowW, totalH),
        0.0f,
        glm::vec4(0.0f, 0.0f, 0.0f, 0.48f)
    );

    // 3.2. Верхний и нижний корпуса здания копра (покрывают всё здание выше и ниже проёма)
    auto drawRoofSection = [&](float topY, float h) {
        if (h <= 0.0f) return;
        // Базовый бетон крыши копра
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(bldX, topY),
            glm::vec2(bldW, h),
            0.0f,
            glm::vec3(0.26f, 0.28f, 0.30f)
        );

        // Продольные и поперечные швы бетонных плит
        float slabSize = std::max(48.0f, cellSize * 1.25f);
        int numSlabsY = static_cast<int>(h / slabSize) + 1;
        for (int sy = 0; sy < numSlabsY; ++sy) {
            float yPos = topY + static_cast<float>(sy) * slabSize;
            renderer->drawSprite(
                whiteTexture,
                glm::vec2(bldX, yPos),
                glm::vec2(bldW, 2.0f),
                0.0f,
                glm::vec3(0.14f, 0.15f, 0.16f)
            );
        }

        int numSlabsX = static_cast<int>(bldW / slabSize) + 1;
        for (int sx = 0; sx < numSlabsX; ++sx) {
            float xPos = bldX + static_cast<float>(sx) * slabSize;
            renderer->drawSprite(
                whiteTexture,
                glm::vec2(xPos, topY),
                glm::vec2(2.0f, h),
                0.0f,
                glm::vec3(0.14f, 0.15f, 0.16f)
            );
        }

        // Индустриальные металлические конструкции: X-образные стальные фермы на крыше
        float trussH = std::min(h - 20.0f, cellSize * 1.5f);
        if (trussH > 24.0f) {
            float trussY = topY + 12.0f;
            float trussW = std::min(bldW - 30.0f, cellSize * 2.2f);
            float trussX = bldX + cellSize * 0.4f;

            // Контур фермы
            glm::vec3 steelCol(0.38f, 0.40f, 0.44f);
            renderer->drawSprite(whiteTexture, glm::vec2(trussX, trussY), glm::vec2(trussW, 3.0f), 0.0f, steelCol);
            renderer->drawSprite(whiteTexture, glm::vec2(trussX, trussY + trussH), glm::vec2(trussW, 3.0f), 0.0f, steelCol);
            renderer->drawSprite(whiteTexture, glm::vec2(trussX, trussY), glm::vec2(3.0f, trussH), 0.0f, steelCol);
            renderer->drawSprite(whiteTexture, glm::vec2(trussX + trussW, trussY), glm::vec2(3.0f, trussH), 0.0f, steelCol);

            // Диагональные раскосы (X)
            float diagSteps = 3.0f;
            float stepW = trussW / diagSteps;
            for (int s = 0; s < 3; ++s) {
                float sx0 = trussX + static_cast<float>(s) * stepW;
                // Стилизованный крест фермы
                renderer->drawSprite(whiteTexture, glm::vec2(sx0 + stepW * 0.5f - 1.5f, trussY), glm::vec2(3.0f, trussH), 0.0f, steelCol * 0.85f);
            }
        }

        // Вентиляционный короб / вытяжной дефлектор копра
        float ventSize = std::max(28.0f, cellSize * 0.65f);
        if (h > ventSize + 20.0f) {
            float ventX = bldX + cellSize * 1.2f;
            float ventY = topY + h * 0.5f - ventSize * 0.5f;

            // Корпус короба
            renderer->drawSprite(whiteTexture, glm::vec2(ventX, ventY), glm::vec2(ventSize, ventSize), 0.0f, glm::vec3(0.18f, 0.20f, 0.22f));
            // Решетка жалюзи вентиляции
            float louverGap = ventSize / 4.0f;
            for (int l = 1; l < 4; ++l) {
                renderer->drawSprite(whiteTexture, glm::vec2(ventX + 2.0f, ventY + static_cast<float>(l) * louverGap), glm::vec2(ventSize - 4.0f, 2.0f), 0.0f, glm::vec3(0.32f, 0.35f, 0.38f));
            }
        }
    };

    drawRoofSection(startY, entranceY - startY);
    drawRoofSection(entranceY + entranceH, endY - (entranceY + entranceH));

    // 3.3. ВХОД В ШАХТУ (3 КЛЕТКИ ПО Y В ЦЕНТРЕ): ТЕМНЫЙ ПРОЕМ СТВОЛА, НАСТИЛ, РЕЛЬСЫ И РАЗМЕТКА
    float portalDepth = std::max(120.0f, cellSize * 2.4f);

    // Глубокий темный тоннель ствола шахты
    renderer->drawSprite(
        whiteTexture,
        glm::vec2(bldX, entranceY),
        glm::vec2(portalDepth, entranceH),
        0.0f,
        glm::vec3(0.02f, 0.02f, 0.03f)
    );

    // Крыша здания позади проёма ствола
    if (bldW > portalDepth) {
        drawRoofSection(entranceY, entranceH);
    }

    // Деревянно-металлический настил внутри тоннеля
    float plankW = std::max(8.0f, cellSize * 0.16f);
    int numPlanks = static_cast<int>(portalDepth / plankW);
    for (int p = 0; p < numPlanks; ++p) {
        float px = bldX + static_cast<float>(p) * plankW;
        glm::vec3 pCol = (p % 2 == 0) ? glm::vec3(0.36f, 0.24f, 0.15f) : glm::vec3(0.32f, 0.21f, 0.13f);
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(px, entranceY + 4.0f),
            glm::vec2(plankW - 1.5f, entranceH - 8.0f),
            0.0f,
            pCol
        );
    }

    // Рельсовые пути, уходящие в темноту шахтного ствола
    float railGauge = cellSize * 0.60f;
    float railThick = std::max(2.5f, cellSize * 0.07f);
    float railTopY = entranceY + (entranceH - railGauge) * 0.5f;
    float railBotY = railTopY + railGauge - railThick;

    // Рельсовые нити
    renderer->drawSprite(whiteTexture, glm::vec2(bldX, railTopY), glm::vec2(portalDepth, railThick), 0.0f, glm::vec3(0.78f, 0.80f, 0.86f));
    renderer->drawSprite(whiteTexture, glm::vec2(bldX, railBotY), glm::vec2(portalDepth, railThick), 0.0f, glm::vec3(0.78f, 0.80f, 0.86f));

    // Тёплый свет шахтерских фонарей из глубины тоннеля (градиентное свечение)
    renderer->drawSpriteRGBA(
        whiteTexture,
        glm::vec2(bldX + portalDepth * 0.35f, entranceY + 6.0f),
        glm::vec2(portalDepth * 0.65f, entranceH - 12.0f),
        0.0f,
        glm::vec4(1.0f, 0.62f, 0.18f, 0.22f)
    );

    // Массивные деревянные и стальные подпорные рамы шахтной крепи портала
    float frameThick = std::max(6.0f, cellSize * 0.12f);
    glm::vec3 timberCol(0.28f, 0.18f, 0.11f);
    // Верхняя балка крепи
    renderer->drawSprite(whiteTexture, glm::vec2(bldX, entranceY), glm::vec2(portalDepth, frameThick), 0.0f, timberCol);
    // Нижняя балка крепи
    renderer->drawSprite(whiteTexture, glm::vec2(bldX, entranceY + entranceH - frameThick), glm::vec2(portalDepth, frameThick), 0.0f, timberCol);

    // Вертикальные стойки крепи (арки) через каждые 0.8 * cellSize
    int numFrames = static_cast<int>(portalDepth / (cellSize * 0.8f)) + 1;
    for (int f = 0; f < numFrames; ++f) {
        float fx = bldX + static_cast<float>(f) * (cellSize * 0.8f);
        renderer->drawSprite(whiteTexture, glm::vec2(fx, entranceY), glm::vec2(frameThick, entranceH), 0.0f, timberCol);
        // Металлические угловые накладки на стойках
        renderer->drawSprite(whiteTexture, glm::vec2(fx, entranceY), glm::vec2(frameThick * 1.5f, frameThick * 1.5f), 0.0f, glm::vec3(0.40f, 0.42f, 0.45f));
        renderer->drawSprite(whiteTexture, glm::vec2(fx, entranceY + entranceH - frameThick * 1.5f), glm::vec2(frameThick * 1.5f, frameThick * 1.5f), 0.0f, glm::vec3(0.40f, 0.42f, 0.45f));
    }

    // ПОРОГ ВХОДА В ШАХТУ: ЖЕЛТО-ЧЕРНАЯ СИГНАЛЬНАЯ РАЗМЕТКА (по всей высоте входа в 3 клетки)
    float stripeW = std::max(10.0f, cellSize * 0.18f);
    float stripeSegH = std::max(12.0f, cellSize * 0.22f);
    int numStripes = static_cast<int>(entranceH / stripeSegH) + 1;
    for (int s = 0; s < numStripes; ++s) {
        float sy = entranceY + static_cast<float>(s) * stripeSegH;
        float sh = std::min(stripeSegH, entranceY + entranceH - sy);
        if (sh <= 0.0f) break;

        glm::vec3 sCol = (s % 2 == 0) ? glm::vec3(0.92f, 0.78f, 0.10f) : glm::vec3(0.12f, 0.12f, 0.14f);
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(bldX - stripeW * 0.5f, sy),
            glm::vec2(stripeW, sh),
            0.0f,
            sCol
        );
    }

    // Шахтные сигнальные фонари по краям портала
    auto drawMineLamp = [&](float ly) {
        renderer->drawSprite(whiteTexture, glm::vec2(bldX - 6.0f, ly - 5.0f), glm::vec2(10.0f, 10.0f), 0.0f, glm::vec3(0.18f, 0.18f, 0.20f));
        renderer->drawSprite(whiteTexture, glm::vec2(bldX - 4.0f, ly - 3.0f), glm::vec2(6.0f, 6.0f), 0.0f, glm::vec3(1.0f, 0.85f, 0.25f));
        renderer->drawSpriteRGBA(whiteTexture, glm::vec2(bldX - 16.0f, ly - 15.0f), glm::vec2(30.0f, 30.0f), 0.0f, glm::vec4(1.0f, 0.80f, 0.20f, 0.25f));
    };
    drawMineLamp(entranceY + frameThick * 1.5f);
    drawMineLamp(entranceY + entranceH - frameThick * 1.5f);

    // 4. СЛЕВА: ЗОНА СПАВНЕРОВ С НАСТИЛОМ В 3 КЛЕТКИ (НАПРОТИВ ВХОДА)
    // Охватывает колонки 0, 1, 2 и уходит влево за экран
    float leftX = yardX;

    // 4.1. Сверху и снизу от зоны спавнеров: отвалы пустой породы (терриконы) и забор
    auto drawWasteRockHeap = [&](float topY, float h) {
        if (h <= 0.0f) return;
        // Каменистая насыпь породы
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(startX, topY),
            glm::vec2(leftX - startX, h),
            0.0f,
            glm::vec3(0.24f, 0.18f, 0.13f)
        );

        // Крупные каменные глыбы и обломки породы
        int numRocks = static_cast<int>(h / (cellSize * 0.7f)) + 1;
        for (int r = 0; r < numRocks; ++r) {
            float ry = topY + static_cast<float>(r) * (cellSize * 0.7f) + 8.0f;
            float rx = leftX - cellSize * 0.8f - static_cast<float>((r * 17) % 40);
            float rSz = std::max(16.0f, cellSize * 0.35f);
            renderer->drawSprite(
                whiteTexture,
                glm::vec2(rx, ry),
                glm::vec2(rSz, rSz * 0.85f),
                static_cast<float>((r * 25) % 60),
                glm::vec3(0.36f, 0.28f, 0.20f)
            );
        }

        // Забор по левой кромке
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(leftX - postW, topY),
            glm::vec2(postW, h),
            0.0f,
            glm::vec3(0.42f, 0.40f, 0.38f)
        );
    };

    drawWasteRockHeap(startY, entranceY - startY);
    drawWasteRockHeap(entranceY + entranceH, endY - (entranceY + entranceH));

    // 4.2. Зона спавнеров: 3 клетки по центру с деревянным настилом и колеями, уходящими в туман влево
    float spawnerRoadW = leftX - startX;
    renderer->drawSprite(
        whiteTexture,
        glm::vec2(startX, entranceY),
        glm::vec2(spawnerRoadW, entranceH),
        0.0f,
        glm::vec3(0.28f, 0.18f, 0.12f)
    );

    // Дощатый настил зоны спавна
    int numSpawnPlanks = static_cast<int>(spawnerRoadW / plankW);
    for (int p = 0; p < numSpawnPlanks; ++p) {
        float px = startX + static_cast<float>(p) * plankW;
        glm::vec3 pCol = (p % 2 == 0) ? glm::vec3(0.40f, 0.27f, 0.17f) : glm::vec3(0.35f, 0.23f, 0.14f);
        renderer->drawSprite(
            whiteTexture,
            glm::vec2(px, entranceY + 2.0f),
            glm::vec2(plankW - 1.5f, entranceH - 4.0f),
            0.0f,
            pCol
        );
    }

    // Накатанные колеи тележек / вагонеток
    renderer->drawSprite(whiteTexture, glm::vec2(startX, railTopY - 2.0f), glm::vec2(spawnerRoadW, 8.0f), 0.0f, glm::vec3(0.20f, 0.13f, 0.08f));
    renderer->drawSprite(whiteTexture, glm::vec2(startX, railBotY - 2.0f), glm::vec2(spawnerRoadW, 8.0f), 0.0f, glm::vec3(0.20f, 0.13f, 0.08f));

    // Клубящийся туман / пыль над зоной выхода врагов (уходит в туман влево)
    renderer->drawSpriteRGBA(
        whiteTexture,
        glm::vec2(startX, entranceY - 10.0f),
        glm::vec2(spawnerRoadW * 0.65f, entranceH + 20.0f),
        0.0f,
        glm::vec4(0.42f, 0.46f, 0.50f, 0.38f)
    );
    renderer->drawSpriteRGBA(
        whiteTexture,
        glm::vec2(startX, entranceY + 10.0f),
        glm::vec2(spawnerRoadW * 0.40f, entranceH - 20.0f),
        0.0f,
        glm::vec4(0.46f, 0.50f, 0.55f, 0.45f)
    );
}


