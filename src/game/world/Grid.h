#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "entities/Enemy.h"
#include "entities/Tower.h"

enum class CellType {
	Ground,    // 1: Можно строить, можно ходить (враги могут тут идти, пока ты не поставишь башню)
	Path,      // 2: Нельзя строить, можно ходить (чистая дорога)
	Platform,  // 3: Можно строить, нельзя ходить (например, гора для снайпера)
	Scenery,   // 4: Нельзя строить, нельзя ходить (вода/скалы)

	Tower, // башня
	Spawner, // точка старта врагов
	Base // база игрока
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
	//std::vector<std::unique_ptr<Tower>> m_towers; // 2D вектор для хранения башен

public:
	Grid(int width, int height, float cellSize, glm::vec2 offset = glm::vec2(0.0f, 0.0f));

	glm::vec2 gridToPixel(int gridX, int gridY) const; // тут получаем левый верхний угол клетки
	glm::ivec2 pixelToGrid(glm::vec2 pixelPos) const; // получаем кординаты мыши и возращаем индекс клетки в масиве

	bool canBuildAt(int gridX, int gridY) const; // проверяем можно ли построить башню на этой клетке
	void setCellType(int gridX, int gridY, CellType type); // устанавливаем тип клетки

	void draw(SpriteRenderer* renderer,
		std::shared_ptr<Texture2D> atlasTexture, // только атлас
		glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f));

	void updateCellSize(int windowWidth, int windowHeight); // обновляем размер клеток при изменении размера окна, чтобы сетка всегда занимала все окно
	float getCellSize() const { return m_cellSize; } // получаем размер клетки
	glm::vec2 getOffset() const { return m_offset; }

	// геттеры для алгоритма А*
	int getWidth() const { return m_width; }
	int getHeight() const { return m_height; }
	CellType getCellType(int gridX, int gridY) const;
};