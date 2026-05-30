#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "Enemy.h"

enum class CellType {
	Empty,
	Tower,
	Road,
	Blocked
};

class SpriteRenderer;
class Texture2D;

class Grid
{

private:
	int m_width; // количество клеток по ширине
	int m_height; // количество клеток по высоте
	float m_cellSize; // размер одной клетки в пикселях
	glm::vec2 m_offset; // ссув поля

	std::vector<std::vector<CellType>> m_grid; // 2D вектор для хранения типа каждой клетки

public:
	Grid(int width, int height, float cellSize, glm::vec2 offset = glm::vec2(0.0f, 0.0f));

	glm::vec2 gridToPixel(int gridX, int gridY) const; // тут получаем левый верхний угол клетки
	glm::ivec2 pixelToGrid(glm::vec2 pixelPos) const; // получаем кординаты мыши и возращаем индекс клетки в масиве

	bool canBuildAt(int gridX, int gridY) const; // проверяем можно ли построить башню на этой клетке
	void setCellType(int gridX, int gridY, CellType type); // устанавливаем тип клетки

	void draw(SpriteRenderer* renderer,
		std::shared_ptr<Texture2D> grassTexture,
		std::shared_ptr<Texture2D> towerTexture,
		glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f)); // рисуем сетку

	void updateCellSize(int windowWidth, int windowHeight); // обновляем размер клеток при изменении размера окна, чтобы сетка всегда занимала все окно
	float getCellSize() const { return m_cellSize; } // получаем размер клетки
	glm::vec2 getOffset() const { return m_offset; }
};