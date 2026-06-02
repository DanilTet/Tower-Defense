#include "Projectile.h"
#include "../renderer/SpriteRenderer.h"
#include "../textures/Texture2D.h"

Projectile::Projectile(glm::vec2 startPos, glm::vec2 targetPos, float speed, int damage, int targetId)
	: m_pos(startPos),
	m_speed(speed),
	m_damage(damage),
	m_radius(5.0f),
	m_destroyed(false),
	m_lifeTime(3.0f),
    m_targetId(targetId)

{
	glm::vec2 toTarget = targetPos - startPos; // векиор от башни к врагу

	// нормализируем его тоесть делаем длину 1 чтобі пуля равномерно летела
	if (glm::length(toTarget) > 0.0f) {
		m_direction = glm::normalize(toTarget);
	}
	else {
        m_direction = glm::vec2(0.0f, 1.0f); // если координаті совпали
	}
}

void Projectile::update(float dt, const std::vector<std::unique_ptr<Enemy>>& enemies, const Grid& grid) {
    if (m_destroyed) return;

    // двигаем пулю по направлению
    m_pos += m_direction * m_speed * dt;

    // уменьшаем время жизни
    m_lifeTime -= dt;
    if (m_lifeTime <= 0.0f) {
        m_destroyed = true;
        return;
    }

    bool targetFound = false; // флаг долетели ли к врагу

    // ищем айди врага
    for (const auto& enemy : enemies) {
        if (enemy && !enemy->isDead() && !enemy->isReachedEnd() && enemy->getId() == m_targetId) {
            targetFound = true; // нашли врага

            // ведем вектор направления прямо на центр врага
            glm::vec2 toTarget = enemy->getCollider(grid).center - m_pos;
            if (glm::length(toTarget) > 0.0f) {
                m_direction = glm::normalize(toTarget);
            }
            
            // летим по вектору
            m_pos += m_direction * m_speed * dt;
            CircleCollider myCollider = { m_pos, m_radius };

            // проверяем столкновение только с текущим врагом
            if (myCollider.intersects(enemy->getCollider(grid))) {
                enemy->takeDamage(m_damage);
                m_destroyed = true;
            }
            break;
        }
    }
    // если враг уже убит то пуля летит дальше
    if (!targetFound) {
        m_pos += m_direction * m_speed * dt;
    }
}

void Projectile::render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> texture) {
    if (m_destroyed) return;

    // размер спрайта пули
    glm::vec2 size(m_radius * 2.0f, m_radius * 2.0f);

    // сдвигаем, чтобы m_pos был центром пули
    glm::vec2 drawPos = m_pos - glm::vec2(m_radius, m_radius);

    // рисуем
    renderer->drawSprite(texture, drawPos, size, 0.0f, glm::vec3(1.0f, 1.0f, 0.0f));
}