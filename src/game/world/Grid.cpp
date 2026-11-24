#include "Grid.h"
#include "renderer/SpriteRenderer.h"
#include "textures/Texture2D.h"
#include "resources/ResourceManager.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <atomic>
#include <cmath>

namespace {
std::atomic<uint64_t> g_nextGridInstanceId{ 1 };

struct CinnabarWallCrystalGeom {
	bool hasCrystal = false;
	glm::vec2 center = glm::vec2(0.0f);
	float cScale = 0.0f;
	float baseAngle = 0.0f;
};

CinnabarWallCrystalGeom computeCinnabarWallCrystalGeom(
	glm::vec2 pixelPos,
	float cellSize,
	uint32_t seed,
	bool wallUp,
	bool wallDown,
	bool wallLeft,
	bool wallRight,
	float bevel,
	float botBevel
) {
	CinnabarWallCrystalGeom geom;
	if ((seed % 100u) >= 28u) {
		return geom;
	}

	float offX = (static_cast<float>((seed >> 10) % 29u) - 14.0f) * 0.01f * cellSize;
	float offY = (static_cast<float>((seed >> 17) % 29u) - 14.0f) * 0.01f * cellSize;
	if (!wallLeft) {
		offX += bevel * 0.45f;
	}
	if (!wallRight) {
		offX -= bevel * 0.45f;
	}
	if (!wallUp) {
		offY += bevel * 0.45f;
	}
	if (!wallDown) {
		offY -= botBevel * 0.45f;
	}

	geom.hasCrystal = true;
	geom.center = pixelPos + glm::vec2(cellSize * 0.5f + offX, cellSize * 0.5f + offY);
	geom.cScale = cellSize * (0.78f + static_cast<float>((seed >> 9) % 41u) * 0.01f);
	geom.baseAngle = static_cast<float>(static_cast<int>((seed >> 15) % 120u) - 60);
	return geom;
}

struct CinnabarCellParams {
	uint32_t hash;
	float deltaL;
	float warmShift;
};

CinnabarCellParams computeCinnabarCellParams(int x, int y) {
	uint32_t h = static_cast<uint32_t>(x) * 374761393u + static_cast<uint32_t>(y) * 668265263u;
	h = (h ^ (h >> 13)) * 1274126177u;
	h ^= (h >> 16);
	float norm1 = static_cast<float>(h & 0xFFu) / 255.0f - 0.5f;
	float norm2 = static_cast<float>((h >> 8) & 0xFFu) / 255.0f - 0.5f;
	float macro = std::sin(static_cast<float>(x) * 0.62f + static_cast<float>(y) * 0.48f) * 0.007f;
	return { h, norm1 * 0.024f + macro, norm2 * 0.009f };
}

glm::vec3 computeCinnabarFloorColor(CellType type, float deltaL, float warmShift) {
	if (type == CellType::Path) {
		return glm::vec3(
			0.236f + deltaL * 0.85f + warmShift,
			0.208f + deltaL * 0.82f,
			0.216f + deltaL * 0.85f - warmShift
		);
	}
	return glm::vec3(
		0.162f + deltaL + warmShift,
		0.173f + deltaL,
		0.208f + deltaL * 1.05f - warmShift
	);
}

uint32_t mixHash(uint32_t h) {
	h ^= h >> 16;
	h *= 0x85ebca6bu;
	h ^= h >> 13;
	h *= 0xc2b2ae35u;
	h ^= h >> 16;
	return h;
}

uint32_t hashCinnabarEdge(int x, int y, uint32_t dir) {
	uint32_t h = static_cast<uint32_t>(x) * 374761393u
		^ static_cast<uint32_t>(y) * 668265263u
		^ (dir * 2246822519u + 0x9e3779b9u);
	return mixHash(h);
}

struct CinnabarPulse {
	float alphaMult;
	float radiusMult;
	float veinBright;
};

CinnabarPulse computeCinnabarPulse(uint32_t seed, float time) {
	constexpr float kTwoPi = 6.2831853f;
	float phase1 = static_cast<float>((seed >> 3) & 0xFFu) / 255.0f * kTwoPi;
	float phase2 = static_cast<float>((seed >> 11) & 0xFFu) / 255.0f * kTwoPi;
	float speedMult = 0.78f + static_cast<float>((seed >> 19) & 0x3Fu) / 63.0f * 0.44f;

	// Медленное дыхание + редкий мягкий отблеск на некратных частотах
	float breath = 0.5f + 0.5f * std::sin(time * 0.85f * speedMult + phase1);
	float shimmerWave = 0.5f + 0.5f * std::sin(time * 2.17f * speedMult + phase2);
	float shimmer = shimmerWave * shimmerWave * (0.4f + 0.6f * shimmerWave);

	float pulse = std::clamp(breath * 0.72f + shimmer * 0.28f, 0.0f, 1.0f);
	return {
		0.65f + pulse * 0.55f,
		0.92f + pulse * 0.18f,
		0.94f + pulse * 0.13f
	};
}

float hash01(uint32_t h) {
	return static_cast<float>(h & 0xFFFFu) / 65535.0f;
}
} // namespace

// конструктор создает пустую сетку заданого размера
Grid::Grid(int width, int height, float cellSize, glm::vec2 offset) {
	m_instanceId = g_nextGridInstanceId.fetch_add(1, std::memory_order_relaxed);
	m_width = width;
	m_height = height;
	m_cellSize = cellSize;
	m_offset = offset;
	m_grid.resize(m_height, std::vector<CellType>(m_width, CellType::Ground));
	m_originalGrid.resize(m_height, std::vector<CellType>(m_width, CellType::Ground));
	m_noBuildMask.assign(m_height, std::vector<bool>(m_width, false));
}

void Grid::saveOriginalGrid() {
	m_originalGrid = m_grid;
	m_visualVersion++;
}

CellType Grid::getOriginalCellType(int gridX, int gridY) const {
	if (gridX >= 0 && gridX < m_width && gridY >= 0 && gridY < m_height) {
		return m_originalGrid[gridY][gridX];
	}
	if (m_hasSidePockets) {
		auto it = m_outerCells.find({gridX, gridY});
		if (it != m_outerCells.end()) return it->second;
		return CellType::Empty;
	}
	return CellType::Ground;
}

// Перевод из индексов клетки в пиксели экрана
glm::vec2 Grid::gridToPixel(int gridX, int gridY) const {
	return glm::vec2(gridX * m_cellSize, gridY * m_cellSize) + m_offset;
}

// Перевод из пикселей экрана в индексы клетки сетки (х,у)
glm::ivec2 Grid::pixelToGrid(glm::vec2 pixelPos) const {
	float localX = pixelPos.x - m_offset.x;
	float localY = pixelPos.y - m_offset.y;

	int gridX = static_cast<int>(std::floor(localX / m_cellSize));
	int gridY = static_cast<int>(std::floor(localY / m_cellSize));

	return glm::ivec2(gridX, gridY);
}

// Проверяем можно ли построить башню на клетке с этими индексами
bool Grid::canBuildAt(int gridX, int gridY) const {
	if (gridX >= 0 && gridX < m_width && gridY >= 0 && gridY < m_height) {
		CellType type = getCellType(gridX, gridY);
		if (type == CellType::Chasm || type == CellType::Rail) {
			return false;
		}
		if (isNoBuild(gridX, gridY)) return false;
		return (type == CellType::Ground || type == CellType::Platform);
	}

	if (m_hasSidePockets) {
		auto it = m_outerCells.find({gridX, gridY});
		if (it == m_outerCells.end()) return false;
		CellType type = it->second;
		if (type == CellType::Chasm || type == CellType::Rail || type == CellType::Empty || type == CellType::Scenery) {
			return false;
		}
		if (isNoBuild(gridX, gridY)) return false;
		return (type == CellType::Ground || type == CellType::Platform);
	}

	return false;
}

// Принудительно меняем тип конкретной ячейки
void Grid::setCellType(int gridX, int gridY, CellType type, bool bumpVersion) {
	bool changed = false;
	CellType oldType = CellType::Empty;
	CellType origType = CellType::Ground;
	if (gridX >= 0 && gridX < m_width && gridY >= 0 && gridY < m_height) {
		oldType = m_grid[gridY][gridX];
		origType = m_originalGrid[gridY][gridX];
		if (oldType != type) {
			m_grid[gridY][gridX] = type;
			changed = true;
		}
	} else if (m_hasSidePockets) {
		auto it = m_outerCells.find({gridX, gridY});
		oldType = (it != m_outerCells.end()) ? it->second : CellType::Empty;
		auto origIt = m_originalOuterCells.find({gridX, gridY});
		origType = (origIt != m_originalOuterCells.end()) ? origIt->second : CellType::Ground;
		if (type == CellType::Empty) {
			if (m_outerCells.erase({gridX, gridY}) > 0) {
				changed = true;
			}
		} else {
			if (it == m_outerCells.end() || it->second != type) {
				m_outerCells[{gridX, gridY}] = type;
				changed = true;
			}
		}
	}
	if (changed && bumpVersion) {
		m_version++;
		const bool isPureTowerToggle =
			(type == CellType::Tower && oldType == origType) ||
			(oldType == CellType::Tower && type == origType);
		if (!isPureTowerToggle) {
			m_visualVersion++;
		}
	}
}

