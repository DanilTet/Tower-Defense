#include "Grid.h"
#include "renderer/SpriteRenderer.h"
#include "textures/Texture2D.h"
#include "resources/ResourceManager.h"
#include "../core/ConfigManager.h"

// конструктор создает пустую сетку заданого размера
Grid::Grid(int width, int height, float cellSize, glm::vec2 offset){
	m_width = width; // Запоминаем количество клеток по горизонтали
	m_height = height; // Запоминаем количество клеток по вертикали
	m_cellSize = cellSize; // Задаем стартовый размер одной клетки в пикселях
	m_offset = offset; // запоминаем ссув
	// Двумерный вектор для хранения типа каждой клетки, изначально все клетки пустые
	m_grid.resize(m_height, std::vector<CellType>(m_width, CellType::Ground));
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

	CellType type = getCellType(gridX, gridY);

	// строить можно
	return (type == CellType::Ground || type == CellType::Platform);
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
void Grid::draw(SpriteRenderer* renderer, std::shared_ptr<Texture2D> atlasTexture, std::shared_ptr<Texture2D> transitionsTexture, glm::vec3 color) {

	// Создаем вектор размера: каждая плитка будет шириной и высотой ровно в m_cellSize пикселей
	glm::vec2 size(m_cellSize, m_cellSize);

	//fromPixels(X, Y, Ширина, Высота, ШиринаАтласа, ВысотаАтласа)
	SpriteUV uvGrass = ConfigManager::getUV("main_atlas", "grass");
	SpriteUV uvPath = ConfigManager::getUV("main_atlas", "path");
	SpriteUV uvPlatform = ConfigManager::getUV("main_atlas", "platform");
	SpriteUV uvScenery = ConfigManager::getUV("main_atlas", "scenery");

	// Текстурные координаты из Frame 1 (1).png (размер 192 x 128)
	int transW = 192;
	int transH = 128;

	// Каменный переход: 0, 0, 64, 64 (смотрит снизу вверх)
	SpriteUV uvStoneTrans = SpriteUV::fromPixels(0, 0, 64, 64, transW, transH);

	// Травяные переходы (4 отдельные текстуры):
	SpriteUV uvGrassTransN = SpriteUV::fromPixels(64, 0, 64, 64, transW, transH);
	SpriteUV uvGrassTransS = SpriteUV::fromPixels(128, 0, 64, 64, transW, transH);
	SpriteUV uvGrassTransE = SpriteUV::fromPixels(64, 64, 64, 64, transW, transH);
	SpriteUV uvGrassTransW = SpriteUV::fromPixels(128, 64, 64, 64, transW, transH);

	auto getPriority = [](CellType t) {
		switch (t) {
			case CellType::Platform: return 3; // Камень (самый высокий)
			case CellType::Ground:   return 2; // Трава
			case CellType::Tower:    return 2; // Башня (на траве)
			case CellType::Path:     return 1; // Земля / Дорога
			default:                 return 0; // Вода / Скалы
		}
	};

	auto getNeighborType = [&](int cx, int cy, CellType currentType) {
		if (cx >= 0 && cx < m_width && cy >= 0 && cy < m_height) {
			return m_grid[cy][cx];
		}
		return currentType; // на границах берем тип текущего тайла
	};

	// вложенный цикл для обхода всей матрицы (Y — строки, X — столбцы)
	for (int y = 0; y < m_height; ++y) {
		for (int x = 0; x < m_width; ++x) {
			// Вычисляем, где физически на экране должен стоять этот квадрат (базовая позиция + общий сдвиг сетки)
			glm::vec2 pixelPos = gridToPixel(x, y);
			CellType type = m_grid[y][x];

			SpriteUV currentUV = uvGrass; // по умолчанию это трава

			if (type == CellType::Path) currentUV = uvPath;
			if (type == CellType::Platform) currentUV = uvPlatform;
			if (type == CellType::Scenery) currentUV = uvScenery;

			// 1. Рисуем базовый тайл
			renderer->drawSprite(atlasTexture, pixelPos, size, 0.0f, color, currentUV);

			// 2. Накладываем переходы поверх базового тайла
			int currPriority = getPriority(type);

			// Проверяем соседей
			CellType nType = getNeighborType(x, y - 1, type);
			CellType sType = getNeighborType(x, y + 1, type);
			CellType eType = getNeighborType(x + 1, y, type);
			CellType wType = getNeighborType(x - 1, y, type);

			int np = getPriority(nType);
			int sp = getPriority(sType);
			int ep = getPriority(eType);
			int wp = getPriority(wType);

			// --- Каменный переход (Platform) ---
			// Рисуем на любых клетках ниже камня, которые с ним граничат
			if (currPriority < 3 && transitionsTexture) {
				if (np == 3) {
					renderer->drawSprite(transitionsTexture, pixelPos, size, 180.0f, color, uvStoneTrans);
				}
				if (sp == 3) {
					renderer->drawSprite(transitionsTexture, pixelPos, size, 0.0f, color, uvStoneTrans);
				}
				if (ep == 3) {
					renderer->drawSprite(transitionsTexture, pixelPos, size, -90.0f, color, uvStoneTrans);
				}
				if (wp == 3) {
					renderer->drawSprite(transitionsTexture, pixelPos, size, 90.0f, color, uvStoneTrans);
				}
			}

			// --- Травяной переход (Ground/Tower) ---
			// Рисуем только на земле (дороге), которая граничит с травой
			if (currPriority == 1 && transitionsTexture) {
				if (np == 2) {
					renderer->drawSprite(transitionsTexture, pixelPos, size, 0.0f, color, uvGrassTransN);
				}
				if (sp == 2) {
					renderer->drawSprite(transitionsTexture, pixelPos, size, 0.0f, color, uvGrassTransS);
				}
				if (ep == 2) {
					renderer->drawSprite(transitionsTexture, pixelPos, size, 0.0f, color, uvGrassTransE);
				}
				if (wp == 2) {
					renderer->drawSprite(transitionsTexture, pixelPos, size, 0.0f, color, uvGrassTransW);
				}
			}
		}
	}
}
// Динамический пересчет размера клеток при изменении размера окна
void Grid::updateCellSize(int windowWidth, int windowHeight) {
	// считаем размер клетки
	float sizeX = static_cast<float>(windowWidth) / static_cast<float>(m_width);
	float sizeY = static_cast<float>(windowHeight) / static_cast<float>(m_height);

	// берем меньшее значение, чтобы клетки всегда оставались квадратными
	m_cellSize = std::min(sizeX, sizeY);

	// считаем фактический размер всей сетки в пикселях
	float actualGridWidth = m_cellSize * m_width;
	float actualGridHeight = m_cellSize * m_height;

	// высчитываем НОВЫЕ отступы
	m_offset.x = (windowWidth - actualGridWidth) / 2.0f;
	m_offset.y = (windowHeight - actualGridHeight) / 2.0f;
}


CellType Grid::getCellType(int gridX, int gridY) const {
	// 1. Сначала железная броня (чтобы не выйти за пределы 10 и 7)
	if (gridX >= 0 && gridX < m_width && gridY >= 0 && gridY < m_height) {

		// 2. ИМЕННО [gridX][gridY], а не наоборот!
		return m_grid[gridY][gridX];
	}

	// Если алгоритм спрашивает про клетку за экраном - говорим, что там стена
	return CellType::Scenery;
}