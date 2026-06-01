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
	int reward;
};

class SpriteRenderer;
class Texture2D;
class Grid;

class Enemy
{
private:
	EnemyType m_type; // тип характеристик врага
	int m_health; // здоровье врага
	float m_speed; // скорость врага
	int m_reward; // награда за убийство врага


	const std::vector<glm::ivec2>& m_path; // Ссылка на общий путь врагов
	size_t m_currentWayPoint; // текущая точка к которой враг идет
	glm::vec2 m_pixelPos; // координаты врага в пикселях экрнана
	bool m_reachedEnd; // флаг дошел ли враг до конца

public:

	// Функция для получения характеристик врага в зависимости от его типа
	static EnemyStats getStatsfromEnemyType(EnemyType type) {
		switch (type) {
			case EnemyType::Basic:
				return { 0.5f, 100, 0.6f, 10 };
			case EnemyType::Fast:
				return { 1.5f, 50, 0.5f, 15};
			case EnemyType::Tank:
				return { 0.3f, 500, 0.7f, 30};
		}
		return { 1.5f, 100, 0.6f, 10 };
	}

	void recalculatePosition(const Grid& oldGrid, const Grid& newGrid); // вызывается при изменении размера окна, чтобы враг всегда был точно на клетке, даже если размер клеток изменится при ресайзе окна

	Enemy(const std::vector<glm::ivec2>& gridPath, const Grid& grid, EnemyType type);
	void update(float dt, const Grid& grid);

	void render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> texture, glm::vec2 gridOffset, const Grid& grid);
	glm::vec2 getPixelPos() const { return m_pixelPos; }
	bool isReachedEnd() const { return m_reachedEnd; }
	void takeDamage(int damage) {
		m_health -= damage;
	}

	bool isDead() const { // возвращает true если враг имеет мельше 0 хп и false если больше 
		if (m_health <= 0) {
			return true;
		}
		return false;
	}

	int getReward() const { return m_reward; } // геттер который возгращает награду за убийство врага
};