// Отрисовка всей карты ячейка за ячейкой
void Grid::renderCellVisual(SpriteRenderer* renderer,
	const Texture2D* whiteTexture,
	int x, int y, CellType type,
	glm::vec2 pixelPos, glm::vec3 color,
	bool isMineTheme, bool isCanalTheme, bool isCinnabarTheme,
	bool includeAnimatedGlow) const
{
	glm::vec2 cellSizeVec(m_cellSize, m_cellSize);
	float borderWidth = std::max(1.0f, std::round(m_cellSize * 0.04f));
	glm::vec2 innerOffset(borderWidth, borderWidth);
	glm::vec2 innerSize(m_cellSize - 2.0f * borderWidth, m_cellSize - 2.0f * borderWidth);

	glm::vec3 fillColor;
	glm::vec3 borderColor;

	uint32_t cinnabarCellHash = 0u;
	float cinnabarDeltaL = 0.0f;
	float cinnabarWarmShift = 0.0f;
	if (isCinnabarTheme) {
		const CinnabarCellParams params = computeCinnabarCellParams(x, y);
		cinnabarCellHash = params.hash;
		cinnabarDeltaL = params.deltaL;
		cinnabarWarmShift = params.warmShift;

		if (type == CellType::Scenery) {
			renderCinnabarWallCell(renderer, whiteTexture, x, y, pixelPos, color, cinnabarCellHash, cinnabarDeltaL, cinnabarWarmShift, includeAnimatedGlow);
			return;
		}
		if (type == CellType::Platform) {
			renderCinnabarPlatformCell(renderer, whiteTexture, pixelPos, color, cinnabarCellHash, cinnabarDeltaL);
			return;
		}
		if (type == CellType::Ground || type == CellType::Path) {
			renderCinnabarFloorCell(renderer, whiteTexture, x, y, type, pixelPos, color, cinnabarCellHash, cinnabarDeltaL, cinnabarWarmShift);
			return;
		}
	}

		switch (type) {
			case CellType::Platform:
				if (isCanalTheme) {
					fillColor   = glm::vec3(0.32f, 0.58f, 0.30f);
					borderColor = glm::vec3(0.20f, 0.40f, 0.18f);
				} else if (isMineTheme) {
					fillColor   = glm::vec3(0.52f, 0.46f, 0.40f);
					borderColor = glm::vec3(0.34f, 0.30f, 0.26f);
				} else {
					fillColor   = glm::vec3(0.70f, 0.72f, 0.76f);
					borderColor = glm::vec3(0.40f, 0.42f, 0.46f);
				}
				break;
			case CellType::Ground:
				if (isCanalTheme) {
					fillColor   = glm::vec3(0.22f, 0.23f, 0.25f);
					borderColor = glm::vec3(0.14f, 0.15f, 0.17f);
				} else if (isMineTheme) {
					fillColor   = glm::vec3(0.46f, 0.29f, 0.17f);
					borderColor = glm::vec3(0.36f, 0.22f, 0.12f);
				} else {
					fillColor   = glm::vec3(0.38f, 0.65f, 0.38f);
					borderColor = glm::vec3(0.20f, 0.42f, 0.20f);
				}
				break;
			case CellType::Path:
				if (isMineTheme) {
					fillColor   = glm::vec3(0.48f, 0.31f, 0.18f);
					borderColor = glm::vec3(0.36f, 0.22f, 0.12f);
				} else {
					fillColor   = glm::vec3(0.80f, 0.70f, 0.52f);
					borderColor = glm::vec3(0.52f, 0.42f, 0.28f);
				}
				break;
			case CellType::Scenery:
				if (isMineTheme) {
					fillColor   = glm::vec3(0.52f, 0.34f, 0.19f); // Деревянный ящик в теме шахты
					borderColor = glm::vec3(0.28f, 0.16f, 0.09f);
				} else {
					fillColor   = glm::vec3(0.20f, 0.35f, 0.52f); // Классическая стена (Канал, стандарт, дефолт)
					borderColor = glm::vec3(0.11f, 0.20f, 0.32f);
				}
				break;
			case CellType::Chasm:
				fillColor   = glm::vec3(0.04f, 0.04f, 0.06f);
				borderColor = glm::vec3(0.02f, 0.02f, 0.03f);
				break;
			case CellType::Spawner:
				fillColor   = glm::vec3(0.65f, 0.35f, 0.75f);
				borderColor = glm::vec3(0.38f, 0.16f, 0.46f);
				break;
			case CellType::Base:
				fillColor   = glm::vec3(0.25f, 0.70f, 0.85f);
				borderColor = glm::vec3(0.12f, 0.42f, 0.55f);
				break;
			case CellType::Rail:
				fillColor   = glm::vec3(0.25f, 0.24f, 0.26f);
				borderColor = glm::vec3(0.14f, 0.13f, 0.16f);
				break;
			default:
				return;
		}

		// 1. Внешний квадрат (окантовка темного оттенка того же цвета)
		renderer->drawSprite(whiteTexture, pixelPos, cellSizeVec, 0.0f, borderColor * color);

		// 2. Внутренний квадрат (основной цвет ячейки)
		renderer->drawSprite(whiteTexture, pixelPos + innerOffset, innerSize, 0.0f, fillColor * color);

		// 2.1. Для темы "Вход в шахту": текстура пыли и камешков на грунте (Ground и Path)
		if (isMineTheme && (type == CellType::Ground || type == CellType::Path)) {
			uint32_t seed = static_cast<uint32_t>(std::abs(x * 7919 + y * 65537));
			float p1x = ((seed % 50) + 10) * 0.01f;
			float p1y = (((seed / 50) % 50) + 10) * 0.01f;
			float p2x = (((seed / 2500) % 50) + 20) * 0.01f;
			float p2y = (((seed / 125000) % 50) + 20) * 0.01f;
			float pSz = std::max(2.0f, std::round(m_cellSize * 0.06f));
			renderer->drawSprite(whiteTexture, pixelPos + glm::vec2(p1x * m_cellSize, p1y * m_cellSize), glm::vec2(pSz, pSz), 0.0f, glm::vec3(0.35f, 0.20f, 0.11f) * color);
			renderer->drawSprite(whiteTexture, pixelPos + glm::vec2(p2x * m_cellSize, p2y * m_cellSize), glm::vec2(pSz * 0.8f, pSz * 0.8f), 0.0f, glm::vec3(0.56f, 0.36f, 0.22f) * color);
		}

		// 2.2. Клетка-препятствие "Ящики" (Scenery в теме "Вход в шахту"):
		// Вид строго сверху: окантовка по периметру, продольные доски и диагональная планка
		if (type == CellType::Scenery && isMineTheme) {
			float frameThick = std::max(3.0f, std::round(m_cellSize * 0.12f));
			glm::vec3 darkWood = glm::vec3(0.26f, 0.15f, 0.08f) * color;
			glm::vec3 plankCol1 = glm::vec3(0.55f, 0.35f, 0.20f) * color;
			glm::vec3 plankCol2 = glm::vec3(0.48f, 0.30f, 0.17f) * color;
			glm::vec3 braceCol = glm::vec3(0.44f, 0.27f, 0.15f) * color;
			glm::vec3 metalCol = glm::vec3(0.38f, 0.40f, 0.44f) * color;
			glm::vec3 rivetCol = glm::vec3(0.85f, 0.86f, 0.90f) * color;

			glm::vec2 boxInnerPos = pixelPos + glm::vec2(frameThick);
			glm::vec2 boxInnerSize(m_cellSize - 2.0f * frameThick, m_cellSize - 2.0f * frameThick);

			float plankH = boxInnerSize.y / 3.0f;
			for (int p = 0; p < 3; ++p) {
				glm::vec3 pCol = (p == 1) ? plankCol1 : plankCol2;
				renderer->drawSprite(whiteTexture, glm::vec2(boxInnerPos.x, boxInnerPos.y + static_cast<float>(p) * plankH), glm::vec2(boxInnerSize.x, plankH), 0.0f, pCol);
				if (p > 0) {
					renderer->drawSprite(whiteTexture, glm::vec2(boxInnerPos.x, boxInnerPos.y + static_cast<float>(p) * plankH - 1.0f), glm::vec2(boxInnerSize.x, 2.0f), 0.0f, darkWood);
				}
			}

			float diagLen = m_cellSize * 1.15f;
			float diagThick = std::max(4.0f, std::round(m_cellSize * 0.13f));
			glm::vec2 centerPos = pixelPos + glm::vec2(m_cellSize * 0.5f);
			renderer->drawSprite(whiteTexture, centerPos - glm::vec2(diagLen * 0.5f, diagThick * 0.5f) + glm::vec2(1.5f, 1.5f), glm::vec2(diagLen, diagThick), 45.0f, darkWood);
			renderer->drawSprite(whiteTexture, centerPos - glm::vec2(diagLen * 0.5f, diagThick * 0.5f), glm::vec2(diagLen, diagThick), 45.0f, braceCol);

			renderer->drawSprite(whiteTexture, pixelPos, glm::vec2(m_cellSize, frameThick), 0.0f, darkWood);
			renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + m_cellSize - frameThick), glm::vec2(m_cellSize, frameThick), 0.0f, darkWood);
			renderer->drawSprite(whiteTexture, pixelPos, glm::vec2(frameThick, m_cellSize), 0.0f, darkWood);
			renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + m_cellSize - frameThick, pixelPos.y), glm::vec2(frameThick, m_cellSize), 0.0f, darkWood);

			float bracketSz = std::max(5.0f, std::round(m_cellSize * 0.16f));
			float rivetSz = std::max(2.0f, std::round(m_cellSize * 0.05f));
		auto drawCornerBracket = [&](glm::vec2 bPos, glm::vec2 rPos) {
				renderer->drawSprite(whiteTexture, bPos, glm::vec2(bracketSz), 0.0f, metalCol);
				renderer->drawSprite(whiteTexture, rPos, glm::vec2(rivetSz), 0.0f, rivetCol);
			};
			drawCornerBracket(pixelPos, pixelPos + glm::vec2(bracketSz * 0.25f));
			drawCornerBracket(glm::vec2(pixelPos.x + m_cellSize - bracketSz, pixelPos.y), glm::vec2(pixelPos.x + m_cellSize - bracketSz * 0.75f, pixelPos.y + bracketSz * 0.25f));
			drawCornerBracket(glm::vec2(pixelPos.x, pixelPos.y + m_cellSize - bracketSz), glm::vec2(pixelPos.x + bracketSz * 0.25f, pixelPos.y + m_cellSize - bracketSz * 0.75f));
			drawCornerBracket(glm::vec2(pixelPos.x + m_cellSize - bracketSz, pixelPos.y + m_cellSize - bracketSz), glm::vec2(pixelPos.x + m_cellSize - bracketSz * 0.75f, pixelPos.y + m_cellSize - bracketSz * 0.75f));
		}

		// 3. Для клетки обрыва (Chasm) рисуем глубокую сердцевину провала
		if (type == CellType::Chasm) {
			float depthPadding = std::max(2.0f, std::round(m_cellSize * 0.16f));
			glm::vec2 pitOffset(depthPadding, depthPadding);
			glm::vec2 pitSize(m_cellSize - 2.0f * depthPadding, m_cellSize - 2.0f * depthPadding);
			if (pitSize.x > 0.0f && pitSize.y > 0.0f) {
				renderer->drawSprite(whiteTexture, pixelPos + pitOffset, pitSize, 0.0f, glm::vec3(0.015f, 0.015f, 0.025f) * color);
			}
		}

		// 4. Для клетки рельсов (Rail) рисуем стилизованную колію с авто-тайлингом
		if (type == CellType::Rail) {
			bool up    = (getCellType(x, y - 1) == CellType::Rail);
			bool down  = (getCellType(x, y + 1) == CellType::Rail);
			bool left  = (getCellType(x - 1, y) == CellType::Rail);
			bool right = (getCellType(x + 1, y) == CellType::Rail);

			bool hasVert  = (up || down);
			bool hasHoriz = (left || right);

			float railOffset = std::max(3.0f, std::round(m_cellSize * 0.22f));
			float railThick  = std::max(2.0f, std::round(m_cellSize * 0.08f));
			float tiePadding = std::max(1.0f, std::round(m_cellSize * 0.08f));
			float tieThick   = std::max(2.0f, std::round(m_cellSize * 0.12f));

			glm::vec3 woodCol  = glm::vec3(0.42f, 0.28f, 0.18f) * color;
			glm::vec3 steelCol = glm::vec3(0.80f, 0.82f, 0.88f) * color;

			float r1 = railOffset;
			float r2 = m_cellSize - railOffset - railThick;

			if (!hasVert && !hasHoriz) {
				for (int i = 0; i < 3; ++i) {
					float tieX = pixelPos.x + (m_cellSize * 0.25f) * (static_cast<float>(i) + 0.5f);
					renderer->drawSprite(whiteTexture, glm::vec2(tieX, pixelPos.y + tiePadding),
										 glm::vec2(tieThick, m_cellSize - 2.0f * tiePadding), 0.0f, woodCol);
				}
				renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + r1), glm::vec2(m_cellSize, railThick), 0.0f, steelCol);
				renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + r2), glm::vec2(m_cellSize, railThick), 0.0f, steelCol);
			} else if (hasVert && !hasHoriz) {
				for (int i = 0; i < 3; ++i) {
					float tieY = pixelPos.y + (m_cellSize * 0.25f) * (static_cast<float>(i) + 0.5f);
					renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + tiePadding, tieY),
										 glm::vec2(m_cellSize - 2.0f * tiePadding, tieThick), 0.0f, woodCol);
				}
				renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r1, pixelPos.y), glm::vec2(railThick, m_cellSize), 0.0f, steelCol);
				renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r2, pixelPos.y), glm::vec2(railThick, m_cellSize), 0.0f, steelCol);
			} else if (hasHoriz && !hasVert) {
				for (int i = 0; i < 3; ++i) {
					float tieX = pixelPos.x + (m_cellSize * 0.25f) * (static_cast<float>(i) + 0.5f);
					renderer->drawSprite(whiteTexture, glm::vec2(tieX, pixelPos.y + tiePadding),
										 glm::vec2(tieThick, m_cellSize - 2.0f * tiePadding), 0.0f, woodCol);
				}
				renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + r1), glm::vec2(m_cellSize, railThick), 0.0f, steelCol);
				renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + r2), glm::vec2(m_cellSize, railThick), 0.0f, steelCol);
			} else {
				if (up) {
					renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + tiePadding, pixelPos.y + m_cellSize * 0.15f),
										 glm::vec2(m_cellSize - 2.0f * tiePadding, tieThick), 0.0f, woodCol);
				}
				if (down) {
					renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + tiePadding, pixelPos.y + m_cellSize * 0.73f),
										 glm::vec2(m_cellSize - 2.0f * tiePadding, tieThick), 0.0f, woodCol);
				}
				if (left) {
					renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + m_cellSize * 0.15f, pixelPos.y + tiePadding),
										 glm::vec2(tieThick, m_cellSize - 2.0f * tiePadding), 0.0f, woodCol);
				}
				if (right) {
					renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + m_cellSize * 0.73f, pixelPos.y + tiePadding),
										 glm::vec2(tieThick, m_cellSize - 2.0f * tiePadding), 0.0f, woodCol);
				}
				float diagSz = std::max(4.0f, std::round(m_cellSize * 0.26f));
				renderer->drawSprite(whiteTexture, pixelPos + glm::vec2((m_cellSize - diagSz) * 0.5f),
									 glm::vec2(diagSz), 0.0f, woodCol);

				if (down && right && !up && !left) {
					renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r1, pixelPos.y + r1), glm::vec2(railThick, m_cellSize - r1), 0.0f, steelCol);
					renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r1, pixelPos.y + r1), glm::vec2(m_cellSize - r1, railThick), 0.0f, steelCol);
				} else if (down && left && !up && !right) {
					renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r2, pixelPos.y + r1), glm::vec2(railThick, m_cellSize - r1), 0.0f, steelCol);
					renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r1, pixelPos.y + r1), glm::vec2(r2 + railThick, railThick), 0.0f, steelCol);
				} else if (up && right && !down && !left) {
					renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r1, pixelPos.y), glm::vec2(railThick, r1 + railThick), 0.0f, steelCol);
					renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r1, pixelPos.y + r2), glm::vec2(m_cellSize - r1, railThick), 0.0f, steelCol);
				} else if (up && left && !down && !right) {
					renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r2, pixelPos.y), glm::vec2(railThick, r1 + railThick), 0.0f, steelCol);
					renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + r2), glm::vec2(r2 + railThick, railThick), 0.0f, steelCol);
				} else {
					if (up) {
						renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r1, pixelPos.y), glm::vec2(railThick, m_cellSize * 0.5f), 0.0f, steelCol);
						renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r2, pixelPos.y), glm::vec2(railThick, m_cellSize * 0.5f), 0.0f, steelCol);
					}
					if (down) {
						renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r1, pixelPos.y + m_cellSize * 0.5f), glm::vec2(railThick, m_cellSize * 0.5f), 0.0f, steelCol);
						renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + r2, pixelPos.y + m_cellSize * 0.5f), glm::vec2(railThick, m_cellSize * 0.5f), 0.0f, steelCol);
					}
					if (left) {
						renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + r1), glm::vec2(m_cellSize * 0.5f, railThick), 0.0f, steelCol);
						renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + r2), glm::vec2(m_cellSize * 0.5f, railThick), 0.0f, steelCol);
					}
					if (right) {
						renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + m_cellSize * 0.5f, pixelPos.y + r1), glm::vec2(m_cellSize * 0.5f, railThick), 0.0f, steelCol);
						renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + m_cellSize * 0.5f, pixelPos.y + r2), glm::vec2(m_cellSize * 0.5f, railThick), 0.0f, steelCol);
					}
				}
			}
		}
	}

