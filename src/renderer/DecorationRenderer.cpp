#include "DecorationRenderer.h"
#include "SpriteRenderer.h"
#include "../textures/Texture2D.h"
#include "../resources/ResourceManager.h"
#include "../game/world/Grid.h"
#include <GLFW/glfw3.h>
#include <cmath>
#include <algorithm>

namespace {

void drawPart(
    SpriteRenderer* renderer,
    const std::shared_ptr<Texture2D>& texture,
    glm::vec2 decorPos,
    float decorRot,
    glm::vec2 decorScale,
    glm::vec2 localOffset,
    glm::vec2 localSize,
    float localAngle,
    glm::vec4 color
) {
    if (!renderer || !texture) return;

    float rad = glm::radians(decorRot);
    float cosR = std::cos(rad);
    float sinR = std::sin(rad);

    glm::vec2 scaledOffset = localOffset * decorScale;
    glm::vec2 rotOffset(
        scaledOffset.x * cosR - scaledOffset.y * sinR,
        scaledOffset.x * sinR + scaledOffset.y * cosR
    );

    glm::vec2 finalCenter = decorPos + rotOffset;
    glm::vec2 finalSize = localSize * decorScale;
    glm::vec2 drawPos = finalCenter - finalSize * 0.5f;

    renderer->drawSpriteRGBA(texture, drawPos, finalSize, localAngle + decorRot, color);
}

void drawBush(SpriteRenderer* renderer, const std::shared_ptr<Texture2D>& tex, const DecorationConfig& d, float a) {
    glm::vec2 sVec(d.scale * d.scaleX, d.scale * d.scaleY);

    // 1. Темно-зеленый базис кроны (круглые перекрывающиеся объемы для зубчатого пушистого края)
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 0.0f }, { 44.0f, 42.0f }, 0.0f, glm::vec4(0.09f, 0.28f, 0.12f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -14.0f, -11.0f }, { 24.0f, 23.0f }, 15.0f, glm::vec4(0.10f, 0.30f, 0.13f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 13.0f, -12.0f }, { 23.0f, 22.0f }, -20.0f, glm::vec4(0.11f, 0.32f, 0.14f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 14.0f, 10.0f }, { 25.0f, 24.0f }, 25.0f, glm::vec4(0.09f, 0.27f, 0.11f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -13.0f, 11.0f }, { 24.0f, 23.0f }, -15.0f, glm::vec4(0.08f, 0.26f, 0.11f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -17.0f, 0.0f }, { 21.0f, 21.0f }, 5.0f, glm::vec4(0.10f, 0.30f, 0.13f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 17.0f, -1.0f }, { 21.0f, 21.0f }, -10.0f, glm::vec4(0.11f, 0.32f, 0.14f, a));

    // 2. Средне-зеленый объем листвы
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -5.0f, -5.0f }, { 28.0f, 26.0f }, 10.0f, glm::vec4(0.18f, 0.52f, 0.22f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 6.0f, -3.0f }, { 27.0f, 26.0f }, -15.0f, glm::vec4(0.20f, 0.56f, 0.24f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -2.0f, 6.0f }, { 26.0f, 24.0f }, 20.0f, glm::vec4(0.17f, 0.50f, 0.21f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 5.0f, 5.0f }, { 24.0f, 23.0f }, -5.0f, glm::vec4(0.19f, 0.54f, 0.23f, a));

    // 3. Салатовые блики по центру для объемного купола сверху
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -2.0f, -3.0f }, { 19.0f, 18.0f }, 5.0f, glm::vec4(0.34f, 0.74f, 0.30f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 4.0f, -1.0f }, { 16.0f, 15.0f }, -10.0f, glm::vec4(0.38f, 0.78f, 0.32f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, -2.0f }, { 10.0f, 9.0f }, 0.0f, glm::vec4(0.48f, 0.88f, 0.38f, 0.95f * a));
}

struct GrassBladeDesc {
    float x;
    float y;
    float height;
    float width;
    float phase;
};

