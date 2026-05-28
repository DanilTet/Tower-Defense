#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <memory>

class SpriteRenderer;
class Texture2D;
class Grid;

class Enemy
{
private:
	const std::vector<glm::ivec2>& m_path;
	size_t m_currentWayPoint;

	glm::vec2 m_pixelPos;
	float m_speed;
	bool m_reachedEnd;

public:
	Enemy(const std::vector<glm::ivec2>& gridPath, const Grid& grid, float speed = 100.0f);
	void update(float dt, const Grid& grid);

	void render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> texture, glm::vec2 gridOffset);

	bool isReachedEnd() const { return m_reachedEnd; }
};