void Grid::renderCinnabarWallCell(SpriteRenderer* renderer,
	const Texture2D* whiteTexture,
	int x, int y, glm::vec2 pixelPos, glm::vec3 color,
	uint32_t seed, float deltaL, float warmShift,
	bool includeAnimatedGlow) const
{
	auto isRockWall = [&](int nx, int ny) -> bool {
		return getCellType(nx, ny) == CellType::Scenery;
	};
	const bool wallUp        = isRockWall(x, y - 1);
	const bool wallDown      = isRockWall(x, y + 1);
	const bool wallLeft      = isRockWall(x - 1, y);
	const bool wallRight     = isRockWall(x + 1, y);
	const bool wallUpLeft    = isRockWall(x - 1, y - 1);
	const bool wallUpRight   = isRockWall(x + 1, y - 1);
	const bool wallDownLeft  = isRockWall(x - 1, y + 1);
	const bool wallDownRight = isRockWall(x + 1, y + 1);

	// 1. Мягкая падающая тень и контактный AO только на соседние не-стеновые клетки
	float shadowDepth = std::max(4.0f, std::round(m_cellSize * 0.16f));
	float shadowNear  = std::max(2.0f, std::round(shadowDepth * 0.52f));
	float aoThin      = std::max(2.0f, std::round(shadowDepth * 0.36f));
	glm::vec3 shadowRgb = glm::vec3(0.015f, 0.015f, 0.024f) * color;

	if (!wallDown) {
		renderer->drawSpriteRGBA(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + m_cellSize), glm::vec2(m_cellSize, shadowDepth), 0.0f, glm::vec4(shadowRgb, 0.18f));
		renderer->drawSpriteRGBA(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + m_cellSize), glm::vec2(m_cellSize, shadowNear), 0.0f, glm::vec4(shadowRgb, 0.26f));
	}
	if (!wallRight) {
		renderer->drawSpriteRGBA(whiteTexture, glm::vec2(pixelPos.x + m_cellSize, pixelPos.y), glm::vec2(shadowDepth, m_cellSize), 0.0f, glm::vec4(shadowRgb, 0.15f));
		renderer->drawSpriteRGBA(whiteTexture, glm::vec2(pixelPos.x + m_cellSize, pixelPos.y), glm::vec2(shadowNear, m_cellSize), 0.0f, glm::vec4(shadowRgb, 0.22f));
	}
	if (!wallDown && !wallRight && !wallDownRight) {
		renderer->drawSpriteRGBA(whiteTexture, pixelPos + glm::vec2(m_cellSize, m_cellSize), glm::vec2(shadowDepth, shadowDepth), 0.0f, glm::vec4(shadowRgb, 0.11f));
		renderer->drawSpriteRGBA(whiteTexture, pixelPos + glm::vec2(m_cellSize, m_cellSize), glm::vec2(shadowNear, shadowNear), 0.0f, glm::vec4(shadowRgb, 0.18f));
	}
	if (!wallUp) {
		renderer->drawSpriteRGBA(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y - aoThin), glm::vec2(m_cellSize, aoThin), 0.0f, glm::vec4(shadowRgb, 0.16f));
	}
	if (!wallLeft) {
		renderer->drawSpriteRGBA(whiteTexture, glm::vec2(pixelPos.x - aoThin, pixelPos.y), glm::vec2(aoThin, m_cellSize), 0.0f, glm::vec4(shadowRgb, 0.14f));
	}

	// 2. Единый тёмный массив скальной породы без внутренних межклеточных швов
	float rockVar = deltaL * 0.45f;
	glm::vec3 rockBase(
		std::max(0.04f, 0.088f + rockVar + warmShift * 0.4f),
		std::max(0.04f, 0.092f + rockVar),
		std::max(0.05f, 0.114f + rockVar * 1.1f - warmShift * 0.4f)
	);
	renderer->drawSprite(whiteTexture, pixelPos, glm::vec2(m_cellSize, m_cellSize), 0.0f, rockBase * color);

	// Лёгкая геологическая неоднородность породы внутри клетки по хэшу
	float patchW = m_cellSize * (0.36f + static_cast<float>((seed >> 6) % 17u) * 0.01f);
	float patchH = m_cellSize * (0.28f + static_cast<float>((seed >> 11) % 17u) * 0.01f);
	float patchMaxX = std::max(0.0f, m_cellSize - patchW);
	float patchMaxY = std::max(0.0f, m_cellSize - patchH);
	float patchOffX = patchMaxX * (static_cast<float>((seed >> 14) % 100u) * 0.01f);
	float patchOffY = patchMaxY * (static_cast<float>((seed >> 20) % 100u) * 0.01f);
	glm::vec4 patchCol;
	if (((seed >> 5) & 1u) == 0u) {
		patchCol = glm::vec4(rockBase * 1.14f * color, 0.22f);
	} else {
		patchCol = glm::vec4(rockBase * 0.80f * color, 0.25f);
	}
	renderer->drawSpriteRGBA(whiteTexture, pixelPos + glm::vec2(patchOffX, patchOffY), glm::vec2(patchW, patchH), 0.0f, patchCol);

	// Мелкие трещины в породе по хэшу координат (примерно в ~65% клеток)
	if (((seed >> 3) % 10u) < 7u) {
		float cx = pixelPos.x + m_cellSize * (0.28f + static_cast<float>((seed >> 16) % 44u) * 0.01f);
		float cy = pixelPos.y + m_cellSize * (0.28f + static_cast<float>((seed >> 22) % 44u) * 0.01f);
		float len1 = m_cellSize * (0.20f + static_cast<float>((seed >> 9) % 14u) * 0.01f);
		float len2 = m_cellSize * (0.14f + static_cast<float>((seed >> 13) % 10u) * 0.01f);
		float thick = std::max(1.0f, m_cellSize * 0.022f);
		float ang1 = static_cast<float>(static_cast<int>((seed >> 18) % 140u) - 70);
		float ang2Delta = (((seed >> 25) & 1u) == 0u) ? 38.0f : -36.0f;
		float ang2 = ang1 + ang2Delta;
		glm::vec4 crackCol(0.032f * color.r, 0.034f * color.g, 0.046f * color.b, 0.72f);

		glm::vec2 seg1Center(cx, cy);
		glm::vec2 seg1Size(len1, thick);
		renderer->drawSpriteRGBA(whiteTexture, seg1Center - seg1Size * 0.5f, seg1Size, ang1, crackCol);

		float rad1 = glm::radians(ang1);
		glm::vec2 seg2Center = seg1Center + glm::vec2(std::cos(rad1), std::sin(rad1)) * (len1 * 0.42f);
		glm::vec2 seg2Size(len2, thick);
		renderer->drawSpriteRGBA(whiteTexture, seg2Center - seg2Size * 0.5f, seg2Size, ang2, crackCol);
	}

	if (((seed >> 4) % 5u) < 2u) {
		float speckSz = std::max(1.5f, std::round(m_cellSize * 0.038f));
		float sx = pixelPos.x + m_cellSize * (0.18f + static_cast<float>((seed >> 12) % 60u) * 0.01f);
		float sy = pixelPos.y + m_cellSize * (0.18f + static_cast<float>((seed >> 19) % 60u) * 0.01f);
		renderer->drawSpriteRGBA(whiteTexture, glm::vec2(sx, sy), glm::vec2(speckSz, speckSz), 0.0f, glm::vec4(rockBase * 1.25f * color, 0.35f));
	}

	// 3. Выразительная каменная кромка/фаска ТОЛЬКО на внешних открытых границах
	float bevel    = std::max(3.0f, std::round(m_cellSize * 0.11f));
	float botBevel = std::max(4.0f, std::round(m_cellSize * 0.13f));
	float rim      = std::max(1.0f, std::round(m_cellSize * 0.035f));
	float contour  = std::max(1.0f, std::round(m_cellSize * 0.028f));

	glm::vec3 contourCol   = glm::vec3(0.028f, 0.030f, 0.042f) * color;
	glm::vec3 topLitRim    = glm::vec3(0.205f, 0.218f, 0.262f) * color;
	glm::vec3 leftLitRim   = glm::vec3(0.168f, 0.180f, 0.220f) * color;
	glm::vec3 topSlopeCol  = glm::vec3(0.132f, 0.140f, 0.172f) * color;
	glm::vec3 botCliffCol  = glm::vec3(0.048f, 0.052f, 0.068f) * color;
	glm::vec3 rightFaceCol = glm::vec3(0.060f, 0.064f, 0.084f) * color;

	if (!wallUp) {
		renderer->drawSprite(whiteTexture, pixelPos, glm::vec2(m_cellSize, bevel), 0.0f, topSlopeCol);
	}
	if (!wallLeft) {
		renderer->drawSprite(whiteTexture, pixelPos, glm::vec2(bevel, m_cellSize), 0.0f, topSlopeCol * 0.94f);
	}
	if (!wallRight) {
		renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + m_cellSize - bevel, pixelPos.y), glm::vec2(bevel, m_cellSize), 0.0f, rightFaceCol);
	}
	if (!wallDown) {
		renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + m_cellSize - botBevel), glm::vec2(m_cellSize, botBevel), 0.0f, botCliffCol);
		renderer->drawSpriteRGBA(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + m_cellSize - botBevel), glm::vec2(m_cellSize, 1.0f), 0.0f, glm::vec4(0.155f * color.r, 0.165f * color.g, 0.205f * color.b, 0.50f));
	}

	// Световые кромки поверх откосов, затем внешний тёмный контур
	if (!wallUp) {
		renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + contour), glm::vec2(m_cellSize, rim), 0.0f, topLitRim);
	}
	if (!wallLeft) {
		renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + contour, pixelPos.y), glm::vec2(rim, m_cellSize), 0.0f, leftLitRim);
	}

	if (!wallUp) {
		renderer->drawSprite(whiteTexture, pixelPos, glm::vec2(m_cellSize, contour), 0.0f, contourCol);
	}
	if (!wallLeft) {
		renderer->drawSprite(whiteTexture, pixelPos, glm::vec2(contour, m_cellSize), 0.0f, contourCol);
	}
	if (!wallRight) {
		renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + m_cellSize - contour, pixelPos.y), glm::vec2(contour, m_cellSize), 0.0f, contourCol);
	}
	if (!wallDown) {
		renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + m_cellSize - contour), glm::vec2(m_cellSize, contour), 0.0f, contourCol);
	}

	// Внутренние вогнутые углы (когда два ортогональных соседа — стены, а диагональный — нет)
	if (wallUp && wallLeft && !wallUpLeft) {
		renderer->drawSprite(whiteTexture, pixelPos, glm::vec2(bevel, bevel), 0.0f, topSlopeCol);
		renderer->drawSprite(whiteTexture, pixelPos + glm::vec2(contour, contour), glm::vec2(rim, rim), 0.0f, leftLitRim);
		renderer->drawSprite(whiteTexture, pixelPos, glm::vec2(contour, contour), 0.0f, contourCol);
	}
	if (wallUp && wallRight && !wallUpRight) {
		renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + m_cellSize - bevel, pixelPos.y), glm::vec2(bevel, bevel), 0.0f, rightFaceCol);
		renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + m_cellSize - contour, pixelPos.y), glm::vec2(contour, contour), 0.0f, contourCol);
	}
	if (wallDown && wallLeft && !wallDownLeft) {
		renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + m_cellSize - botBevel), glm::vec2(bevel, botBevel), 0.0f, botCliffCol);
		renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + m_cellSize - contour), glm::vec2(contour, contour), 0.0f, contourCol);
	}
	if (wallDown && wallRight && !wallDownRight) {
		renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + m_cellSize - bevel, pixelPos.y + m_cellSize - botBevel), glm::vec2(bevel, botBevel), 0.0f, botCliffCol);
		renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + m_cellSize - contour, pixelPos.y + m_cellSize - contour), glm::vec2(contour, contour), 0.0f, contourCol);
	}

	// 4. Редкие вкрапления кристаллов и жил киновари (~28% клеток) с тусклым красным свечением
	const CinnabarWallCrystalGeom crystal = computeCinnabarWallCrystalGeom(
		pixelPos, m_cellSize, seed, wallUp, wallDown, wallLeft, wallRight, bevel, botBevel);
	if (crystal.hasCrystal) {
		const glm::vec2 crystalCenter = crystal.center;
		const float cScale = crystal.cScale;
		const float baseAngle = crystal.baseAngle;

		glm::vec3 veinTint = color;
		if (includeAnimatedGlow) {
			const float time = static_cast<float>(glfwGetTime());
			const CinnabarPulse pulse = computeCinnabarPulse(seed, time);
			veinTint = color * pulse.veinBright;

			// Мягкое тусклое свечение киновари в толще сланца
			glm::vec2 outerGlowSz(cScale * 0.56f * pulse.radiusMult, cScale * 0.42f * pulse.radiusMult);
			glm::vec2 innerGlowSz(cScale * 0.34f * pulse.radiusMult, cScale * 0.25f * pulse.radiusMult);
			renderer->drawSpriteRGBA(whiteTexture, crystalCenter - outerGlowSz * 0.5f, outerGlowSz, baseAngle, glm::vec4(0.56f * color.r, 0.08f * color.g, 0.10f * color.b, 0.14f * pulse.alphaMult));
			renderer->drawSpriteRGBA(whiteTexture, crystalCenter - innerGlowSz * 0.5f, innerGlowSz, baseAngle, glm::vec4(0.76f * color.r, 0.13f * color.g, 0.15f * color.b, 0.22f * pulse.alphaMult));
		}

		uint32_t variant = (seed >> 7) % 3u;
		if (variant == 0u) {
			// Вариант 1: короткая ломаная прожилка киновари
			float len1 = cScale * 0.28f;
			float len2 = cScale * 0.22f;
			float thick = std::max(2.0f, cScale * 0.075f);
			float a1 = baseAngle - 10.0f;
			float a2 = baseAngle + 18.0f;
			float r1 = glm::radians(a1);
			glm::vec2 p1 = crystalCenter - glm::vec2(std::cos(r1), std::sin(r1)) * (len1 * 0.22f);
			glm::vec2 p2 = crystalCenter + glm::vec2(std::cos(r1), std::sin(r1)) * (len1 * 0.28f);

			glm::vec2 crevSz1(len1 + 3.0f, thick + 2.5f);
			glm::vec2 crevSz2(len2 + 3.0f, thick + 2.0f);
			renderer->drawSprite(whiteTexture, p1 - crevSz1 * 0.5f, crevSz1, a1, glm::vec3(0.032f, 0.022f, 0.032f) * color);
			renderer->drawSprite(whiteTexture, p2 - crevSz2 * 0.5f, crevSz2, a2, glm::vec3(0.032f, 0.022f, 0.032f) * color);

			glm::vec2 vSz1(len1, thick);
			glm::vec2 vSz2(len2, std::max(1.8f, thick * 0.85f));
			renderer->drawSprite(whiteTexture, p1 - vSz1 * 0.5f, vSz1, a1, glm::vec3(0.52f, 0.09f, 0.12f) * veinTint);
			renderer->drawSprite(whiteTexture, p2 - vSz2 * 0.5f, vSz2, a2, glm::vec3(0.74f, 0.15f, 0.17f) * veinTint);

			glm::vec2 coreSz(len1 * 0.40f, std::max(1.5f, thick * 0.48f));
			renderer->drawSprite(whiteTexture, p2 - coreSz * 0.5f, coreSz, a2, glm::vec3(0.90f, 0.27f, 0.27f) * veinTint);
		} else if (variant == 1u) {
			// Вариант 2: друза из 3 угловатых кристаллов
			glm::vec2 cavSz(cScale * 0.26f, cScale * 0.20f);
			renderer->drawSprite(whiteTexture, crystalCenter - cavSz * 0.5f, cavSz, baseAngle, glm::vec3(0.030f, 0.022f, 0.032f) * color);

			glm::vec2 s2Offset(cScale * 0.06f, -cScale * 0.04f);
			glm::vec2 s3Offset(-cScale * 0.06f, cScale * 0.04f);
			glm::vec2 s2Sz(cScale * 0.14f, cScale * 0.085f);
			glm::vec2 s3Sz(cScale * 0.11f, cScale * 0.075f);
			glm::vec2 s1Sz(cScale * 0.18f, cScale * 0.10f);

			renderer->drawSprite(whiteTexture, (crystalCenter + s2Offset) - s2Sz * 0.5f, s2Sz, baseAngle + 48.0f, glm::vec3(0.54f, 0.09f, 0.12f) * veinTint);
			renderer->drawSprite(whiteTexture, (crystalCenter + s3Offset) - s3Sz * 0.5f, s3Sz, baseAngle - 42.0f, glm::vec3(0.64f, 0.12f, 0.14f) * veinTint);
			renderer->drawSprite(whiteTexture, crystalCenter - s1Sz * 0.5f, s1Sz, baseAngle, glm::vec3(0.72f, 0.14f, 0.16f) * veinTint);

			glm::vec2 glintSz(std::max(2.0f, cScale * 0.06f), std::max(1.5f, cScale * 0.04f));
			renderer->drawSprite(whiteTexture, crystalCenter - glintSz * 0.5f, glintSz, baseAngle, glm::vec3(0.92f, 0.28f, 0.28f) * veinTint);
		} else {
			// Вариант 3: компактный кристалл-вкрапление + осколок + редкая капля ртути
			float dAngle = 45.0f + baseAngle * 0.3f;
			glm::vec2 rimSz(cScale * 0.18f, cScale * 0.18f);
			glm::vec2 bodySz(cScale * 0.15f, cScale * 0.15f);
			glm::vec2 facetSz(cScale * 0.075f, cScale * 0.075f);

			renderer->drawSprite(whiteTexture, crystalCenter - rimSz * 0.5f, rimSz, dAngle, glm::vec3(0.030f, 0.022f, 0.032f) * color);
			renderer->drawSprite(whiteTexture, crystalCenter - bodySz * 0.5f, bodySz, dAngle, glm::vec3(0.66f, 0.12f, 0.15f) * veinTint);
			renderer->drawSprite(whiteTexture, crystalCenter - facetSz * 0.5f, facetSz, dAngle, glm::vec3(0.88f, 0.24f, 0.25f) * veinTint);

			float fleckSz = std::max(2.0f, cScale * 0.065f);
			glm::vec2 fleckCenter = crystalCenter + glm::vec2(cScale * 0.14f, -cScale * 0.10f);
			renderer->drawSprite(whiteTexture, fleckCenter - glm::vec2(fleckSz * 0.5f), glm::vec2(fleckSz, fleckSz), dAngle + 20.0f, glm::vec3(0.60f, 0.11f, 0.14f) * veinTint);

			if (((seed >> 21) % 3u) == 0u) {
				float hgSz = std::max(2.0f, std::round(m_cellSize * 0.055f));
				glm::vec2 hgCenter = crystalCenter + glm::vec2(-cScale * 0.13f, cScale * 0.11f);
				renderer->drawSprite(whiteTexture, hgCenter - glm::vec2(hgSz * 0.5f), glm::vec2(hgSz, hgSz), 0.0f, glm::vec3(0.78f, 0.84f, 0.90f) * color);
			}
		}
	}
}