void drawGrassTuft(SpriteRenderer* renderer, const std::shared_ptr<Texture2D>& tex, const DecorationConfig& d, float a) {
    glm::vec2 sVec(d.scale * d.scaleX, d.scale * d.scaleY);
    float time = static_cast<float>(glfwGetTime());

    // 1. Небольшие мягкие корневые тени под скоплениями травинок
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 4.0f }, { 36.0f, 14.0f }, 0.0f, glm::vec4(0.05f, 0.10f, 0.05f, 0.25f * a));

    // Набор вертикальных травинок (18-22 шт.), хаотично распределенных по пятну ~40-50px
    static const GrassBladeDesc s_blades[] = {
        { -19.0f,   4.0f, 13.0f, 1.8f, 0.1f },
        { -15.0f,   8.0f, 16.0f, 1.9f, 0.8f },
        { -12.0f,  -2.0f, 18.0f, 1.8f, 1.4f },
        { -10.0f,   6.0f, 14.0f, 1.7f, 2.1f },
        {  -7.0f,   1.0f, 21.0f, 2.0f, 0.5f },
        {  -5.0f,  -6.0f, 15.0f, 1.8f, 2.9f },
        {  -3.0f,   8.0f, 19.0f, 1.9f, 1.1f },
        {  -1.0f,   2.0f, 22.0f, 2.0f, 0.2f },
        {   1.0f,  -4.0f, 17.0f, 1.8f, 1.7f },
        {   3.0f,   7.0f, 20.0f, 1.9f, 2.5f },
        {   6.0f,   0.0f, 23.0f, 2.0f, 0.9f },
        {   8.0f,  -5.0f, 14.0f, 1.7f, 3.2f },
        {  11.0f,   6.0f, 18.0f, 1.9f, 1.5f },
        {  13.0f,  -2.0f, 21.0f, 2.0f, 0.4f },
        {  15.0f,   5.0f, 15.0f, 1.8f, 2.3f },
        {  18.0f,  -1.0f, 17.0f, 1.8f, 1.9f },
        {  20.0f,   6.0f, 12.0f, 1.7f, 0.7f },
        {  -9.0f,  -8.0f, 13.0f, 1.7f, 1.8f },
        {   0.0f, -10.0f, 14.0f, 1.8f, 0.6f },
        {   9.0f,  -9.0f, 13.0f, 1.7f, 2.7f }
    };

    for (const auto& b : s_blades) {
        // Колыхание верхушки от ветра (основание зафиксировано на земле)
        float sway = std::sin(time * 2.2f + b.x * 0.05f + b.phase) * (b.height * 0.15f);
        float tiltAngleDeg = glm::degrees(std::atan2(sway, b.height));

        // Центр основного тела травинки
        glm::vec2 bodyCenter(b.x + sway * 0.5f, b.y - b.height * 0.5f);
        drawPart(renderer, tex, d.worldPos, d.rotation, sVec,
                 bodyCenter, { b.width, b.height }, tiltAngleDeg,
                 glm::vec4(0.22f, 0.68f, 0.28f, 0.70f * a));

        // Светлый кончик травинки (верхние 30% высоты)
        float tipH = b.height * 0.32f;
        glm::vec2 tipCenter(b.x + sway * 0.84f, b.y - b.height + tipH * 0.5f);
        drawPart(renderer, tex, d.worldPos, d.rotation, sVec,
                 tipCenter, { b.width * 0.9f, tipH }, tiltAngleDeg,
                 glm::vec4(0.40f, 0.86f, 0.35f, 0.78f * a));
    }
}

