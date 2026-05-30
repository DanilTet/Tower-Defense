#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <memory>

enum class EnemyType {
	Basic,
	Fast,
	Tank
};

// Структура для хранения характеристик врага
struct EnemyStats {
	float speed;
	int Maxhealth;
	float sizeScale;
	//int reward;
};

class SpriteRenderer;
class Texture2D;
class Grid;

class Enemy
{
private:
	EnemyType m_type;
	int m_health;
	float m_speed;

	const std::vector<glm::ivec2>& m_path;
	size_t m_currentWayPoint;
	glm::vec2 m_pixelPos;
	bool m_reachedEnd;

public:

	// Функция для получения характеристик врага в зависимости от его типа
	static EnemyStats getStatsfromEnemyType(EnemyType type) {
		switch (type) {
			case EnemyType::Basic:
				return { 100.0f, 100, 0.6f };
			case EnemyType::Fast:
				return { 150.0f, 50, 0.5f};
			case EnemyType::Tank:
				return { 50.0f, 500, 0.7f};
		}
		return { 100.0f, 100, 0.6f };
	}

	void recalculatePosition(const Grid& oldGrid, const Grid& newGrid); // вызывается при изменении размера окна, чтобы враг всегда был точно на клетке, даже если размер клеток изменится при ресайзе окна

	Enemy(const std::vector<glm::ivec2>& gridPath, const Grid& grid, EnemyType type);
	void update(float dt, const Grid& grid);

	void render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> texture, glm::vec2 gridOffset, const Grid& grid);

	bool isReachedEnd() const { return m_reachedEnd; }
};