void Grid::renderCinnabarPlatformCell(SpriteRenderer* renderer,
	const Texture2D* whiteTexture,
	glm::vec2 pixelPos, glm::vec3 color,
	uint32_t seed, float deltaL) const
{
	float slotGap = std::max(1.5f, std::round(m_cellSize * 0.045f));
	float stepH   = std::max(3.0f, std::round(m_cellSize * 0.105f));
	glm::vec3 creviceCol = glm::vec3(0.088f, 0.078f, 0.080f) * color;

	// 1. Чёткое разделение соседних слотов и мягкая тень снизу под вырубленной ступенькой
	renderer->drawSprite(whiteTexture, pixelPos, glm::vec2(m_cellSize, m_cellSize), 0.0f, creviceCol);
	renderer->drawSpriteRGBA(
		whiteTexture,
		pixelPos + glm::vec2(slotGap, m_cellSize),
		glm::vec2(m_cellSize - 2.0f * slotGap, std::max(2.0f, std::round(m_cellSize * 0.065f))),
		0.0f,
		glm::vec4(0.015f * color.r, 0.012f * color.g, 0.018f * color.b, 0.28f)
	);

	// 2. Грубый сколотый передний торец ступеньки породы
	float slabW = m_cellSize - 2.0f * slotGap;
	float riserY = pixelPos.y + m_cellSize - slotGap - stepH;
	if (slabW > 4.0f && stepH > 1.0f) {
		float f1 = 0.34f + (static_cast<float>((seed >> 1) & 0x7u) - 3.5f) * 0.01f;
		float f2 = 0.36f + (static_cast<float>((seed >> 4) & 0x7u) - 3.5f) * 0.01f;
		float w1 = slabW * f1;
		float w2 = slabW * f2;
		float w3 = std::max(1.0f, slabW - w1 - w2);
		float x0 = pixelPos.x + slotGap;

		renderer->drawSprite(whiteTexture, glm::vec2(x0, riserY), glm::vec2(w1, stepH), 0.0f, glm::vec3(0.136f, 0.114f, 0.106f) * color);
		renderer->drawSprite(whiteTexture, glm::vec2(x0 + w1, riserY), glm::vec2(w2, stepH), 0.0f, glm::vec3(0.160f, 0.135f, 0.122f) * color);
		renderer->drawSprite(whiteTexture, glm::vec2(x0 + w1 + w2, riserY), glm::vec2(w3, stepH), 0.0f, glm::vec3(0.142f, 0.118f, 0.110f) * color);
	}

	// 3. Тёплая пыльно-каменная / сланцево-бурая верхняя поверхность уступа
	glm::vec2 slabPos(pixelPos.x + slotGap, pixelPos.y + slotGap);
	glm::vec2 slabSize(slabW, m_cellSize - 2.0f * slotGap - stepH * 0.72f);
	if (slabSize.x <= 4.0f || slabSize.y <= 4.0f) {
		return;
	}

	float sVar = deltaL * 0.48f;
	float warmJitter = (static_cast<float>((seed >> 4) & 0x1Fu) / 31.0f - 0.5f) * 0.014f;
	glm::vec3 stoneBase(
		0.328f + sVar + warmJitter,
		0.274f + sVar * 0.92f,
		0.228f + sVar * 0.80f - warmJitter * 0.5f
	);
	renderer->drawSprite(whiteTexture, slabPos, slabSize, 0.0f, stoneBase * color);

	// Лёгкая пыльно-минеральная неоднородность породы внутри площадки
	float mottleW = slabSize.x * (0.40f + static_cast<float>((seed >> 9) % 18u) * 0.01f);
	float mottleH = slabSize.y * (0.34f + static_cast<float>((seed >> 13) % 18u) * 0.01f);
	float mottleX = slabPos.x + (slabSize.x - mottleW) * (static_cast<float>((seed >> 17) % 100u) * 0.01f);
	float mottleY = slabPos.y + (slabSize.y - mottleH) * (static_cast<float>((seed >> 22) % 100u) * 0.01f);
	glm::vec4 mottleCol;
	if (((seed >> 3) & 1u) == 0u) {
		mottleCol = glm::vec4(stoneBase.r * 1.12f * color.r, stoneBase.g * 1.10f * color.g, stoneBase.b * 1.08f * color.b, 0.20f);
	} else {
		mottleCol = glm::vec4(stoneBase.r * 0.84f * color.r, stoneBase.g * 0.84f * color.g, stoneBase.b * 0.86f * color.b, 0.20f);
	}
	renderer->drawSpriteRGBA(whiteTexture, glm::vec2(mottleX, mottleY), glm::vec2(mottleW, mottleH), 0.0f, mottleCol);

	// 4. Неровный вырубленный край породы (сегментированные фаски и сколы углов по хэшу)
	float bevel = std::max(2.0f, std::round(m_cellSize * 0.068f));
	glm::vec3 topLitCol   = glm::vec3(0.405f + sVar * 0.6f, 0.348f + sVar * 0.5f, 0.292f + sVar * 0.4f) * color;
	glm::vec3 leftLitCol  = glm::vec3(0.365f + sVar * 0.5f, 0.312f + sVar * 0.4f, 0.264f + sVar * 0.4f) * color;
	glm::vec3 rightShdCol = glm::vec3(0.215f, 0.180f, 0.156f) * color;
	glm::vec3 botShdCol   = glm::vec3(0.195f, 0.162f, 0.142f) * color;

	auto segFactor = [&](uint32_t shift, float minF, float spanF) -> float {
		return minF + (static_cast<float>((seed >> shift) & 0xFu) / 15.0f) * spanF;
	};

	// Верхняя освещённая кромка (3 неровных сегмента)
	float topFracs[3] = { 0.33f, 0.34f, 0.33f };
	float cursorX = slabPos.x;
	for (int i = 0; i < 3; ++i) {
		float segW;
		if (i < 2) {
			segW = slabSize.x * topFracs[i];
		} else {
			segW = (slabPos.x + slabSize.x) - cursorX;
		}
		float segThick = std::max(1.5f, bevel * segFactor(static_cast<uint32_t>(i * 4 + 2), 0.72f, 0.56f));
		float bright   = segFactor(static_cast<uint32_t>(i * 4 + 14), 0.94f, 0.11f);
		renderer->drawSprite(whiteTexture, glm::vec2(cursorX, slabPos.y), glm::vec2(segW, segThick), 0.0f, topLitCol * bright);
		cursorX += segW;
	}

	// Левая освещённая кромка (2 сегмента)
	float leftMidY = slabSize.y * segFactor(6u, 0.42f, 0.18f);
	float leftW1 = std::max(1.5f, bevel * segFactor(10u, 0.75f, 0.45f));
	float leftW2 = std::max(1.5f, bevel * segFactor(18u, 0.75f, 0.45f));
	renderer->drawSprite(whiteTexture, slabPos, glm::vec2(leftW1, leftMidY), 0.0f, leftLitCol);
	renderer->drawSprite(whiteTexture, glm::vec2(slabPos.x, slabPos.y + leftMidY), glm::vec2(leftW2, slabSize.y - leftMidY), 0.0f, leftLitCol * 0.95f);

	// Правая затенённая кромка (2 сегмента)
	float rightMidY = slabSize.y * segFactor(8u, 0.40f, 0.20f);
	float rightW1 = std::max(1.5f, bevel * segFactor(12u, 0.75f, 0.50f));
	float rightW2 = std::max(1.5f, bevel * segFactor(20u, 0.75f, 0.50f));
	renderer->drawSprite(whiteTexture, glm::vec2(slabPos.x + slabSize.x - rightW1, slabPos.y), glm::vec2(rightW1, rightMidY), 0.0f, rightShdCol);
	renderer->drawSprite(whiteTexture, glm::vec2(slabPos.x + slabSize.x - rightW2, slabPos.y + rightMidY), glm::vec2(rightW2, slabSize.y - rightMidY), 0.0f, rightShdCol * 0.94f);

	// Нижняя кромка верхней грани (3 сегмента)
	cursorX = slabPos.x;
	for (int i = 0; i < 3; ++i) {
		float segW;
		if (i < 2) {
			segW = slabSize.x * topFracs[2 - i];
		} else {
			segW = (slabPos.x + slabSize.x) - cursorX;
		}
		float segThick = std::max(1.5f, bevel * segFactor(static_cast<uint32_t>(i * 3 + 11), 0.75f, 0.45f));
		renderer->drawSprite(whiteTexture, glm::vec2(cursorX, slabPos.y + slabSize.y - segThick), glm::vec2(segW, segThick), 0.0f, botShdCol);
		cursorX += segW;
	}

	// Сколы на 2–3 углах для силуэта грубо обтёсанного камня
	int chipCount = 2 + static_cast<int>((seed >> 26) & 1u);
	uint32_t firstCorner = (seed >> 24) % 4u;
	for (int c = 0; c < chipCount; ++c) {
		uint32_t cornerIdx = (firstCorner + static_cast<uint32_t>(c) * 2u - static_cast<uint32_t>(c > 1 ? 1 : 0)) % 4u;
		float chipW = std::max(2.0f, m_cellSize * segFactor(static_cast<uint32_t>(5 + c * 5), 0.052f, 0.042f));
		float chipH = std::max(2.0f, m_cellSize * segFactor(static_cast<uint32_t>(8 + c * 5), 0.050f, 0.040f));
		glm::vec2 chipPos;
		glm::vec2 facetPos;
		glm::vec3 facetCol;
		if (cornerIdx == 0u) {
			chipPos = slabPos;
			facetPos = slabPos + glm::vec2(chipW * 0.65f, chipH * 0.65f);
			facetCol = topLitCol * 0.92f;
		} else if (cornerIdx == 1u) {
			chipPos = glm::vec2(slabPos.x + slabSize.x - chipW, slabPos.y);
			facetPos = glm::vec2(slabPos.x + slabSize.x - chipW * 1.45f, slabPos.y + chipH * 0.55f);
			facetCol = rightShdCol * 1.08f;
		} else if (cornerIdx == 2u) {
			chipPos = glm::vec2(slabPos.x, slabPos.y + slabSize.y - chipH);
			facetPos = glm::vec2(slabPos.x + chipW * 0.55f, slabPos.y + slabSize.y - chipH * 1.40f);
			facetCol = leftLitCol * 0.88f;
		} else {
			chipPos = glm::vec2(slabPos.x + slabSize.x - chipW, slabPos.y + slabSize.y - chipH);
			facetPos = glm::vec2(slabPos.x + slabSize.x - chipW * 1.45f, slabPos.y + slabSize.y - chipH * 1.40f);
			facetCol = botShdCol * 0.92f;
		}
		renderer->drawSprite(whiteTexture, chipPos, glm::vec2(chipW, chipH), 0.0f, creviceCol);
		renderer->drawSpriteRGBA(whiteTexture, facetPos, glm::vec2(chipW * 0.8f, chipH * 0.65f), 25.0f, glm::vec4(facetCol, 0.55f));
	}

	// 5. Едва заметная притоптанная/выровненная центральная площадка под башню
	float padOffX = (static_cast<float>((seed >> 15) & 0xFu) / 15.0f - 0.5f) * 0.05f * m_cellSize;
	float padOffY = (static_cast<float>((seed >> 19) & 0xFu) / 15.0f - 0.5f) * 0.05f * m_cellSize;
	glm::vec2 padCenter = slabPos + slabSize * 0.5f + glm::vec2(padOffX, padOffY);
	glm::vec2 outerPadSz = slabSize * 0.54f;
	glm::vec2 innerPadSz = slabSize * 0.44f;
	renderer->drawSpriteRGBA(
		whiteTexture,
		padCenter - outerPadSz * 0.5f,
		outerPadSz,
		0.0f,
		glm::vec4(0.24f * color.r, 0.20f * color.g, 0.17f * color.b, 0.18f)
	);
	renderer->drawSpriteRGBA(
		whiteTexture,
		padCenter - innerPadSz * 0.5f,
		innerPadSz,
		0.0f,
		glm::vec4(0.375f * color.r, 0.320f * color.g, 0.268f * color.b, 0.34f)
	);

	// 6. Детали неухоженной выработки по углам и краям (следы кирки, щебень, тусклая пыль киновари)
	// Короткие царапины и следы от кирки
	int markCount = 2 + static_cast<int>((seed >> 27) & 1u);
	float markThick = std::max(1.0f, m_cellSize * 0.020f);
	for (int m = 0; m < markCount; ++m) {
		float angleDeg = -55.0f + static_cast<float>((seed >> (4 + m * 6)) % 111u);
		float markLen = m_cellSize * (0.12f + static_cast<float>((seed >> (7 + m * 5)) % 11u) * 0.01f);
		float mxFrac;
		float myFrac;
		if (m == 0) {
			mxFrac = 0.24f + static_cast<float>((seed >> 10) % 16u) * 0.01f;
			myFrac = 0.22f + static_cast<float>((seed >> 14) % 16u) * 0.01f;
		} else if (m == 1) {
			mxFrac = 0.62f + static_cast<float>((seed >> 18) % 16u) * 0.01f;
			myFrac = 0.64f + static_cast<float>((seed >> 22) % 14u) * 0.01f;
		} else {
			mxFrac = 0.66f + static_cast<float>((seed >> 12) % 14u) * 0.01f;
			myFrac = 0.22f + static_cast<float>((seed >> 16) % 14u) * 0.01f;
		}
		glm::vec2 markCenter = slabPos + glm::vec2(slabSize.x * mxFrac, slabSize.y * myFrac);
		glm::vec2 markSize(markLen, markThick);
		renderer->drawSpriteRGBA(
			whiteTexture,
			markCenter - markSize * 0.5f,
			markSize,
			angleDeg,
			glm::vec4(0.115f * color.r, 0.095f * color.g, 0.088f * color.b, 0.62f)
		);
		if (m == 0) {
			renderer->drawSpriteRGBA(
				whiteTexture,
				markCenter - markSize * 0.5f + glm::vec2(0.0f, markThick),
				glm::vec2(markLen * 0.85f, std::max(1.0f, markThick * 0.75f)),
				angleDeg,
				glm::vec4(0.45f * color.r, 0.39f * color.g, 0.33f * color.b, 0.28f)
			);
		}
	}

	// Карманы углов для щебня и пыли выкопанной киновари
	auto getCornerPocket = [&](uint32_t cIdx, float insetFrac) -> glm::vec2 {
		if (cIdx == 0u) {
			return slabPos + glm::vec2(slabSize.x * insetFrac, slabSize.y * insetFrac);
		}
		if (cIdx == 1u) {
			return slabPos + glm::vec2(slabSize.x * (1.0f - insetFrac), slabSize.y * insetFrac);
		}
		if (cIdx == 2u) {
			return slabPos + glm::vec2(slabSize.x * insetFrac, slabSize.y * (1.0f - insetFrac));
		}
		return slabPos + glm::vec2(slabSize.x * (1.0f - insetFrac), slabSize.y * (1.0f - insetFrac));
	};

	// Мелкий щебень и каменная крошка по углам и краям (5 камешков)
	uint32_t pebbleCornerBase = (seed >> 2) % 4u;
	for (int p = 0; p < 5; ++p) {
		uint32_t cIdx = (pebbleCornerBase + static_cast<uint32_t>(p % 3)) % 4u;
		float inset = 0.14f + static_cast<float>((seed >> (p * 4 + 3)) % 10u) * 0.01f;
		glm::vec2 baseP = getCornerPocket(cIdx, inset);
		float jx = (static_cast<float>((seed >> (p * 5 + 1)) % 15u) - 7.0f) * 0.01f * slabSize.x;
		float jy = (static_cast<float>((seed >> (p * 5 + 7)) % 15u) - 7.0f) * 0.01f * slabSize.y;
		glm::vec2 pCenter(
			std::clamp(baseP.x + jx, slabPos.x + bevel, slabPos.x + slabSize.x - bevel),
			std::clamp(baseP.y + jy, slabPos.y + bevel, slabPos.y + slabSize.y - bevel)
		);
		float pSz = std::max(1.5f, m_cellSize * (0.032f + static_cast<float>((seed >> (p * 3 + 9)) % 30u) * 0.001f));
		if (p < 2) {
			renderer->drawSpriteRGBA(
				whiteTexture,
				pCenter - glm::vec2(pSz * 0.5f) + glm::vec2(0.5f, 1.0f),
				glm::vec2(pSz, pSz),
				0.0f,
				glm::vec4(0.05f * color.r, 0.04f * color.g, 0.04f * color.b, 0.35f)
			);
		}
		glm::vec3 pebCol;
		if ((p % 2) == 0) {
			pebCol = glm::vec3(0.145f, 0.130f, 0.130f) * color;
		} else {
			pebCol = glm::vec3(0.425f, 0.365f, 0.305f) * color;
		}
		renderer->drawSprite(whiteTexture, pCenter - glm::vec2(pSz * 0.5f), glm::vec2(pSz, pSz), static_cast<float>((p * 27) % 60), pebCol);
	}

	// Еле заметные тусклые крошки и пыль выкопанной киновари в паре углов (строго без свечения)
	uint32_t cinCorner1 = (seed >> 6) % 4u;
	uint32_t cinCorner2 = (cinCorner1 + 1u + ((seed >> 8) % 2u)) % 4u;
	uint32_t cinCorners[2] = { cinCorner1, cinCorner2 };
	for (int k = 0; k < 2; ++k) {
		glm::vec2 cPos = getCornerPocket(cinCorners[k], 0.17f);
		float dustSz = std::max(3.0f, m_cellSize * 0.13f);
		renderer->drawSpriteRGBA(
			whiteTexture,
			cPos - glm::vec2(dustSz * 0.5f),
			glm::vec2(dustSz, dustSz * 0.85f),
			static_cast<float>((seed >> (k * 6)) % 45u),
			glm::vec4(0.42f * color.r, 0.15f * color.g, 0.13f * color.b, 0.15f)
		);

		float crumbSz = std::max(1.5f, std::round(m_cellSize * 0.032f));
		glm::vec2 crumb1 = cPos + glm::vec2(-crumbSz * 0.6f, crumbSz * 0.2f);
		renderer->drawSpriteRGBA(
			whiteTexture,
			crumb1,
			glm::vec2(crumbSz, crumbSz),
			0.0f,
			glm::vec4(0.46f * color.r, 0.14f * color.g, 0.14f * color.b, 0.68f)
		);
		if (k == 0) {
			glm::vec2 crumb2 = cPos + glm::vec2(crumbSz * 1.1f, -crumbSz * 0.7f);
			float smallSz = std::max(1.2f, crumbSz * 0.72f);
			renderer->drawSpriteRGBA(
				whiteTexture,
				crumb2,
				glm::vec2(smallSz, smallSz),
				0.0f,
				glm::vec4(0.42f * color.r, 0.13f * color.g, 0.13f * color.b, 0.50f)
			);
		}
	}
}

