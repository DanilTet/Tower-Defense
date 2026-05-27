#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>

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

	std::vector<std::vector<CellType>> m_grid; // 2D вектор для хранения типа каждой клетки

public:
	Grid(int width, int height, float cellSize);

	glm::vec2 gridToPixel(int gridX, int gridY) const; // тут получаем левый верхний угол клетки
	glm::ivec2 pixelToGrid(glm::vec2 pixelPos) const; // получаем кординаты мыши и возращаем индекс клетки в масиве

	bool canBuildAt(int gridX, int gridY) const; // проверяем можно ли построить башню на этой клетке
	void setCellType(int gridX, int gridY, CellType type); // устанавливаем тип клетки

	void draw(SpriteRenderer* renderer, std::shared_ptr<Texture2D> cellTexture); // рисуем сетку
};