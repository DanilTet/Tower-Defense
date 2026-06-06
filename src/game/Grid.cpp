#include "Grid.h"
#include "renderer/SpriteRenderer.h"
#include "textures/Texture2D.h"
#include "resources/ResourceManager.h"
#include "Enemy.h"

// конструктор создает пустую сетку заданого размера
Grid::Grid(int width, int height, float cellSize, glm::vec2 offset){
	m_width = width; // Запоминаем количество клеток по горизонтали
	m_height = height; // Запоминаем количество клеток по вертикали
	m_cellSize = cellSize; // Задаем стартовый размер одной клетки в пикселях
	m_offset = offset; // запоминаем ссув
	// Двумерный вектор для хранения типа каждой клетки, изначально все клетки пустые
	m_grid.resize(m_height, std::vector<CellType>(m_width, CellType::Empty));
}

// Перевод из индексов клетки в пиксели экрана
glm::vec2 Grid::gridToPixel(int gridX, int gridY) const {
	// умножаем индекс строки и столбца на размер клетки в пикселях
	return glm::vec2(gridX * m_cellSize, gridY * m_cellSize) + m_offset;
}

// Перевод из пикселей экрана в индексы клетки сетки (х,у)
glm::ivec2 Grid::pixelToGrid(glm::vec2 pixelPos) const {
	// Считаем смещение сетки. чтобы получить координаты относительно сетки
	float localX = pixelPos.x - m_offset.x;
	float localY = pixelPos.y - m_offset.y;

	// Делим локальные координаты на размер клетки, чтобы получить индексы строки и столбца
	int gridX = static_cast<int>(localX / m_cellSize);
	int gridY = static_cast<int>(localY / m_cellSize);

	// возвращаем индексы клетки в виде целочисленного вектора
	return glm::ivec2(gridX, gridY);
}

// Проверяем можно ли построить башню на клетке с этими индексами
bool Grid::canBuildAt(int gridX, int gridY) const {
	// если координаты вышли за поле то строить нельзя
	if (gridX < 0 || gridX >= m_width || gridY < 0 || gridY >= m_height) {
		return false;
	}
	return m_grid[gridY][gridX] == CellType::Empty;
}

// Принудительно меняем тип конкретной ячейки
void Grid::setCellType(int gridX, int gridY, CellType type) {
	// защищаем массив от вылета за границы памяти
	if (gridX >= 0 && gridX < m_width && gridY >= 0 && gridY < m_height) {
		// В нашем движке первый индекс всегда X (столбец), а второй Y (строка)!
		m_grid[gridY][gridX] = type;
	}
}

// Отрисовка всей карты ячейка за ячейкой
void Grid::draw(SpriteRenderer* renderer,
	std::shared_ptr<Texture2D> grassTexture,
	std::shared_ptr<Texture2D> towerTexture,
	glm::vec3 color) {

	// Создаем вектор размера: каждая плитка будет шириной и высотой ровно в m_cellSize пикселей
	glm::vec2 size(m_cellSize, m_cellSize);

	// вложенный цикл для обхода всей матрицы (Y — строки, X — столбцы)
	for (int y = 0; y < m_height; ++y) {
		for (int x = 0; x < m_width; ++x) {
			// Вычисляем, где физически на экране должен стоять этот квадрат (базовая позиция + общий сдвиг сетки)
			glm::vec2 pixelPos = gridToPixel(x, y);

			// если ячейка пустая в масиве
			if (m_grid[y][x] == CellType::Empty || m_grid[y][x] == CellType::Tower || m_grid[y][x] == CellType::Path) {
				// Рисуем обычную стандартную плитку
				renderer->drawSprite(grassTexture, pixelPos, size, 0.0f, color);
			}
		}
	}
}
// Динамический пересчет размера клеток при изменении размера окна
void Grid::updateCellSize(int windowWidth, int windowHeight) {
	// Считаем доступну. ширину и высоту с вычетом симетричных отступов
	float availableWidth = static_cast<float>(windowWidth) - (2.0f * m_offset.x);
	float availableHeight = static_cast<float>(windowHeight) - (2.0f * m_offset.y);

	// защита если окно свернуто
	if (availableWidth < 0.0f) {
		availableWidth = 0.0f;
	}
	if (availableHeight < 0.0f) {
		availableHeight = 0.0f;
	}

	// считаем сколько пикселей занимает одна клетка в доступной зоне
	float sizeX = availableWidth / m_width;
	float sizeY = availableHeight / m_height;
	
	// Выбираем меньший из этих двух размеров, чтобы клетки всегда были квадратными и сетка занимала все окно
	if (sizeX < sizeY) {
		m_cellSize = sizeX;
	}
	else {
		m_cellSize = sizeY;
	}
}


CellType Grid::getCellType(int gridX, int gridY) const {
	// 1. Сначала железная броня (чтобы не выйти за пределы 10 и 7)
	if (gridX >= 0 && gridX < m_width && gridY >= 0 && gridY < m_height) {

		// 2. ИМЕННО [gridX][gridY], а не наоборот!
		return m_grid[gridY][gridX];
	}

	// Если алгоритм спрашивает про клетку за экраном - говорим, что там стена
	return CellType::Blocked;
}