CellType Grid::getVisualFloorType(int gridX, int gridY) const
{
	CellType t = CellType::Scenery;
	if (gridX >= 0 && gridX < m_width && gridY >= 0 && gridY < m_height) {
		t = m_grid[gridY][gridX];
		if (t == CellType::Tower) {
			t = m_originalGrid[gridY][gridX];
		}
	} else if (m_hasSidePockets) {
		auto it = m_outerCells.find({gridX, gridY});
		if (it == m_outerCells.end()) {
			return CellType::Empty;
		}
		t = it->second;
		if (t == CellType::Tower) {
			auto origIt = m_originalOuterCells.find({gridX, gridY});
			t = (origIt != m_originalOuterCells.end()) ? origIt->second : CellType::Ground;
		}
	} else {
		return CellType::Scenery;
	}

	if (t == CellType::Spawner || t == CellType::Base) {
		return CellType::Ground;
	}
	return t;
}

void Grid::renderCinnabarFloorCell(SpriteRenderer* renderer,
	const Texture2D* whiteTexture,
	int x, int y, CellType type,
	glm::vec2 pixelPos, glm::vec3 color,
	uint32_t seed, float deltaL, float warmShift) const
{
	const glm::vec3 fillColor = computeCinnabarFloorColor(type, deltaL, warmShift);
	const glm::vec3 borderColor = fillColor * ((type == CellType::Ground) ? 0.67f : 0.64f);
	const CellType oppType = (type == CellType::Ground) ? CellType::Path : CellType::Ground;

	const bool oppUp    = (getVisualFloorType(x, y - 1) == oppType);
	const bool oppDown  = (getVisualFloorType(x, y + 1) == oppType);
	const bool oppLeft  = (getVisualFloorType(x - 1, y) == oppType);
	const bool oppRight = (getVisualFloorType(x + 1, y) == oppType);

	// 1. Полная базовая заливка плиты
	renderer->drawSprite(whiteTexture, pixelPos, glm::vec2(m_cellSize, m_cellSize), 0.0f, fillColor * color);

	// 2. Контрастная рамка только на сторонах, НЕ граничащих с противоположным типом пола (Ground <-> Path)
	const float borderWidth = std::max(1.0f, std::round(m_cellSize * 0.04f));
	const glm::vec3 borderCol = borderColor * color;
	if (!oppUp) {
		renderer->drawSprite(whiteTexture, pixelPos, glm::vec2(m_cellSize, borderWidth), 0.0f, borderCol);
	}
	if (!oppDown) {
		renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x, pixelPos.y + m_cellSize - borderWidth), glm::vec2(m_cellSize, borderWidth), 0.0f, borderCol);
	}
	if (!oppLeft) {
		renderer->drawSprite(whiteTexture, pixelPos, glm::vec2(borderWidth, m_cellSize), 0.0f, borderCol);
	}
	if (!oppRight) {
		renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + m_cellSize - borderWidth, pixelPos.y), glm::vec2(borderWidth, m_cellSize), 0.0f, borderCol);
	}

	// 3. Едва заметная фаска каменной плиты только вдоль непереходных граней
	const float edgeH = std::max(1.0f, std::round(m_cellSize * 0.025f));
	const float leftInset = oppLeft ? 0.0f : borderWidth;
	const float rightInset = oppRight ? 0.0f : borderWidth;
	const float bevelW = std::max(0.0f, m_cellSize - leftInset - rightInset);
	if (bevelW > 0.0f) {
		if (!oppUp) {
			renderer->drawSprite(
				whiteTexture,
				glm::vec2(pixelPos.x + leftInset, pixelPos.y + borderWidth),
				glm::vec2(bevelW, edgeH),
				0.0f,
				fillColor * 1.08f * color
			);
		}
		if (!oppDown) {
			renderer->drawSprite(
				whiteTexture,
				glm::vec2(pixelPos.x + leftInset, pixelPos.y + m_cellSize - borderWidth - edgeH),
				glm::vec2(bevelW, edgeH),
				0.0f,
				fillColor * 0.88f * color
			);
		}
	}

	// 4. Процедурные швы, трещины и крошка сланца в центре плиты
	const float edgeMargin = m_cellSize * 0.14f;
	const float seamLeft   = oppLeft  ? edgeMargin : borderWidth;
	const float seamRight  = oppRight ? edgeMargin : borderWidth;
	const float seamTop    = oppUp    ? edgeMargin : borderWidth;
	const float seamBottom = oppDown  ? edgeMargin : borderWidth;
	const float seamSpanX  = std::max(0.0f, m_cellSize - seamLeft - seamRight);
	const float seamSpanY  = std::max(0.0f, m_cellSize - seamTop - seamBottom);

	const glm::vec2 innerOffset(borderWidth, borderWidth);
	const glm::vec2 innerSize(std::max(0.0f, m_cellSize - 2.0f * borderWidth), std::max(0.0f, m_cellSize - 2.0f * borderWidth));

	const uint32_t patternKind = (seed >> 16) % 11u;
	const float seamThick = std::max(1.0f, m_cellSize * 0.020f);
	if (patternKind <= 1u) {
		const float frac = 0.34f + static_cast<float>((seed >> 21) % 32u) * 0.01f;
		if (((seed >> 20) & 1u) == 0u) {
			if (seamSpanX > 0.0f) {
				float sy = pixelPos.y + innerOffset.y + innerSize.y * frac;
				renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + seamLeft, sy), glm::vec2(seamSpanX, seamThick), 0.0f, fillColor * 0.78f * color);
				renderer->drawSprite(whiteTexture, glm::vec2(pixelPos.x + seamLeft, sy + seamThick), glm::vec2(seamSpanX, seamThick), 0.0f, fillColor * 1.07f * color);
			}
		} else {
			if (seamSpanY > 0.0f) {
				float sx = pixelPos.x + innerOffset.x + innerSize.x * frac;
				renderer->drawSprite(whiteTexture, glm::vec2(sx, pixelPos.y + seamTop), glm::vec2(seamThick, seamSpanY), 0.0f, fillColor * 0.78f * color);
				renderer->drawSprite(whiteTexture, glm::vec2(sx + seamThick, pixelPos.y + seamTop), glm::vec2(seamThick, seamSpanY), 0.0f, fillColor * 1.07f * color);
			}
		}
	} else if (patternKind <= 4u) {
		float cx = pixelPos.x + innerOffset.x + innerSize.x * (0.28f + static_cast<float>((seed >> 21) % 36u) * 0.01f);
		float cy = pixelPos.y + innerOffset.y + innerSize.y * (0.30f + static_cast<float>((seed >> 26) % 34u) * 0.01f);
		float len1 = m_cellSize * 0.26f;
		float len2 = m_cellSize * 0.16f;
		float thick = std::max(1.0f, m_cellSize * 0.022f);
		float ang1 = (((seed >> 19) & 1u) == 0u) ? 24.0f : -28.0f;
		float ang2 = ang1 + (((seed >> 20) & 1u) == 0u ? -34.0f : 32.0f);
		renderer->drawSprite(whiteTexture, glm::vec2(cx - len1 * 0.45f, cy), glm::vec2(len1, thick), ang1, fillColor * 0.74f * color);
		renderer->drawSprite(whiteTexture, glm::vec2(cx + len1 * 0.15f, cy + thick), glm::vec2(len2, thick), ang2, fillColor * 0.76f * color);
	}

	float p1x = static_cast<float>((seed % 52u) + 14u) * 0.01f;
	float p1y = static_cast<float>(((seed / 53u) % 52u) + 14u) * 0.01f;
	float p2x = static_cast<float>(((seed / 2809u) % 48u) + 22u) * 0.01f;
	float p2y = static_cast<float>(((seed / 134832u) % 48u) + 22u) * 0.01f;
	float pSz = std::max(1.5f, std::round(m_cellSize * 0.042f));
	renderer->drawSprite(whiteTexture, pixelPos + glm::vec2(p1x * m_cellSize, p1y * m_cellSize), glm::vec2(pSz, pSz), 0.0f, fillColor * 0.76f * color);
	if ((seed % 3u) == 0u) {
		renderer->drawSprite(whiteTexture, pixelPos + glm::vec2(p2x * m_cellSize, p2y * m_cellSize), glm::vec2(pSz * 0.85f, pSz * 0.85f), 0.0f, fillColor * 1.13f * color);
	}
}

