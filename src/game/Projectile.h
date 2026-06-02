#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "CircleCollider.h"
#include "Enemy.h"

class SpriteRenderer;
class Texture2D;
class Grid;

class Projectile {
public:
	// конструктор принимает откуда летим, куда целимся, скорость и урон и айди цели
	Projectile(glm::vec2 startPos, glm::vec2 targetPos, float speed, int damage, int targetId);

	// двигаем пулу и проверяем столкновение со всеми врагами
	void update(float dt, const std::vector<std::unique_ptr<Enemy>>& enemies, const Grid& grid);

	// отрисовка
	void render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> texture);

	// жива ли еще пуля
	bool isDestroyed() const {
		return m_destroyed;
	}
private:
	glm::vec2 m_pos; // текущая позиция пули
	glm::vec2 m_direction; // нормализированій вектор направления
	float m_speed; // скорость пиксели в секунду
	int m_damage; // урон
	float m_radius; // размер хитбокса пули

	bool m_destroyed; // флаг на удаление
	float m_lifeTime; // таймер самоуничтожения

	int m_targetId; // айди врага к которому летит
};