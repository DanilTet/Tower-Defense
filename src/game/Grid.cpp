#include "Grid.h"
#include "renderer/SpriteRenderer.h"
#include "textures/Texture2D.h"

Grid::Grid(int width, int height, float cellSize){
	m_width = width;
	m_height = height;
	m_cellSize = cellSize;
	m_grid.resize(m_height, std::vector<CellType>(m_width, CellType::Empty));
}

glm::vec2 Grid::gridToPixel(int gridX, int gridY) const {
	return glm::vec2(gridX * m_cellSize, gridY * m_cellSize);
}

glm::ivec2 Grid::pixelToGrid(glm::vec2 pixelPos) const {
	int gridX = static_cast<int>(pixelPos.x / m_cellSize);
	int gridY = static_cast<int>(pixelPos.y / m_cellSize);

	return glm::ivec2(gridX, gridY);
}

bool Grid::canBuildAt(int gridX, int gridY) const {
	if (gridX >= 0 && gridX < m_width && gridY >= 0 && gridY < m_height) {
		return m_grid[gridY][gridX] == CellType::Empty;
	}
	return false;
}

void Grid::setCellType(int gridX, int gridY, CellType type) {
	if (gridX >= 0 && gridX < m_width && gridY >= 0 && gridY < m_height) {
		m_grid[gridY][gridX] = type;
	}
}

void Grid::draw(SpriteRenderer* renderer, std::shared_ptr<Texture2D> cellTexture) {
	for (int y = 0; y < m_height; ++y) {
		for (int x = 0; x < m_width; ++x) {
			glm::vec2 pixelPos = gridToPixel(x, y);
			glm::vec2 size(m_cellSize, m_cellSize);
			renderer->drawSprite(cellTexture, pixelPos, size, 0.0f, { 1.0f, 1.0f, 1.0f });
		}
	}
}