void Grid::renderCinnabarFloorTransitions(SpriteRenderer* renderer,
	const Texture2D* whiteTexture,
	glm::vec3 color) const
{
	auto forEachFloorCell = [&](auto&& fn) {
		for (int y = 0; y < m_height; ++y) {
			for (int x = 0; x < m_width; ++x) {
				CellType type = getVisualFloorType(x, y);
				if (type == CellType::Ground || type == CellType::Path) {
					fn(x, y, type, gridToPixel(x, y));
				}
			}
		}
		for (const auto& [pos, rawType] : m_outerCells) {
			(void)rawType;
			CellType type = getVisualFloorType(pos.first, pos.second);
			if (type == CellType::Ground || type == CellType::Path) {
				fn(pos.first, pos.second, type, gridToPixel(pos.first, pos.second));
			}
		}
	};

	auto neighborColor = [&](int nx, int ny, CellType nt) -> glm::vec3 {
		const CinnabarCellParams p = computeCinnabarCellParams(nx, ny);
		return computeCinnabarFloorColor(nt, p.deltaL, p.warmShift) * color;
	};

	const float gradW = std::max(4.0f, m_cellSize * 0.23f);

	// Sub-pass 1: Мягкие линейные градиенты вдоль всех стыков Path <-> Ground
	const Texture2D* gradY = ResourceManager::getLinearGradYTexture();
	if (gradY != nullptr) {
		const SpriteUV flippedY{ glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 0.0f) };
		constexpr float kSeamAlpha = 0.50f;

		forEachFloorCell([&](int x, int y, CellType type, glm::vec2 pixelPos) {
			const CellType oppType = (type == CellType::Ground) ? CellType::Path : CellType::Ground;

			if (getVisualFloorType(x, y - 1) == oppType) {
				renderer->drawSpriteRGBA(
					gradY,
					pixelPos,
					glm::vec2(m_cellSize, gradW),
					0.0f,
					glm::vec4(neighborColor(x, y - 1, oppType), kSeamAlpha)
				);
			}
			if (getVisualFloorType(x, y + 1) == oppType) {
				renderer->drawSpriteRGBA(
					gradY,
					glm::vec2(pixelPos.x, pixelPos.y + m_cellSize - gradW),
					glm::vec2(m_cellSize, gradW),
					0.0f,
					glm::vec4(neighborColor(x, y + 1, oppType), kSeamAlpha),
					flippedY
				);
			}
			if (getVisualFloorType(x - 1, y) == oppType) {
				const glm::vec2 size(m_cellSize, gradW);
				const glm::vec2 center(pixelPos.x + gradW * 0.5f, pixelPos.y + m_cellSize * 0.5f);
				renderer->drawSpriteRGBA(
					gradY,
					center - size * 0.5f,
					size,
					-90.0f,
					glm::vec4(neighborColor(x - 1, y, oppType), kSeamAlpha)
				);
			}
			if (getVisualFloorType(x + 1, y) == oppType) {
				const glm::vec2 size(m_cellSize, gradW);
				const glm::vec2 center(pixelPos.x + m_cellSize - gradW * 0.5f, pixelPos.y + m_cellSize * 0.5f);
				renderer->drawSpriteRGBA(
					gradY,
					center - size * 0.5f,
					size,
					90.0f,
					glm::vec4(neighborColor(x + 1, y, oppType), kSeamAlpha)
				);
			}
		});
	}

	// Sub-pass 2: Диагональные скругления внешних L-углов и пыльные пятна вдоль линии стыка
	const Texture2D* softRadial = ResourceManager::getSoftRadialTexture();
	if (softRadial != nullptr) {
		const float cornerDia = gradW * 1.55f;
		const float cornerOffset = gradW * 0.12f;

		forEachFloorCell([&](int x, int y, CellType type, glm::vec2 pixelPos) {
			const CellType oppType = (type == CellType::Ground) ? CellType::Path : CellType::Ground;
			const bool oppUp    = (getVisualFloorType(x, y - 1) == oppType);
			const bool oppDown  = (getVisualFloorType(x, y + 1) == oppType);
			const bool oppLeft  = (getVisualFloorType(x - 1, y) == oppType);
			const bool oppRight = (getVisualFloorType(x + 1, y) == oppType);

			auto drawCorner = [&](int dx, int dy, glm::vec2 center) {
				if (getVisualFloorType(dx, dy) == oppType) {
					renderer->drawSpriteRGBA(
						softRadial,
						center - glm::vec2(cornerDia * 0.5f),
						glm::vec2(cornerDia, cornerDia),
						0.0f,
						glm::vec4(neighborColor(dx, dy, oppType), 0.24f)
					);
				}
			};

			if (!oppUp && !oppLeft) {
				drawCorner(x - 1, y - 1, pixelPos + glm::vec2(cornerOffset, cornerOffset));
			}
			if (!oppUp && !oppRight) {
				drawCorner(x + 1, y - 1, glm::vec2(pixelPos.x + m_cellSize - cornerOffset, pixelPos.y + cornerOffset));
			}
			if (!oppDown && !oppLeft) {
				drawCorner(x - 1, y + 1, glm::vec2(pixelPos.x + cornerOffset, pixelPos.y + m_cellSize - cornerOffset));
			}
			if (!oppDown && !oppRight) {
				drawCorner(x + 1, y + 1, glm::vec2(pixelPos.x + m_cellSize - cornerOffset, pixelPos.y + m_cellSize - cornerOffset));
			}

			// Пыльные пятна вдоль стыка рисуем по одному разу со стороны Path
			if (type != CellType::Path) return;

			const glm::vec3 pathCol = neighborColor(x, y, CellType::Path);
			const struct { int dx; int dy; } dirs[4] = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };
			for (uint32_t dir = 0; dir < 4; ++dir) {
				const int nx = x + dirs[dir].dx;
				const int ny = y + dirs[dir].dy;
				if (getVisualFloorType(nx, ny) != CellType::Ground) continue;

				const glm::vec3 groundCol = neighborColor(nx, ny, CellType::Ground);
				glm::vec2 edgeOrigin = pixelPos;
				glm::vec2 tangent(1.0f, 0.0f);
				glm::vec2 normal(0.0f, -1.0f);
				float baseAngle = 0.0f;

				if (dir == 0) {
					edgeOrigin = pixelPos;
					tangent = glm::vec2(1.0f, 0.0f);
					normal = glm::vec2(0.0f, -1.0f);
					baseAngle = 0.0f;
				} else if (dir == 1) {
					edgeOrigin = glm::vec2(pixelPos.x, pixelPos.y + m_cellSize);
					tangent = glm::vec2(1.0f, 0.0f);
					normal = glm::vec2(0.0f, 1.0f);
					baseAngle = 0.0f;
				} else if (dir == 2) {
					edgeOrigin = pixelPos;
					tangent = glm::vec2(0.0f, 1.0f);
					normal = glm::vec2(-1.0f, 0.0f);
					baseAngle = 90.0f;
				} else {
					edgeOrigin = glm::vec2(pixelPos.x + m_cellSize, pixelPos.y);
					tangent = glm::vec2(0.0f, 1.0f);
					normal = glm::vec2(1.0f, 0.0f);
					baseAngle = 90.0f;
				}

				uint32_t h = hashCinnabarEdge(x, y, dir);
				const int dustCount = 2 + static_cast<int>(h & 1u);
				for (int i = 0; i < dustCount; ++i) {
					h = mixHash(h + static_cast<uint32_t>(i) * 0x9e3779b9u);
					const float tBase = (static_cast<float>(i) + 0.5f) / static_cast<float>(dustCount);
					const float t = std::clamp(tBase + (hash01(h) - 0.5f) * 0.22f, 0.16f, 0.84f);
					const uint32_t h2 = mixHash(h ^ 0x517cc1b7u);
					const float nOff = (hash01(h2) - 0.5f) * 0.16f * m_cellSize;
					const float len = m_cellSize * (0.26f + hash01(h >> 8) * 0.18f);
					const float thick = m_cellSize * (0.14f + hash01(h2 >> 8) * 0.08f);
					const float ang = baseAngle + (hash01(h >> 16) - 0.5f) * 24.0f;
					const float alpha = 0.22f + hash01(h2 >> 16) * 0.08f;

					const glm::vec3 dustCol = ((i & 1) == 0)
						? (glm::mix(pathCol, groundCol, 0.28f) * 1.04f)
						: (glm::mix(pathCol, groundCol, 0.72f) * 0.96f);

					const glm::vec2 center = edgeOrigin + tangent * (t * m_cellSize) + normal * nOff;
					const glm::vec2 sz(len, thick);
					renderer->drawSpriteRGBA(softRadial, center - sz * 0.5f, sz, ang, glm::vec4(dustCol, alpha));
				}
			}
		});
	}

	// Sub-pass 3: Мелкий щебень и небольшие каменные осколки вдоль стыков (разбивают прямую сетку)
	forEachFloorCell([&](int x, int y, CellType type, glm::vec2 pixelPos) {
		if (type != CellType::Path) return;

		const glm::vec3 pathCol = neighborColor(x, y, CellType::Path);
		const struct { int dx; int dy; } dirs[4] = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };

		for (uint32_t dir = 0; dir < 4; ++dir) {
			const int nx = x + dirs[dir].dx;
			const int ny = y + dirs[dir].dy;
			if (getVisualFloorType(nx, ny) != CellType::Ground) continue;

			const glm::vec3 groundCol = neighborColor(nx, ny, CellType::Ground);
			const glm::vec3 midCol = 0.5f * (pathCol + groundCol);

			glm::vec2 edgeOrigin = pixelPos;
			glm::vec2 tangent(1.0f, 0.0f);
			glm::vec2 normal(0.0f, -1.0f);
			float baseAngle = 0.0f;

			if (dir == 0) {
				edgeOrigin = pixelPos;
				tangent = glm::vec2(1.0f, 0.0f);
				normal = glm::vec2(0.0f, -1.0f);
				baseAngle = 0.0f;
			} else if (dir == 1) {
				edgeOrigin = glm::vec2(pixelPos.x, pixelPos.y + m_cellSize);
				tangent = glm::vec2(1.0f, 0.0f);
				normal = glm::vec2(0.0f, 1.0f);
				baseAngle = 0.0f;
			} else if (dir == 2) {
				edgeOrigin = pixelPos;
				tangent = glm::vec2(0.0f, 1.0f);
				normal = glm::vec2(-1.0f, 0.0f);
				baseAngle = 90.0f;
			} else {
				edgeOrigin = glm::vec2(pixelPos.x + m_cellSize, pixelPos.y);
				tangent = glm::vec2(0.0f, 1.0f);
				normal = glm::vec2(1.0f, 0.0f);
				baseAngle = 90.0f;
			}

			uint32_t h = hashCinnabarEdge(x, y, dir);

			// 2–3 каменных осколка / неровности прямо на линии стыка
			const int chipCount = 2 + static_cast<int>((h >> 4) & 1u);
			for (int i = 0; i < chipCount; ++i) {
				h = mixHash(h + static_cast<uint32_t>(i + 1) * 0x85ebca6bu);
				const uint32_t h2 = mixHash(h ^ 0xc2b2ae35u);
				const float tBase = (static_cast<float>(i) + 0.5f) / static_cast<float>(chipCount);
				const float t = std::clamp(tBase + (hash01(h) - 0.5f) * 0.24f, 0.12f, 0.88f);
				const float nOff = (hash01(h2) - 0.5f) * 0.09f * m_cellSize;
				const float chipLen = std::max(2.5f, m_cellSize * (0.07f + hash01(h >> 8) * 0.06f));
				const float chipThick = std::max(1.8f, m_cellSize * (0.038f + hash01(h2 >> 8) * 0.030f));
				const float ang = baseAngle + (hash01(h >> 16) - 0.5f) * 40.0f;
				const float mixFactor = 0.25f + hash01(h2 >> 16) * 0.50f;
				const float alpha = 0.48f + hash01(h >> 12) * 0.14f;

				const glm::vec2 center = edgeOrigin + tangent * (t * m_cellSize) + normal * nOff;
				const glm::vec2 sz(chipLen, chipThick);
				const glm::vec2 topLeft = center - sz * 0.5f;

				renderer->drawSpriteRGBA(whiteTexture, topLeft + glm::vec2(1.0f, 1.0f), sz, ang, glm::vec4(groundCol * 0.50f, 0.22f));
				renderer->drawSpriteRGBA(whiteTexture, topLeft, sz, ang, glm::vec4(glm::mix(groundCol, pathCol, mixFactor), alpha));
			}

			// 3–5 зёрен мелкого щебня в пределах переходной полосы (центр клетки остаётся чистым)
			const int gravelCount = 3 + static_cast<int>((h >> 7) % 3u);
			for (int i = 0; i < gravelCount; ++i) {
				h = mixHash(h + static_cast<uint32_t>(i + 11) * 0x27d4eb2du);
				const uint32_t h2 = mixHash(h ^ 0x165667b1u);
				const float t = 0.06f + hash01(h) * 0.88f;
				const float nOff = (hash01(h2) - 0.5f) * 0.28f * m_cellSize;
				const float baseSz = std::max(1.5f, m_cellSize * (0.024f + hash01(h >> 8) * 0.020f));
				const float aspect = 0.75f + hash01(h2 >> 8) * 0.50f;
				const float ang = hash01(h >> 16) * 180.0f;

				glm::vec4 pebbleCol;
				switch (i % 3) {
					case 0:  pebbleCol = glm::vec4(groundCol * 0.68f, 0.62f); break;
					case 1:  pebbleCol = glm::vec4(midCol * 0.86f, 0.55f);    break;
					default: pebbleCol = glm::vec4(pathCol * 1.12f, 0.52f);   break;
				}

				const glm::vec2 center = edgeOrigin + tangent * (t * m_cellSize) + normal * nOff;
				const glm::vec2 sz(baseSz, baseSz * aspect);
				renderer->drawSpriteRGBA(whiteTexture, center - sz * 0.5f, sz, ang, pebbleCol);
			}
		}
	});
}