void drawGrassField(SpriteRenderer* renderer, const std::shared_ptr<Texture2D>& tex, const DecorationConfig& d, float a) {
    glm::vec2 sVec(d.scale * d.scaleX, d.scale * d.scaleY);
    float time = static_cast<float>(glfwGetTime());

    // Плотная россыпь из 48 тонких вертикальных травинок по площади тайла 64x64px
    // Базовая высота травинок уменьшена на 35% (9.0f - 14.0f вместо 14.0f - 22.0f)
    // чтобы они не вылезали за пределы клетки и аккуратно заполняли внутренность тайла.
    static const GrassBladeDesc s_fieldBlades[48] = {
        { -27.0f, -22.0f, 10.0f, 1.4f, 0.2f },
        { -20.0f, -24.0f, 11.5f, 1.5f, 1.1f },
        { -12.0f, -23.0f,  9.5f, 1.3f, 2.4f },
        {  -4.0f, -25.0f, 12.0f, 1.5f, 0.8f },
        {   5.0f, -23.0f, 11.0f, 1.4f, 1.9f },
        {  13.0f, -24.0f, 10.0f, 1.3f, 3.1f },
        {  21.0f, -22.0f, 12.0f, 1.5f, 0.5f },
        {  28.0f, -23.0f,  9.5f, 1.4f, 2.0f },

        { -28.0f, -14.0f, 11.0f, 1.4f, 1.5f },
        { -22.0f, -12.0f, 13.0f, 1.6f, 0.3f },
        { -14.0f, -15.0f, 10.0f, 1.3f, 2.8f },
        {  -6.0f, -13.0f, 13.5f, 1.5f, 1.0f },
        {   2.0f, -14.0f, 11.5f, 1.4f, 2.3f },
        {  11.0f, -12.0f, 13.0f, 1.6f, 0.7f },
        {  19.0f, -15.0f, 11.0f, 1.4f, 2.7f },
        {  27.0f, -13.0f, 12.0f, 1.5f, 1.2f },

        { -26.0f,  -4.0f, 12.0f, 1.5f, 0.6f },
        { -18.0f,  -6.0f, 10.0f, 1.3f, 2.2f },
        { -10.0f,  -3.0f, 14.0f, 1.6f, 1.4f },
        {  -2.0f,  -5.0f, 11.5f, 1.4f, 0.1f },
        {   7.0f,  -4.0f, 13.5f, 1.6f, 2.9f },
        {  15.0f,  -6.0f, 11.0f, 1.4f, 1.8f },
        {  23.0f,  -3.0f, 13.0f, 1.5f, 0.4f },
        {  29.0f,  -5.0f, 10.0f, 1.3f, 2.5f },

        { -27.0f,   5.0f, 11.0f, 1.4f, 2.0f },
        { -21.0f,   3.0f, 13.5f, 1.6f, 0.9f },
        { -13.0f,   6.0f, 11.5f, 1.4f, 3.2f },
        {  -5.0f,   4.0f, 14.5f, 1.6f, 1.6f },
        {   3.0f,   5.0f, 12.0f, 1.5f, 0.2f },
        {  12.0f,   3.0f, 14.0f, 1.6f, 2.6f },
        {  20.0f,   6.0f, 11.0f, 1.4f, 1.1f },
        {  28.0f,   4.0f, 13.0f, 1.5f, 3.0f },

        { -28.0f,  15.0f, 13.0f, 1.5f, 0.7f },
        { -19.0f,  17.0f, 11.0f, 1.4f, 2.5f },
        { -11.0f,  14.0f, 14.0f, 1.6f, 1.3f },
        {  -3.0f,  16.0f, 11.5f, 1.4f, 3.0f },
        {   6.0f,  15.0f, 13.5f, 1.6f, 0.5f },
        {  14.0f,  17.0f, 11.0f, 1.4f, 2.1f },
        {  22.0f,  14.0f, 13.0f, 1.5f, 1.7f },
        {  27.0f,  16.0f, 10.0f, 1.3f, 0.8f },

        { -26.0f,  26.0f, 11.0f, 1.4f, 1.4f },
        { -18.0f,  24.0f, 12.0f, 1.5f, 2.8f },
        { -10.0f,  27.0f, 10.0f, 1.3f, 0.4f },
        {  -1.0f,  25.0f, 13.0f, 1.5f, 1.9f },
        {   8.0f,  26.0f, 11.0f, 1.4f, 3.3f },
        {  16.0f,  24.0f, 13.5f, 1.6f, 1.0f },
        {  24.0f,  27.0f, 10.0f, 1.3f, 2.4f },
        {  29.0f,  25.0f, 11.5f, 1.4f, 0.6f }
    };

    for (const auto& b : s_fieldBlades) {
        float sway = std::sin(time * 2.2f + b.x * 0.05f + b.phase) * (b.height * 0.12f);
        float tiltAngleDeg = glm::degrees(std::atan2(sway, b.height));

        // Основание травинки: более плотное и насыщенное
        glm::vec2 bodyCenter(b.x + sway * 0.5f, b.y - b.height * 0.5f);
        drawPart(renderer, tex, d.worldPos, d.rotation, sVec,
                 bodyCenter, { b.width, b.height }, tiltAngleDeg,
                 glm::vec4(0.20f, 0.62f, 0.24f, 0.85f * a));

        // Мягкий светлый кончик (верхние 30% высоты)
        float tipH = b.height * 0.32f;
        glm::vec2 tipCenter(b.x + sway * 0.84f, b.y - b.height + tipH * 0.5f);
        drawPart(renderer, tex, d.worldPos, d.rotation, sVec,
                 tipCenter, { b.width * 0.85f, tipH }, tiltAngleDeg,
                 glm::vec4(0.44f, 0.90f, 0.38f, 0.92f * a));
    }
}

