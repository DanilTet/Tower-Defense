#pragma once
#include <glm/glm.hpp>

// все опрные точки
enum class UIAnchor {
    TopLeft,
    TopCenter,
    TopRight,
    BottomLeft,
    BottomCenter,
    BottomRight,
    Center
};

// четенькая функция расчета масштаба интерфейса
inline float GetUIScale(int screenWidth, int screenHeight) {
    float scaleX = static_cast<float>(screenWidth) / 1280.0f;
    float scaleY = static_cast<float>(screenHeight) / 720.0f;

    // выбираем минимальный масштаб чтобы интерфейс не сплющивало
    return std::min(scaleX, scaleY);
}

// функция которая считает левый верхний угол (X, Y) для отрисовки спрайта или текста
inline glm::vec2 CalculateAnchorPosition(UIAnchor anchor, glm::vec2 offset, glm::vec2 elementSize, int screenWidth, int screenHeight) {
    glm::vec2 finalPos(0.0f);

    switch (anchor) {
    case UIAnchor::TopLeft:
        finalPos = glm::vec2(0.0f, 0.0f) + offset;
        break;

    case UIAnchor::TopCenter:
        finalPos.x = (static_cast<float>(screenWidth) / 2.0f) - (elementSize.x / 2.0f) + offset.x;
        finalPos.y = 0.0f + offset.y;
        break;

    case UIAnchor::TopRight:
        finalPos.x = static_cast<float>(screenWidth) - elementSize.x - offset.x;
        finalPos.y = 0.0f + offset.y;
        break;

    case UIAnchor::BottomLeft:
        finalPos.x = 0.0f + offset.x;
        finalPos.y = static_cast<float>(screenHeight) - elementSize.y - offset.y;
        break;

    case UIAnchor::BottomCenter:
        finalPos.x = (static_cast<float>(screenWidth) / 2.0f) - (elementSize.x / 2.0f) + offset.x;
        finalPos.y = static_cast<float>(screenHeight) - elementSize.y - offset.y;
        break;

    case UIAnchor::BottomRight:
        finalPos.x = static_cast<float>(screenWidth) - elementSize.x - offset.x;
        finalPos.y = static_cast<float>(screenHeight) - elementSize.y - offset.y;
        break;

    case UIAnchor::Center:
        finalPos.x = (static_cast<float>(screenWidth) / 2.0f) - (elementSize.x / 2.0f) + offset.x;
        finalPos.y = (static_cast<float>(screenHeight) / 2.0f) - (elementSize.y / 2.0f) + offset.y;
        break;
    }

    return finalPos;
}