void Grid::drawBaseCells(SpriteRenderer* renderer,
	const Texture2D* whiteTexture,
	glm::vec3 color,
	const std::string& backgroundPreset) const
{
	if (!renderer || !whiteTexture) return;

	const bool isMineTheme = (backgroundPreset == "mine_entrance" || backgroundPreset == "mine");
	const bool isCanalTheme = (backgroundPreset == "pipes_canal" || backgroundPreset == "channel");
	const bool isCinnabarTheme = (backgroundPreset == "cinnabar_mine" || backgroundPreset == "cinnabar");

	// 1. Отрисовка базовых клеток основной матрицы (без Scenery)
	for (int y = 0; y < m_height; ++y) {
		for (int x = 0; x < m_width; ++x) {
			CellType type = m_grid[y][x];
			if (type == CellType::Tower) {
				type = m_originalGrid[y][x];
			}
			if (type == CellType::Spawner || type == CellType::Base) {
				type = CellType::Ground;
			}
			if (type == CellType::Scenery || type == CellType::Empty) continue;
			renderCellVisual(renderer, whiteTexture, x, y, type, gridToPixel(x, y), color, isMineTheme, isCanalTheme, isCinnabarTheme);
		}
	}

	// 2. Отрисовка внешних базовых клеток (Грунт, Дорога, Платформа, Шурф, Рельсы) под фоновым окружением
	for (const auto& [pos, type] : m_outerCells) {
		CellType ct = type;
		if (ct == CellType::Spawner || ct == CellType::Base) {
			ct = CellType::Ground;
		}
		if (ct == CellType::Scenery || ct == CellType::Empty) continue;
		renderCellVisual(renderer, whiteTexture, pos.first, pos.second, ct, gridToPixel(pos.first, pos.second), color, isMineTheme, isCanalTheme, isCinnabarTheme);
	}

	if (isCinnabarTheme) {
		renderCinnabarFloorTransitions(renderer, whiteTexture, color);
	}
}