void drawFlower(SpriteRenderer* renderer, const std::shared_ptr<Texture2D>& tex, const DecorationConfig& d, float a) {
    glm::vec2 sVec(d.scale * d.scaleX, d.scale * d.scaleY);

    // 1. Тень
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 1.5f, 2.0f }, { 24.0f, 20.0f }, 0.0f, glm::vec4(0.08f, 0.12f, 0.08f, 0.35f * a));
    // 2. Листики под цветками
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -8.0f, 4.0f }, { 12.0f, 5.5f }, -30.0f, glm::vec4(0.20f, 0.55f, 0.18f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 8.0f, 4.0f }, { 12.0f, 5.5f }, 30.0f, glm::vec4(0.25f, 0.62f, 0.22f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, -9.0f }, { 5.5f, 12.0f }, 0.0f, glm::vec4(0.22f, 0.58f, 0.20f, a));
    // 3. Лепестки главного цветка (вид сверху, распустившийся бутон)
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -1.0f, 0.0f }, { 16.0f, 8.0f }, 0.0f, glm::vec4(0.95f, 0.25f, 0.38f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -1.0f, 0.0f }, { 8.0f, 16.0f }, 0.0f, glm::vec4(0.95f, 0.25f, 0.38f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -1.0f, 0.0f }, { 14.0f, 7.0f }, 45.0f, glm::vec4(0.98f, 0.35f, 0.48f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -1.0f, 0.0f }, { 7.0f, 14.0f }, 45.0f, glm::vec4(0.98f, 0.35f, 0.48f, a));
    // 4. Золотистая сердцевина
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -1.0f, 0.0f }, { 6.5f, 6.5f }, 0.0f, glm::vec4(1.0f, 0.88f, 0.15f, a));
    // 5. Маленький синий цветок рядом
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 11.0f, -4.0f }, { 10.0f, 5.0f }, 25.0f, glm::vec4(0.35f, 0.65f, 1.0f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 11.0f, -4.0f }, { 5.0f, 10.0f }, 25.0f, glm::vec4(0.35f, 0.65f, 1.0f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 11.0f, -4.0f }, { 4.0f, 4.0f }, 0.0f, glm::vec4(1.0f, 0.90f, 0.20f, a));
}

void drawStone(SpriteRenderer* renderer, const std::shared_ptr<Texture2D>& tex, const DecorationConfig& d, float a) {
    glm::vec2 sVec(d.scale * d.scaleX, d.scale * d.scaleY);

    // 1. Тень
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 2.5f, 3.0f }, { 34.0f, 26.0f }, 0.0f, glm::vec4(0.06f, 0.07f, 0.09f, 0.40f * a));
    // 2. Темная грань (низ-право)
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 1.5f, 1.5f }, { 30.0f, 23.0f }, 5.0f, glm::vec4(0.22f, 0.23f, 0.26f, a));
    // 3. Основное тело камня
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -1.0f, -1.0f }, { 27.0f, 20.0f }, -3.0f, glm::vec4(0.38f, 0.40f, 0.44f, a));
    // 4. Освещенная грань скола (верх-лево)
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -4.0f, -4.0f }, { 18.0f, 13.0f }, -8.0f, glm::vec4(0.52f, 0.54f, 0.58f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -5.0f, -6.0f }, { 11.0f, 7.0f }, -12.0f, glm::vec4(0.68f, 0.70f, 0.74f, a));
    // 5. Мелкий камешек рядом
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -14.0f, 8.0f }, { 9.0f, 7.0f }, 10.0f, glm::vec4(0.40f, 0.42f, 0.46f, a));
}

