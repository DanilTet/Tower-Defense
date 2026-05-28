#include "Enemy.h"
#include "../renderer/SpriteRenderer.h"
#include "../textures/Texture2D.h"
#include "Grid.h"
#include <iostream>

Enemy::Enemy(const std::vector<glm::ivec2>& gridPath, const Grid& grid, float speed)
    : m_path(gridPath), m_currentWayPoint(0), m_speed(speed), m_reachedEnd(false)
{
    if (!m_path.empty()) {
        m_pixelPos = grid.gridToPixel(m_path[0].x, m_path[0].y);
    }
}

void Enemy::update(float dt, const Grid& grid) {
    if (m_reachedEnd || m_path.empty()) return;

    glm::ivec2 targetCell = m_path[m_currentWayPoint];
    glm::vec2 targetPixelPos = grid.gridToPixel(targetCell.x, targetCell.y);

    glm::vec2 toTarget = targetPixelPos - m_pixelPos;
    float distance = glm::length(toTarget);

    float moveDistance = m_speed * dt;

    if (distance <= moveDistance) {
        m_pixelPos = targetPixelPos;
        m_currentWayPoint++;

        if (m_currentWayPoint >= m_path.size()) {
            m_reachedEnd = true;
            std::cout << "Враг прорвал оборону!" << std::endl;
        }
    }
    else {
        glm::vec2 direction = glm::normalize(toTarget);
        m_pixelPos += direction * moveDistance;
    }
}

void Enemy::render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> texture, glm::vec2 gridOffset) {
    if (m_reachedEnd) return;

    glm::vec2 size(40.0f, 40.0f);

    glm::vec2 centeredPos = m_pixelPos + gridOffset + glm::vec2(12.0f, 12.0f);

    renderer->drawSprite(texture, centeredPos, size, 0.0f, glm::vec3(0.2f, 1.0f, 0.2f));
}