void Grid::drawEditorGridLines(SpriteRenderer* renderer,
	const Texture2D* whiteTexture) const
{
	if (!renderer || !whiteTexture) return;

	// При включенных свободных границах рисуем мягкую полупрозрачную сетку вокруг матрицы поверх фона
	if (m_hasSidePockets) {
		float gBorder = std::max(1.0f, std::round(m_cellSize * 0.02f));
		glm::vec4 gBorderCol(1.0f, 1.0f, 1.0f, 0.22f);
		int padX = 14;
		int padY = 8;
		for (int y = -padY; y < m_height + padY; ++y) {
			for (int x = -padX; x < m_width + padX; ++x) {
				if (x >= 0 && x < m_width && y >= 0 && y < m_height) continue;
				glm::vec2 pixelPos = gridToPixel(x, y);
				renderer->drawSpriteRGBA(whiteTexture, pixelPos, glm::vec2(m_cellSize, gBorder), 0.0f, gBorderCol);
				renderer->drawSpriteRGBA(whiteTexture, pixelPos + glm::vec2(0.0f, m_cellSize - gBorder), glm::vec2(m_cellSize, gBorder), 0.0f, gBorderCol);
				renderer->drawSpriteRGBA(whiteTexture, pixelPos, glm::vec2(gBorder, m_cellSize), 0.0f, gBorderCol);
				renderer->drawSpriteRGBA(whiteTexture, pixelPos + glm::vec2(m_cellSize - gBorder, 0.0f), glm::vec2(gBorder, m_cellSize), 0.0f, gBorderCol);
			}
		}
	}
}

void Grid::drawObstacleCells(SpriteRenderer* renderer,
	const Texture2D* whiteTexture,
	glm::vec3 color,
	const std::string& backgroundPreset,
	bool includeAnimatedGlow) const
{
	if (!renderer || !whiteTexture) return;

	const bool isMineTheme = (backgroundPreset == "mine_entrance" || backgroundPreset == "mine");
	const bool isCanalTheme = (backgroundPreset == "pipes_canal" || backgroundPreset == "channel");
	const bool isCinnabarTheme = (backgroundPreset == "cinnabar_mine" || backgroundPreset == "cinnabar");

	// 1. Отрисовка препятствий Scenery ("Ящики" / "Стены" / "Киноварь") в матрице
	for (int y = 0; y < m_height; ++y) {
		for (int x = 0; x < m_width; ++x) {
			CellType type = m_grid[y][x];
			if (type == CellType::Tower) {
				type = m_originalGrid[y][x];
			}
			if (type == CellType::Scenery) {
				renderCellVisual(renderer, whiteTexture, x, y, type, gridToPixel(x, y), color, isMineTheme, isCanalTheme, isCinnabarTheme, includeAnimatedGlow);
			}
		}
	}

	// 2. Отрисовка препятствий Scenery во внешних клетках
	for (const auto& [pos, type] : m_outerCells) {
		if (type == CellType::Scenery) {
			renderCellVisual(renderer, whiteTexture, pos.first, pos.second, type, gridToPixel(pos.first, pos.second), color, isMineTheme, isCanalTheme, isCinnabarTheme, includeAnimatedGlow);
		}
	}
}

void Grid::drawCinnabarObstacleAnimated(SpriteRenderer* renderer,
	const Texture2D* whiteTexture,
	glm::vec3 color,
	glm::vec2 cullMin,
	glm::vec2 cullMax,
	bool useCulling) const
{
	if (!renderer || !whiteTexture) return;

	const float time = static_cast<float>(glfwGetTime());
	const Texture2D* radialTex = ResourceManager::getSoftRadialTexture();
	const Texture2D* glowTex = (radialTex != nullptr) ? radialTex : whiteTexture;

	const float bevel = std::max(3.0f, std::round(m_cellSize * 0.11f));
	const float botBevel = std::max(4.0f, std::round(m_cellSize * 0.13f));

	const glm::vec2 minBound = glm::min(cullMin, cullMax);
	const glm::vec2 maxBound = glm::max(cullMin, cullMax);

	int minX = 0;
	int maxX = m_width - 1;
	int minY = 0;
	int maxY = m_height - 1;
	if (useCulling && m_cellSize > 0.001f) {
		minX = std::max(0, static_cast<int>(std::floor((minBound.x - m_offset.x) / m_cellSize)) - 1);
		maxX = std::min(m_width - 1, static_cast<int>(std::floor((maxBound.x - m_offset.x) / m_cellSize)) + 1);
		minY = std::max(0, static_cast<int>(std::floor((minBound.y - m_offset.y) / m_cellSize)) - 1);
		maxY = std::min(m_height - 1, static_cast<int>(std::floor((maxBound.y - m_offset.y) / m_cellSize)) + 1);
	}

	auto emitWallGlow = [&](int x, int y) {
		const CinnabarCellParams params = computeCinnabarCellParams(x, y);
		const bool wallUp    = (getCellType(x, y - 1) == CellType::Scenery);
		const bool wallDown  = (getCellType(x, y + 1) == CellType::Scenery);
		const bool wallLeft  = (getCellType(x - 1, y) == CellType::Scenery);
		const bool wallRight = (getCellType(x + 1, y) == CellType::Scenery);

		const CinnabarWallCrystalGeom crystal = computeCinnabarWallCrystalGeom(
			gridToPixel(x, y), m_cellSize, params.hash, wallUp, wallDown, wallLeft, wallRight, bevel, botBevel);
		if (!crystal.hasCrystal) {
			return;
		}

		const CinnabarPulse pulse = computeCinnabarPulse(params.hash, time);
		const glm::vec2 outerSz(crystal.cScale * 0.85f * pulse.radiusMult);
		const glm::vec2 innerSz(crystal.cScale * 0.48f * pulse.radiusMult);

		renderer->drawSpriteRGBA(
			glowTex,
			crystal.center - outerSz * 0.5f,
			outerSz,
			0.0f,
			glm::vec4(0.70f * color.r, 0.10f * color.g, 0.12f * color.b, 0.16f * pulse.alphaMult)
		);
		renderer->drawSpriteRGBA(
			glowTex,
			crystal.center - innerSz * 0.5f,
			innerSz,
			0.0f,
			glm::vec4(0.92f * color.r, 0.18f * color.g, 0.18f * color.b, 0.22f * pulse.alphaMult)
		);
	};

	renderer->setAdditiveBlending(true);

	for (int y = minY; y <= maxY; ++y) {
		for (int x = minX; x <= maxX; ++x) {
			CellType type = m_grid[y][x];
			if (type == CellType::Tower) {
				type = m_originalGrid[y][x];
			}
			if (type == CellType::Scenery) {
				emitWallGlow(x, y);
			}
		}
	}

	for (const auto& [pos, type] : m_outerCells) {
		if (type != CellType::Scenery) continue;
		if (useCulling) {
			glm::vec2 pPos = gridToPixel(pos.first, pos.second);
			if (pPos.x + m_cellSize < minBound.x - m_cellSize || pPos.x > maxBound.x + m_cellSize ||
				pPos.y + m_cellSize < minBound.y - m_cellSize || pPos.y > maxBound.y + m_cellSize) {
				continue;
			}
		}
		emitWallGlow(pos.first, pos.second);
	}

	renderer->setAdditiveBlending(false);
}

void Grid::drawOuterNoBuildOverlay(SpriteRenderer* renderer,
	const Texture2D* whiteTexture) const
{
	if (!renderer || !whiteTexture) return;

	for (const auto& pos : m_outerNoBuild) {
		glm::vec2 pPos = gridToPixel(pos.first, pos.second);
		float borderW = std::max(2.0f, std::round(m_cellSize * 0.04f));
		glm::vec4 bgCol(1.0f, 0.20f, 0.20f, 0.18f);
		glm::vec4 borderCol(0.95f, 0.30f, 0.30f, 0.75f);
		glm::vec4 crossCol(0.95f, 0.25f, 0.25f, 0.85f);
		float barLen = m_cellSize * 0.70f;
		float barThick = std::max(2.0f, std::round(m_cellSize * 0.05f));

		renderer->drawSpriteRGBA(whiteTexture, pPos + glm::vec2(1.0f), glm::vec2(m_cellSize - 2.0f), 0.0f, bgCol);
		renderer->drawSpriteRGBA(whiteTexture, pPos, glm::vec2(m_cellSize, borderW), 0.0f, borderCol);
		renderer->drawSpriteRGBA(whiteTexture, pPos + glm::vec2(0.0f, m_cellSize - borderW), glm::vec2(m_cellSize, borderW), 0.0f, borderCol);
		renderer->drawSpriteRGBA(whiteTexture, pPos, glm::vec2(borderW, m_cellSize), 0.0f, borderCol);
		renderer->drawSpriteRGBA(whiteTexture, pPos + glm::vec2(m_cellSize - borderW, 0.0f), glm::vec2(borderW, m_cellSize), 0.0f, borderCol);

		glm::vec2 center = pPos + glm::vec2(m_cellSize * 0.5f);
		renderer->drawSpriteRGBA(whiteTexture, center - glm::vec2(barLen * 0.5f, barThick * 0.5f), glm::vec2(barLen, barThick), 45.0f, crossCol);
		renderer->drawSpriteRGBA(whiteTexture, center - glm::vec2(barLen * 0.5f, barThick * 0.5f), glm::vec2(barLen, barThick), -45.0f, crossCol);
	}
}

void Grid::draw(SpriteRenderer* renderer,
	const Texture2D* whiteTexture,
	glm::vec3 color,
	const std::string& backgroundPreset)
{
	drawBaseCells(renderer, whiteTexture, color, backgroundPreset);
	drawObstacleCells(renderer, whiteTexture, color, backgroundPreset);
	drawEditorGridLines(renderer, whiteTexture);
	drawOuterNoBuildOverlay(renderer, whiteTexture);
}

// Динамический пересчет размера клеток при изменении размера окна с учетом нижней панели и верхнего отступа
void Grid::updateCellSize(int windowWidth, int windowHeight, float bottomMargin, float topMargin) {
	const float oldCellSize = m_cellSize;
	const glm::vec2 oldOffset = m_offset;
	float usableHeight = std::max(1.0f, static_cast<float>(windowHeight) - bottomMargin - topMargin);
	float sizeX = static_cast<float>(windowWidth) / static_cast<float>(m_width);
	float sizeY = usableHeight / static_cast<float>(m_height);
	m_cellSize = std::min(sizeX, sizeY);
	float actualGridWidth = m_cellSize * m_width;
	float actualGridHeight = m_cellSize * m_height;
	m_offset.x = (static_cast<float>(windowWidth) - actualGridWidth) / 2.0f;
	m_offset.y = topMargin + (usableHeight - actualGridHeight) / 2.0f;
	if (std::abs(m_cellSize - oldCellSize) > 1e-4f || glm::length(m_offset - oldOffset) > 1e-4f) {
		m_visualVersion++;
	}
}

CellType Grid::getCellType(int gridX, int gridY) const {
	if (gridX >= 0 && gridX < m_width && gridY >= 0 && gridY < m_height) {
		return m_grid[gridY][gridX];
	}

	if (m_hasSidePockets) {
		auto it = m_outerCells.find({gridX, gridY});
		if (it != m_outerCells.end()) {
			return it->second;
		}
		return CellType::Empty;
	}

	return CellType::Scenery;
}

void Grid::setNoBuildMask(const std::vector<std::vector<bool>>& mask) {
	m_noBuildMask.assign(m_height, std::vector<bool>(m_width, false));
	for (int y = 0; y < (int)mask.size() && y < m_height; ++y) {
		for (int x = 0; x < (int)mask[y].size() && x < m_width; ++x) {
			m_noBuildMask[y][x] = mask[y][x];
		}
	}
}

bool Grid::isNoBuild(int gridX, int gridY) const {
	if (gridX >= 0 && gridX < m_width && gridY >= 0 && gridY < m_height) {
		return m_noBuildMask[gridY][gridX];
	}
	if (m_hasSidePockets) {
		return m_outerNoBuild.find({gridX, gridY}) != m_outerNoBuild.end();
	}
	return false;
}

void Grid::setNoBuildAt(int gridX, int gridY, bool value) {
	if (gridX >= 0 && gridX < m_width && gridY >= 0 && gridY < m_height) {
		m_noBuildMask[gridY][gridX] = value;
	} else if (m_hasSidePockets) {
		if (value) {
			m_outerNoBuild.insert({gridX, gridY});
		} else {
			m_outerNoBuild.erase({gridX, gridY});
		}
	}
}

void Grid::removeNoBuildAt(int gridX, int gridY) {
	setNoBuildAt(gridX, gridY, false);
}