void drawHelmet(SpriteRenderer* renderer, const std::shared_ptr<Texture2D>& tex, const DecorationConfig& d, float a) {
    glm::vec2 sVec(d.scale * d.scaleX, d.scale * d.scaleY);

    // 1. Тень
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 1.5f, 2.0f }, { 28.0f, 24.0f }, 0.0f, glm::vec4(0.06f, 0.06f, 0.08f, 0.35f * a));
    // 2. Поля каски (овальный ободок)
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 0.0f }, { 28.0f, 24.0f }, 0.0f, glm::vec4(0.82f, 0.58f, 0.05f, a));
    // 3. Купол каски
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 0.0f }, { 22.0f, 19.0f }, 0.0f, glm::vec4(0.98f, 0.78f, 0.08f, a));
    // 4. Продольное ребро жесткости (сверху вдоль каски)
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 0.0f }, { 5.0f, 20.0f }, 0.0f, glm::vec4(1.0f, 0.90f, 0.28f, a));
    // 5. Крепление шахтерского налобного фонаря
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, -11.0f }, { 6.0f, 5.0f }, 0.0f, glm::vec4(0.24f, 0.26f, 0.30f, a));
    // 6. Линза фонаря
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, -13.0f }, { 4.0f, 3.0f }, 0.0f, glm::vec4(0.90f, 0.96f, 1.0f, a));
    // 7. Световой конус фонаря
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, -20.0f }, { 16.0f, 12.0f }, 0.0f, glm::vec4(1.0f, 0.95f, 0.55f, 0.30f * a));
}

void drawPickaxe(SpriteRenderer* renderer, const std::shared_ptr<Texture2D>& tex, const DecorationConfig& d, float a) {
    glm::vec2 sVec(d.scale * d.scaleX, d.scale * d.scaleY);

    // 1. Тень
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 1.5f, 2.0f }, { 3.5f, 42.0f }, 0.0f, glm::vec4(0.06f, 0.06f, 0.08f, 0.35f * a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 1.5f, -12.0f }, { 44.0f, 5.0f }, 0.0f, glm::vec4(0.06f, 0.06f, 0.08f, 0.35f * a));

    // 2. Прямая деревянная рукоять
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 4.0f }, { 3.5f, 40.0f }, 0.0f, glm::vec4(0.58f, 0.40f, 0.24f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -0.7f, 4.0f }, { 1.2f, 38.0f }, 0.0f, glm::vec4(0.72f, 0.52f, 0.32f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 23.0f }, { 5.0f, 3.5f }, 0.0f, glm::vec4(0.42f, 0.28f, 0.16f, a));

    // 3. Стальная проушина/обух на конце рукояти
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, -14.0f }, { 7.0f, 8.0f }, 0.0f, glm::vec4(0.24f, 0.25f, 0.28f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, -14.0f }, { 2.5f, 2.5f }, 0.0f, glm::vec4(0.38f, 0.28f, 0.18f, a));

    // 4. Левое лезвие (заостренный носик-клюв)
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -7.0f, -14.0f }, { 8.0f, 5.0f }, 0.0f, glm::vec4(0.32f, 0.34f, 0.38f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -14.0f, -14.0f }, { 8.0f, 3.8f }, 0.0f, glm::vec4(0.42f, 0.44f, 0.48f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -20.0f, -14.0f }, { 5.0f, 2.2f }, 0.0f, glm::vec4(0.68f, 0.72f, 0.78f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -23.5f, -14.0f }, { 2.5f, 1.2f }, 0.0f, glm::vec4(0.88f, 0.90f, 0.95f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -13.0f, -15.5f }, { 16.0f, 1.0f }, 0.0f, glm::vec4(0.75f, 0.78f, 0.85f, a));

    // 5. Правое лезвие (прямой боек / обух)
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 7.0f, -14.0f }, { 8.0f, 5.5f }, 0.0f, glm::vec4(0.32f, 0.34f, 0.38f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 13.0f, -14.0f }, { 6.0f, 5.0f }, 0.0f, glm::vec4(0.44f, 0.46f, 0.50f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 17.5f, -14.0f }, { 3.5f, 5.5f }, 0.0f, glm::vec4(0.65f, 0.68f, 0.74f, a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 19.5f, -14.0f }, { 1.5f, 5.0f }, 0.0f, glm::vec4(0.82f, 0.85f, 0.90f, a));
}

void drawPuddle(SpriteRenderer* renderer, const std::shared_ptr<Texture2D>& tex, const DecorationConfig& d, float a) {
    glm::vec2 sVec(d.scale * d.scaleX, d.scale * d.scaleY);
    float op = std::clamp(d.opacity, 0.10f, 1.0f);
    float baseWaterAlpha = 0.45f * (op / 0.5f);

    // 1. Мягкая влажная кайма грунта/асфальта вокруг лужи
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 0.0f }, { 52.0f, 32.0f }, 0.0f, glm::vec4(0.08f, 0.12f, 0.14f, 0.18f * baseWaterAlpha * a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 3.0f, 1.0f }, { 44.0f, 28.0f }, 5.0f, glm::vec4(0.08f, 0.12f, 0.14f, 0.15f * baseWaterAlpha * a));

    // 2. Тело лужи (полупрозрачная вода 0.15, 0.22, 0.28, baseWaterAlpha)
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 0.0f }, { 46.0f, 26.0f }, 0.0f, glm::vec4(0.15f, 0.22f, 0.28f, baseWaterAlpha * a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 2.0f, 1.0f }, { 36.0f, 22.0f }, 4.0f, glm::vec4(0.14f, 0.21f, 0.27f, 0.85f * baseWaterAlpha * a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -3.0f, -1.0f }, { 28.0f, 18.0f }, -6.0f, glm::vec4(0.13f, 0.20f, 0.26f, 0.75f * baseWaterAlpha * a));

    // 3. Более глубокая зона в центре
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 1.0f }, { 24.0f, 13.0f }, 0.0f, glm::vec4(0.11f, 0.18f, 0.23f, 0.60f * baseWaterAlpha * a));

    // 4. Тонкий мягкий блик отражения неба по верхнему краю лужи
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, -8.5f }, { 28.0f, 2.2f }, -2.0f, glm::vec4(0.68f, 0.84f, 0.96f, 0.28f * a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -6.0f, -7.5f }, { 14.0f, 1.6f }, -5.0f, glm::vec4(0.78f, 0.90f, 1.0f, 0.32f * a));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 8.0f, -4.0f }, { 12.0f, 1.8f }, 8.0f, glm::vec4(0.60f, 0.78f, 0.92f, 0.22f * a));

    // 5. Тонкая светлая рябь
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -4.0f, 2.0f }, { 15.0f, 1.4f }, -3.0f, glm::vec4(0.65f, 0.82f, 0.95f, 0.20f * a));
}

void drawCrack(SpriteRenderer* renderer, const std::shared_ptr<Texture2D>& tex, const DecorationConfig& d, float a) {
    glm::vec2 sVec(d.scale * d.scaleX, d.scale * d.scaleY);
    float crackAlpha = std::clamp(d.opacity, 0.10f, 1.0f) * a;

    // 1. Разлом
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, 0.0f }, { 40.0f, 3.5f }, 12.0f, glm::vec4(0.04f, 0.04f, 0.06f, 0.95f * crackAlpha));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -12.0f, -3.5f }, { 16.0f, 2.5f }, -35.0f, glm::vec4(0.05f, 0.05f, 0.07f, 0.90f * crackAlpha));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 13.0f, 3.5f }, { 17.0f, 2.5f }, 45.0f, glm::vec4(0.05f, 0.05f, 0.07f, 0.90f * crackAlpha));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { -2.0f, 2.5f }, { 12.0f, 2.0f }, -18.0f, glm::vec4(0.06f, 0.06f, 0.08f, 0.80f * crackAlpha));
    // 2. Сколы краев
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 0.0f, -2.5f }, { 34.0f, 1.5f }, 12.0f, glm::vec4(0.55f, 0.56f, 0.60f, 0.40f * crackAlpha));
    drawPart(renderer, tex, d.worldPos, d.rotation, sVec, { 14.0f, 1.0f }, { 14.0f, 1.5f }, 45.0f, glm::vec4(0.50f, 0.52f, 0.56f, 0.35f * crackAlpha));
}

void drawFog(SpriteRenderer* renderer, const std::shared_ptr<Texture2D>& whiteTexture, const DecorationConfig& d, float a) {
    glm::vec2 sVec(d.scale * d.scaleX, d.scale * d.scaleY);
    float op = std::clamp(d.opacity, 0.05f, 1.0f);
    float finalAlpha = op * a;

    static Texture2D* rawPartTex = nullptr;
    if (!rawPartTex) {
        if (ResourceManager::hasTexture("particleTexture")) {
            rawPartTex = ResourceManager::getTexture("particleTexture");
        } else {
            rawPartTex = ResourceManager::loadTexture("particleTexture", "res/textures/particle.png");
        }
    }
    std::shared_ptr<Texture2D> partTex = rawPartTex 
        ? std::shared_ptr<Texture2D>(rawPartTex, [](Texture2D*) {}) 
        : whiteTexture;

    // Мягкий объемный туман / облако дыма: несколько смещенных перекрывающихся слоев
    glm::vec4 fogColor(0.48f, 0.50f, 0.52f, 0.40f * finalAlpha);
    glm::vec4 coreColor(0.52f, 0.54f, 0.56f, 0.55f * finalAlpha);
    glm::vec4 outerColor(0.44f, 0.46f, 0.48f, 0.25f * finalAlpha);

    // 1. Широкое рассеянное внешнее облако
    drawPart(renderer, partTex, d.worldPos, d.rotation, sVec, { 0.0f, 0.0f }, { 140.0f, 110.0f }, 0.0f, outerColor);
    // 2. Смещенные полупрозрачные лепестки для естественной аморфной формы
    drawPart(renderer, partTex, d.worldPos, d.rotation, sVec, { -22.0f, -10.0f }, { 110.0f, 85.0f }, 15.0f, fogColor);
    drawPart(renderer, partTex, d.worldPos, d.rotation, sVec, { 25.0f, 8.0f }, { 115.0f, 90.0f }, -12.0f, fogColor);
    drawPart(renderer, partTex, d.worldPos, d.rotation, sVec, { 8.0f, -18.0f }, { 95.0f, 75.0f }, 8.0f, fogColor);
    // 3. Более плотное центральное ядро
    drawPart(renderer, partTex, d.worldPos, d.rotation, sVec, { 2.0f, 2.0f }, { 80.0f, 65.0f }, 0.0f, coreColor);
}

void drawSelection(SpriteRenderer* renderer, const std::shared_ptr<Texture2D>& tex, const DecorationConfig& d) {
    float sx = std::max(0.2f, d.scale * d.scaleX);
    float sy = std::max(0.2f, d.scale * d.scaleY);
    float baseDim = (d.type == "fog") ? 130.0f : ((d.type == "grass_field") ? 64.0f : ((d.type == "bush") ? 56.0f : (d.type == "puddle" ? 52.0f : 42.0f)));
    float boxW = baseDim * sx;
    float boxH = baseDim * sy;
    float cornerLen = std::min(12.0f, std::min(boxW, boxH) * 0.35f);
    float cornerThick = 2.0f;
    glm::vec4 frameCol(0.35f, 0.85f, 1.0f, 0.95f);
    glm::vec4 dotCol(1.0f, 0.85f, 0.20f, 0.95f);

    float halfW = boxW * 0.5f;
    float halfH = boxH * 0.5f;
    glm::vec2 s1(1.0f, 1.0f);

    // 4 уголка
    // Top-Left
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { -halfW + cornerLen * 0.5f, -halfH + cornerThick * 0.5f }, { cornerLen, cornerThick }, 0.0f, frameCol);
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { -halfW + cornerThick * 0.5f, -halfH + cornerLen * 0.5f }, { cornerThick, cornerLen }, 0.0f, frameCol);
    // Top-Right
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { halfW - cornerLen * 0.5f, -halfH + cornerThick * 0.5f }, { cornerLen, cornerThick }, 0.0f, frameCol);
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { halfW - cornerThick * 0.5f, -halfH + cornerLen * 0.5f }, { cornerThick, cornerLen }, 0.0f, frameCol);
    // Bottom-Left
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { -halfW + cornerLen * 0.5f, halfH - cornerThick * 0.5f }, { cornerLen, cornerThick }, 0.0f, frameCol);
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { -halfW + cornerThick * 0.5f, halfH - cornerLen * 0.5f }, { cornerThick, cornerLen }, 0.0f, frameCol);
    // Bottom-Right
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { halfW - cornerLen * 0.5f, halfH - cornerThick * 0.5f }, { cornerLen, cornerThick }, 0.0f, frameCol);
    drawPart(renderer, tex, d.worldPos, d.rotation, s1, { halfW - cornerThick * 0.5f, halfH - cornerLen * 0.5f }, { cornerThick, cornerLen }, 0.0f, frameCol);

    // Точка в центре
    drawPart(renderer, tex, d.worldPos, 0.0f, s1, { 0.0f, 0.0f }, { 4.0f, 4.0f }, 0.0f, dotCol);
}

} // namespace

void DecorationRenderer::renderDecoration(
    SpriteRenderer* renderer,
    std::shared_ptr<Texture2D> whiteTexture,
    const DecorationConfig& decor,
    bool isSelected,
    float alpha,
    float cellSize
) {
    if (!renderer || !whiteTexture) return;

    const float kTileRef = 64.0f;
    float cellScaleFactor = (cellSize > 0.001f) ? (cellSize / kTileRef) : 1.0f;

    DecorationConfig d = decor;
    d.scale *= cellScaleFactor;

    if (d.type == "bush") {
        drawBush(renderer, whiteTexture, d, alpha);
    } else if (d.type == "grass_tuft") {
        drawGrassTuft(renderer, whiteTexture, d, alpha);
    } else if (d.type == "grass_field") {
        drawGrassField(renderer, whiteTexture, d, alpha);
    } else if (d.type == "flower") {
        drawFlower(renderer, whiteTexture, d, alpha);
    } else if (d.type == "stone") {
        drawStone(renderer, whiteTexture, d, alpha);
    } else if (d.type == "helmet") {
        drawHelmet(renderer, whiteTexture, d, alpha);
    } else if (d.type == "pickaxe") {
        drawPickaxe(renderer, whiteTexture, d, alpha);
    } else if (d.type == "puddle") {
        drawPuddle(renderer, whiteTexture, d, alpha);
    } else if (d.type == "crack") {
        drawCrack(renderer, whiteTexture, d, alpha);
    } else if (d.type == "fog") {
        drawFog(renderer, whiteTexture, d, alpha);
    } else {
        // Запасной вариант для неизвестного типа
        drawBush(renderer, whiteTexture, d, alpha);
    }

    if (isSelected) {
        drawSelection(renderer, whiteTexture, d);
    }
}

void DecorationRenderer::renderDecoration(
    SpriteRenderer* renderer,
    std::shared_ptr<Texture2D> whiteTexture,
    glm::vec2 renderPos,
    const DecorationConfig& decor,
    bool isSelected,
    float alpha,
    float cellSize
) {
    DecorationConfig decCopy = decor;
    decCopy.worldPos = renderPos;
    renderDecoration(renderer, whiteTexture, decCopy, isSelected, alpha, cellSize);
}

void DecorationRenderer::renderDecorations(
    SpriteRenderer* renderer,
    std::shared_ptr<Texture2D> whiteTexture,
    const std::vector<DecorationConfig>& decorations,
    int selectedIndex,
    float cellSize,
    bool onlyFog
) {
    if (!renderer || !whiteTexture) return;

    for (size_t i = 0; i < decorations.size(); ++i) {
        bool isFog = (decorations[i].type == "fog" || decorations[i].type == "darkness");
        if (onlyFog != isFog) continue;
        bool isSel = (static_cast<int>(i) == selectedIndex);
        renderDecoration(renderer, whiteTexture, decorations[i], isSel, 1.0f, cellSize);
    }
}

void DecorationRenderer::renderDecorations(
    SpriteRenderer* renderer,
    std::shared_ptr<Texture2D> whiteTexture,
    const std::vector<DecorationConfig>& decorations,
    glm::vec2 gridOffset,
    float cellSize,
    int selectedIndex,
    bool onlyFog
) {
    if (!renderer || !whiteTexture) return;

    for (size_t i = 0; i < decorations.size(); ++i) {
        bool isFog = (decorations[i].type == "fog" || decorations[i].type == "darkness");
        if (onlyFog != isFog) continue;
        bool isSel = (static_cast<int>(i) == selectedIndex);
        glm::vec2 renderPos = gridOffset + decorations[i].tilePos * cellSize;
        renderDecoration(renderer, whiteTexture, renderPos, decorations[i], isSel, 1.0f, cellSize);
    }
}

void DecorationRenderer::renderDecorations(
    SpriteRenderer* renderer,
    std::shared_ptr<Texture2D> whiteTexture,
    const std::vector<DecorationConfig>& decorations,
    const Grid& grid,
    int selectedIndex,
    bool onlyFog
) {
    renderDecorations(renderer, whiteTexture, decorations, grid.getOffset(), grid.getCellSize(), selectedIndex, onlyFog);
}

void DecorationRenderer::renderPreview(
    SpriteRenderer* renderer,
    std::shared_ptr<Texture2D> whiteTexture,
    const std::string& type,
    glm::vec2 worldPos,
    float rotation,
    float scale,
    float scaleX,
    float scaleY,
    float opacity,
    float cellSize
) {
    DecorationConfig d;
    d.type = type;
    d.worldPos = worldPos;
    d.rotation = rotation;
    d.scale = scale;
    d.scaleX = scaleX;
    d.scaleY = scaleY;
    d.opacity = opacity;
    renderDecoration(renderer, whiteTexture, d, false, 0.65f, cellSize);
}

