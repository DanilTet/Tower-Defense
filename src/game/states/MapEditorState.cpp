#include "MapEditorState.h"
#include "GameStateManager.h"
#include "MainMenuState.h"
#include "LevelSelectState.h"
#include "GameplayState.h"
#include "../renderer/SpriteRenderer.h"
#include "../renderer/TextRenderer.h"
#include "../resources/ResourceManager.h"
#include "../textures/Texture2D.h"
#include "../world/PathService.h"
#include "../ui/UICommon.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <queue>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include "../core/InputManager.h"
#include "../core/LocalizationManager.h"
#include "../core/CampaignManager.h"

static const std::vector<glm::ivec2> c_sizePresets = {
    { 8, 5 },
    { 10, 5 },
    { 12, 5 },
    { 10, 7 },
    { 16, 10 },
    { 20, 12 },
    { 24, 14 },
    { 30, 18 },
    { 40, 24 },
    { 50, 30 },
    { 64, 36 },
    { 80, 48 },
    { 100, 60 },
    { 128, 72 }
};

MapEditorState::MapEditorState(GameStateManager& stateManager, int width, int height,
                               std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer,
                               const std::string& levelToLoad, EditorOrigin origin)
    : m_stateManager(stateManager),
      m_width(width),
      m_height(height),
      m_renderer(renderer),
      m_textRenderer(textRenderer),
      m_origin(origin)
{
    if (!levelToLoad.empty()) {
        std::string fname = levelToLoad;
        size_t lastSlash = fname.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            fname = fname.substr(lastSlash + 1);
        }
        m_currentLevelFileName = fname;
    } else {
        m_mapsBrowserModal.open(m_currentLevelFileName, true);
    }
    std::cout << "[MapEditor] Ctor: levelToLoad='" << levelToLoad << "', currentLevel='" << m_currentLevelFileName
              << "', m_isMapsModalOpen=" << (m_mapsBrowserModal.isOpen() ? "true" : "false")
              << ", m_suppressClick=" << (m_suppressClickUntilRelease ? "true" : "false") << std::endl;
}

void MapEditorState::returnToOrigin() {
    if (m_origin == EditorOrigin::Campaign) {
        std::cout << "[MapEditor] Returning to Campaign LevelSelect..." << std::endl;
        m_stateManager.setState(std::make_unique<LevelSelectState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, LevelTab::Campaign));
    } else if (m_origin == EditorOrigin::Custom) {
        std::cout << "[MapEditor] Returning to Custom LevelSelect..." << std::endl;
        m_stateManager.setState(std::make_unique<LevelSelectState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, LevelTab::Custom));
    } else {
        std::cout << "[MapEditor] Returning to MainMenu..." << std::endl;
        m_stateManager.setState(std::make_unique<MainMenuState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer));
    }
}

glm::vec3 MapEditorState::getIdColor(int id) const {
    return EditorToolbarUI::getIdColor(id);
}

void MapEditorState::init() {
    std::cout << "[MapEditor] init(): resolution=" << m_width << "x" << m_height
              << ", m_isMapsModalOpen=" << (m_mapsBrowserModal.isOpen() ? "true" : "false")
              << ", m_suppressClick=" << (m_suppressClickUntilRelease ? "true" : "false") << std::endl;
    ResourceManager::loadTexture("uiBaseTexture", "res/textures/ui_space.png");
    ResourceManager::loadTexture("arrowTexture", "res/textures/pathArrow.png");

    auto wrapNoDelete = [](const std::string& name) {
        Texture2D* tex = ResourceManager::getTexture(name);
        return std::shared_ptr<Texture2D>(tex, [](Texture2D*) {});
    };

    m_uiTexture = wrapNoDelete("uiBaseTexture");
    m_arrowTexture = wrapNoDelete("arrowTexture");
    m_whiteTexture = std::shared_ptr<Texture2D>(ResourceManager::getWhiteTexture(), [](Texture2D*) {});

    loadInitialMap();
    updateButtonLayout();
}

void MapEditorState::cleanup() {
    std::cout << "[MapEditor] Cleanup completed" << std::endl;
}

void MapEditorState::updateCurrentLevelDisplayName() {
    std::string stem = m_currentLevelFileName;
    if (stem.length() >= 5 && stem.substr(stem.length() - 5) == ".json") {
        stem = stem.substr(0, stem.length() - 5);
    }
    if (stem == "level_editor") {
        m_currentLevelDisplayName = "MY MAP";
    } else {
        m_currentLevelDisplayName = stem;
    }
    m_editorSavePath = "res/levels/" + m_currentLevelFileName;
}

void MapEditorState::loadLevelByName(const std::string& fileName) {
    m_currentLevelFileName = LevelManager::sanitizeLevelFileName(fileName);
    std::cout << "[MapEditor] loadLevelByName: requesting '" << fileName << "' -> file='" << m_currentLevelFileName << "'" << std::endl;
    updateCurrentLevelDisplayName();

    std::string path = "res/levels/" + m_currentLevelFileName;
    LevelMapData data = LevelManager::loadLevelMap(path);
    if (data.gridWidth <= 0 || data.gridHeight <= 0) {
        auto dirs = LevelManager::getLevelDirectories();
        for (const auto& d : dirs) {
            std::string p = (std::filesystem::path(d) / m_currentLevelFileName).string();
            data = LevelManager::loadLevelMap(p);
            if (data.gridWidth > 0 && data.gridHeight > 0) break;
        }
    }

    if (data.gridWidth <= 0 || data.gridHeight <= 0) {
        if (m_currentLevelFileName != "level_editor.json") {
            data = LevelManager::loadLevelMap("res/levels/level_1.json");
        }
    }

    if (data.gridWidth > 0 && data.gridHeight > 0) {
        m_gridWidth = data.gridWidth;
        m_gridHeight = data.gridHeight;
        m_cellSize = data.cellSize;
        m_spawners = data.spawners;
        m_bases = data.bases;
        m_rawLayout = data.layout;
        m_minecarts = data.minecarts;
        m_waves = data.waves;
        m_isCampaign = data.isCampaign;
        m_tags = data.tags;
        if (!data.name.empty()) {
            m_currentLevelDisplayName = data.name;
        }
        m_suppressPlacementUntilRelease = true;
        std::cout << "[MapEditor] Successfully loaded: " << m_currentLevelFileName << " (\"" << m_currentLevelDisplayName << "\", " << m_gridWidth << "x" << m_gridHeight << ")" << std::endl;
    } else {
        m_gridWidth = 20;
        m_gridHeight = 12;
        m_cellSize = 64.0f;
        m_spawners.clear();
        m_bases.clear();
        m_bases.push_back(BaseData(17, 6, 0));
        SpawnerData sp;
        sp.pos = glm::ivec2(2, 6);
        sp.targetBaseIndex = 0;
        m_spawners.push_back(sp);
        m_rawLayout.assign(12, std::vector<int>(20, 0));
        m_minecarts.clear();
        m_waves.clear();
        m_isCampaign = false;
        m_tags = { "Тест" };
        m_currentLevelDisplayName = "MY MAP";
        std::cout << "[MapEditor] In-memory blank template 20x12 initialized (no disk write)" << std::endl;
    }

    if (m_minecarts.empty()) {
        m_minecarts.push_back(MinecartData{});
    }

    if (m_waves.empty()) {
        WaveConfig defWave;
        defWave.parts.push_back({ "Basic", 10, 0.8f, 2.0f });
        m_waves.push_back(defWave);
    }

    if (m_rawLayout.empty() || (int)m_rawLayout.size() != m_gridHeight) {
        m_rawLayout.assign(m_gridHeight, std::vector<int>(m_gridWidth, 0));
    }

    m_grid = std::make_unique<Grid>(m_gridWidth, m_gridHeight, m_cellSize);
    m_grid->updateCellSize(m_width, m_height, getBottomDockHeight() + 6.0f, getTopBarHeight() + 6.0f);

    for (int y = 0; y < m_gridHeight; ++y) {
        for (int x = 0; x < m_gridWidth; ++x) {
            int cellVal = m_rawLayout[y][x];
            if (cellVal == 1) {
                m_grid->setCellType(x, y, CellType::Path);
            } else if (cellVal == 2) {
                m_grid->setCellType(x, y, CellType::Platform);
            } else if (cellVal == 3) {
                m_grid->setCellType(x, y, CellType::Scenery);
            } else if (cellVal == 4) {
                m_grid->setCellType(x, y, CellType::Chasm);
            } else if (cellVal == 5) {
                m_grid->setCellType(x, y, CellType::Rail);
            } else {
                m_grid->setCellType(x, y, CellType::Ground);
            }
        }
    }

    for (const auto& sp : m_spawners) {
        if (sp.pos.x >= 0 && sp.pos.x < m_gridWidth && sp.pos.y >= 0 && sp.pos.y < m_gridHeight) {
            m_grid->setCellType(sp.pos.x, sp.pos.y, CellType::Spawner);
        }
    }

    for (const auto& b : m_bases) {
        if (b.x >= 0 && b.x < m_gridWidth && b.y >= 0 && b.y < m_gridHeight) {
            m_grid->setCellType(b.x, b.y, CellType::Base);
        }
    }

    m_grid->saveOriginalGrid();
    m_pathfinder = std::make_unique<Pathfinder>(m_gridWidth, m_gridHeight);
    recalculatePaths();

    m_isDirty = false;

    updateButtonLayout();
}

void MapEditorState::loadInitialMap() {
    loadLevelByName(m_currentLevelFileName);
}

void MapEditorState::resizeMap(int newW, int newH) {
    newW = std::clamp(newW, 1, 250);
    newH = std::clamp(newH, 1, 250);
    if (newW == m_gridWidth && newH == m_gridHeight) return;

    // Сохраняем существующие тайлы
    std::vector<std::vector<int>> newLayout(newH, std::vector<int>(newW, 0));
    for (int y = 0; y < std::min(m_gridHeight, newH); ++y) {
        for (int x = 0; x < std::min(m_gridWidth, newW); ++x) {
            newLayout[y][x] = m_rawLayout[y][x];
        }
    }
    m_rawLayout = newLayout;
    m_gridWidth = newW;
    m_gridHeight = newH;

    // Удаляем спавнеры и базы, вышедшие за границы новой сетки
    m_spawners.erase(
        std::remove_if(m_spawners.begin(), m_spawners.end(),
                       [newW, newH](const SpawnerData& s) { return s.pos.x >= newW || s.pos.y >= newH; }),
        m_spawners.end()
    );

    m_bases.erase(
        std::remove_if(m_bases.begin(), m_bases.end(),
                       [newW, newH](const BaseData& b) { return b.x >= newW || b.y >= newH; }),
        m_bases.end()
    );

    for (auto& mc : m_minecarts) {
        if (mc.start.x >= newW || mc.start.y >= newH) mc.start = glm::ivec2(-1, -1);
        if (mc.end.x >= newW || mc.end.y >= newH) mc.end = glm::ivec2(-1, -1);
    }

    m_grid = std::make_unique<Grid>(m_gridWidth, m_gridHeight, m_cellSize);
    m_grid->updateCellSize(m_width, m_height, getBottomDockHeight() + 6.0f, getTopBarHeight() + 6.0f);

    for (int y = 0; y < m_gridHeight; ++y) {
        for (int x = 0; x < m_gridWidth; ++x) {
            int cellVal = m_rawLayout[y][x];
            if (cellVal == 1) m_grid->setCellType(x, y, CellType::Path);
            else if (cellVal == 2) m_grid->setCellType(x, y, CellType::Platform);
            else if (cellVal == 3) m_grid->setCellType(x, y, CellType::Scenery);
            else if (cellVal == 4) m_grid->setCellType(x, y, CellType::Chasm);
            else if (cellVal == 5) m_grid->setCellType(x, y, CellType::Rail);
            else m_grid->setCellType(x, y, CellType::Ground);
        }
    }

    for (const auto& sp : m_spawners) {
        m_grid->setCellType(sp.pos.x, sp.pos.y, CellType::Spawner);
    }
    for (const auto& b : m_bases) {
        m_grid->setCellType(b.x, b.y, CellType::Base);
    }

    m_grid->saveOriginalGrid();
    m_pathfinder = std::make_unique<Pathfinder>(m_gridWidth, m_gridHeight);
    recalculatePaths();

    m_statusMessage = "Grid resized to " + std::to_string(m_gridWidth) + "x" + std::to_string(m_gridHeight);
    m_statusColor = glm::vec3(0.3f, 0.9f, 1.0f);
    m_statusTimer = 2.5f;

    updateButtonLayout();
}

void MapEditorState::cycleMapSizePreset() {
    int nextIdx = 0;
    for (size_t i = 0; i < c_sizePresets.size(); ++i) {
        if (c_sizePresets[i].x == m_gridWidth && c_sizePresets[i].y == m_gridHeight) {
            nextIdx = (i + 1) % c_sizePresets.size();
            break;
        }
    }
    resizeMap(c_sizePresets[nextIdx].x, c_sizePresets[nextIdx].y);
}

void MapEditorState::cycleSelectedId(int step) {
    if (step > 0) {
        m_selectedId = (m_selectedId >= 5) ? -1 : (m_selectedId + 1);
    } else if (step < 0) {
        m_selectedId = (m_selectedId <= -1) ? 5 : (m_selectedId - 1);
    }

    std::string idStr = (m_selectedId == -1) ? "Auto" : ("#" + std::to_string(m_selectedId));
    m_statusMessage = "Selected Active ID: " + idStr + (m_selectedId == -1 ? " (Nearest Base)" : "");
    m_statusColor = getIdColor(m_selectedId);
    m_statusTimer = 2.5f;

    updateButtonLayout();
}

float MapEditorState::getTopBarHeight() const {
    return EditorToolbarUI::calculateTopBarHeight(m_width, m_height);
}

float MapEditorState::getBottomDockHeight() const {
    return EditorToolbarUI::calculateBottomDockHeight(m_width, m_height);
}

void MapEditorState::showToast(const std::string& msg, const glm::vec4& color) {
    m_toastMessage = msg;
    m_toastColor = color;
    m_toastTimer = 2.8f;
}

EditorContext MapEditorState::buildEditorContext() const {
    EditorContext ctx;
    ctx.screenWidth = m_width;
    ctx.screenHeight = m_height;
    ctx.currentLevelDisplayName = m_currentLevelDisplayName;
    ctx.currentLevelFileName = m_currentLevelFileName;
    ctx.isCampaign = m_isCampaign;
    ctx.gridWidth = m_gridWidth;
    ctx.gridHeight = m_gridHeight;
    ctx.currentBrush = m_currentBrush;
    ctx.selectedId = m_selectedId;
    ctx.statusMessage = m_statusMessage;
    ctx.statusTimer = m_statusTimer;
    ctx.statusColor = m_statusColor;
    ctx.hasInvalidSpawner = m_hasInvalidSpawner;
    ctx.missingBaseWarning = m_missingBaseWarning;
    ctx.missingBaseId = m_missingBaseId;
    ctx.hasMinecartStartEnd = !m_minecarts.empty() && m_minecarts[0].hasStart() && m_minecarts[0].hasEnd();
    ctx.isRailPathValid = m_isRailPathValid;
    ctx.railPathSize = m_railPath.size();
    ctx.isWaveModalOpen = m_waveModal.isOpen();
    ctx.isMinecartModalOpen = m_minecartModal.isOpen();
    ctx.currentCategory = m_toolbarUI.getCategory();
    return ctx;
}

void MapEditorState::updateButtonLayout() {
    m_toolbarUI.updateLayout(m_textRenderer, buildEditorContext());
}

bool MapEditorState::isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) const {
    return point.x >= rectPos.x && point.x <= rectPos.x + rectSize.x &&
           point.y >= rectPos.y && point.y <= rectPos.y + rectSize.y;
}

void MapEditorState::recalculatePaths() {
    if (!m_grid || !m_pathfinder) return;

    m_activePaths = PathService::calculateAllPaths(*m_grid, *m_pathfinder, m_spawners, m_bases);

    bool wasInvalidSpawner = m_hasInvalidSpawner;
    m_hasInvalidSpawner = false;
    m_missingBaseWarning = false;
    m_missingBaseId = -1;

    for (size_t i = 0; i < m_spawners.size(); ++i) {
        if (i >= m_activePaths.size() || m_activePaths[i].empty()) {
            m_hasInvalidSpawner = true;
        }

        int targetId = m_spawners[i].targetBaseIndex;
        if (targetId >= 0) {
            bool found = false;
            for (const auto& b : m_bases) {
                if (b.id == targetId) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                m_missingBaseWarning = true;
                m_missingBaseId = targetId;
            }
        }
    }

    if (m_hasInvalidSpawner && !wasInvalidSpawner) {
        showToast("Ошибка пути: спавнер заблокирован!", glm::vec4(0.95f, 0.25f, 0.25f, 1.0f));
    }

    // Перераховуємо маршрут рейок після будь-якої зміни сітки
    recalculateRailPath();
}

// ---------------------------------------------------------------------------
// BFS-валідація маршруту рейок: від m_minecarts[0].start до .end
// ---------------------------------------------------------------------------
void MapEditorState::recalculateRailPath() {
    bool wasRailValid = m_isRailPathValid;
    m_railPath.clear();
    m_isRailPathValid = false;

    if (!m_grid || m_minecarts.empty()) return;
    const auto& mc = m_minecarts[0];
    if (!mc.hasStart() || !mc.hasEnd()) return;

    glm::ivec2 startPt = mc.start;
    glm::ivec2 endPt   = mc.end;

    // Клітинка вважається рейковою якщо rawLayout == 5
    auto isRail = [&](int x, int y) -> bool {
        if (x < 0 || x >= m_gridWidth || y < 0 || y >= m_gridHeight) return false;
        return m_rawLayout[y][x] == 5;
    };

    if (!isRail(startPt.x, startPt.y) || !isRail(endPt.x, endPt.y)) return;

    // BFS (4-напрямки)
    const glm::ivec2 dirs[4] = { {0,-1}, {0,1}, {-1,0}, {1,0} };
    std::vector<std::vector<glm::ivec2>> parent(
        m_gridHeight, std::vector<glm::ivec2>(m_gridWidth, {-1, -1}));
    std::vector<std::vector<bool>> visited(
        m_gridHeight, std::vector<bool>(m_gridWidth, false));

    std::queue<glm::ivec2> q;
    q.push(startPt);
    visited[startPt.y][startPt.x] = true;
    parent[startPt.y][startPt.x] = startPt; // sentinel

    bool found = false;
    while (!q.empty()) {
        glm::ivec2 cur = q.front(); q.pop();
        if (cur.x == endPt.x && cur.y == endPt.y) { found = true; break; }
        for (const auto& d : dirs) {
            int nx = cur.x + d.x, ny = cur.y + d.y;
            if (isRail(nx, ny) && !visited[ny][nx]) {
                visited[ny][nx] = true;
                parent[ny][nx] = cur;
                q.push({nx, ny});
            }
        }
    }

    if (!found) {
        if (wasRailValid && mc.hasStart() && mc.hasEnd()) {
            showToast("Ошибка рельсов: маршрут разорван!", glm::vec4(0.95f, 0.25f, 0.25f, 1.0f));
        }
        return;
    }

    // Відновлення ланцюжка (зворотно від end до start)
    std::vector<glm::ivec2> path;
    glm::ivec2 cur = endPt;
    while (!(cur.x == startPt.x && cur.y == startPt.y)) {
        path.push_back(cur);
        cur = parent[cur.y][cur.x];
    }
    path.push_back(startPt);
    std::reverse(path.begin(), path.end());

    m_railPath = std::move(path);
    m_isRailPathValid = true;
}

void MapEditorState::applyBrush(int gridX, int gridY, EditorBrush brush) {
    if (gridX < 0 || gridX >= m_gridWidth || gridY < 0 || gridY >= m_gridHeight) return;

    // Режим ластика: очищает клетку до чистой Земли
    if (brush == EditorBrush::Eraser) {
        eraseCell(gridX, gridY);
        return;
    }

    // Особый случай: клик по УЖЕ существующему спавнеру
    if (brush == EditorBrush::Spawner) {
        for (auto& sp : m_spawners) {
            if (sp.pos.x == gridX && sp.pos.y == gridY) {
                if (sp.targetBaseIndex == m_selectedId) {
                    sp.targetBaseIndex = (sp.targetBaseIndex >= 5) ? -1 : (sp.targetBaseIndex + 1);
                } else {
                    sp.targetBaseIndex = m_selectedId;
                }
                std::string idStr = (sp.targetBaseIndex == -1) ? "Auto" : ("#" + std::to_string(sp.targetBaseIndex));
                m_statusMessage = "Spawner at (" + std::to_string(gridX) + "," + std::to_string(gridY) + ") target Base ID set to: " + idStr;
                m_statusColor = getIdColor(sp.targetBaseIndex);
                m_statusTimer = 2.5f;
                recalculatePaths();
                return;
            }
        }
    }

    // Особый случай: клик по УЖЕ существующей базе
    if (brush == EditorBrush::Base) {
        int baseTargetId = (m_selectedId < 0) ? 0 : m_selectedId;
        for (auto& b : m_bases) {
            if (b.x == gridX && b.y == gridY) {
                if (b.id == baseTargetId) {
                    b.id = (b.id >= 5) ? 0 : (b.id + 1);
                } else {
                    b.id = baseTargetId;
                }
                m_statusMessage = "Base at (" + std::to_string(gridX) + "," + std::to_string(gridY) + ") ID set to: #" + std::to_string(b.id);
                m_statusColor = getIdColor(b.id);
                m_statusTimer = 2.5f;
                recalculatePaths();
                return;
            }
        }
    }

    // Иначе удаляем старый спавнер или базу в этой точке
    m_spawners.erase(
        std::remove_if(m_spawners.begin(), m_spawners.end(),
                       [gridX, gridY](const SpawnerData& s) { return s.pos.x == gridX && s.pos.y == gridY; }),
        m_spawners.end()
    );

    m_bases.erase(
        std::remove_if(m_bases.begin(), m_bases.end(),
                       [gridX, gridY](const BaseData& b) { return b.x == gridX && b.y == gridY; }),
        m_bases.end()
    );

    // Сброс маркеров вагонетки, если на их место ставится не-рейковый тайл
    if (brush != EditorBrush::Rail && brush != EditorBrush::RailStart && brush != EditorBrush::RailEnd) {
        if (!m_minecarts.empty()) {
            if (m_minecarts[0].start == glm::ivec2(gridX, gridY)) m_minecarts[0].start = glm::ivec2(-1, -1);
            if (m_minecarts[0].end == glm::ivec2(gridX, gridY)) m_minecarts[0].end = glm::ivec2(-1, -1);
        }
    }

    switch (brush) {
        case EditorBrush::Ground:
            m_rawLayout[gridY][gridX] = 0;
            m_grid->setCellType(gridX, gridY, CellType::Ground);
            break;
        case EditorBrush::Wall:
            m_rawLayout[gridY][gridX] = 3;
            m_grid->setCellType(gridX, gridY, CellType::Scenery);
            break;
        case EditorBrush::Platform:
            m_rawLayout[gridY][gridX] = 2;
            m_grid->setCellType(gridX, gridY, CellType::Platform);
            break;
        case EditorBrush::Path:
            m_rawLayout[gridY][gridX] = 1;
            m_grid->setCellType(gridX, gridY, CellType::Path);
            break;
        case EditorBrush::Chasm:
            m_rawLayout[gridY][gridX] = 4;
            m_grid->setCellType(gridX, gridY, CellType::Chasm);
            break;
        case EditorBrush::Rail:
            m_rawLayout[gridY][gridX] = 5;
            m_grid->setCellType(gridX, gridY, CellType::Rail);
            break;
        case EditorBrush::RailStart:
            if (m_minecarts.empty()) m_minecarts.push_back(MinecartData{});
            m_minecarts[0].start = glm::ivec2(gridX, gridY);
            m_rawLayout[gridY][gridX] = 5;
            m_grid->setCellType(gridX, gridY, CellType::Rail);
            m_statusMessage = "Minecart Depot [Start] set at (" + std::to_string(gridX) + ", " + std::to_string(gridY) + ")";
            m_statusColor = glm::vec3(1.0f, 0.75f, 0.2f);
            m_statusTimer = 2.0f;
            break;
        case EditorBrush::RailEnd:
            if (m_minecarts.empty()) m_minecarts.push_back(MinecartData{});
            m_minecarts[0].end = glm::ivec2(gridX, gridY);
            m_rawLayout[gridY][gridX] = 5;
            m_grid->setCellType(gridX, gridY, CellType::Rail);
            m_statusMessage = "Minecart Buffer [End] set at (" + std::to_string(gridX) + ", " + std::to_string(gridY) + ")";
            m_statusColor = glm::vec3(1.0f, 0.35f, 0.35f);
            m_statusTimer = 2.0f;
            break;
        case EditorBrush::Spawner:
            m_rawLayout[gridY][gridX] = 0;
            m_grid->setCellType(gridX, gridY, CellType::Spawner);
            m_spawners.push_back({ glm::ivec2(gridX, gridY), m_selectedId });
            {
                std::string idStr = (m_selectedId == -1) ? "Auto" : ("#" + std::to_string(m_selectedId));
                m_statusMessage = "Placed Spawner with target Base ID: " + idStr;
                m_statusColor = getIdColor(m_selectedId);
                m_statusTimer = 2.0f;
            }
            break;
        case EditorBrush::Base:
            {
                int baseId = (m_selectedId < 0) ? 0 : m_selectedId;
                m_rawLayout[gridY][gridX] = 0;
                m_grid->setCellType(gridX, gridY, CellType::Base);
                m_bases.push_back(BaseData(gridX, gridY, baseId));
                m_statusMessage = "Placed Base with ID: #" + std::to_string(baseId);
                m_statusColor = getIdColor(baseId);
                m_statusTimer = 2.0f;
            }
            break;
        default:
            break;
    }

    m_grid->saveOriginalGrid();
    recalculatePaths();
    m_isDirty = true;
}

void MapEditorState::eraseCell(int gridX, int gridY) {
    if (gridX < 0 || gridX >= m_gridWidth || gridY < 0 || gridY >= m_gridHeight) return;

    m_spawners.erase(
        std::remove_if(m_spawners.begin(), m_spawners.end(),
                       [gridX, gridY](const SpawnerData& s) { return s.pos.x == gridX && s.pos.y == gridY; }),
        m_spawners.end()
    );

    m_bases.erase(
        std::remove_if(m_bases.begin(), m_bases.end(),
                       [gridX, gridY](const BaseData& b) { return b.x == gridX && b.y == gridY; }),
        m_bases.end()
    );

    if (!m_minecarts.empty()) {
        if (m_minecarts[0].start == glm::ivec2(gridX, gridY)) {
            m_minecarts[0].start = glm::ivec2(-1, -1);
        }
        if (m_minecarts[0].end == glm::ivec2(gridX, gridY)) {
            m_minecarts[0].end = glm::ivec2(-1, -1);
        }
    }

    m_rawLayout[gridY][gridX] = 0;
    m_grid->setCellType(gridX, gridY, CellType::Ground);
    m_grid->saveOriginalGrid();
    recalculatePaths();
    m_isDirty = true;
}

void MapEditorState::saveMap() {
    LevelMapData data;
    data.name = m_currentLevelDisplayName;
    data.isCampaign = m_isCampaign;
    data.tags = m_tags;
    data.gridWidth = m_gridWidth;
    data.gridHeight = m_gridHeight;
    data.cellSize = m_grid->getCellSize();
    data.offsetX = m_grid->getOffset().x;
    data.offsetY = m_grid->getOffset().y;
    data.spawners = m_spawners;
    data.bases = m_bases;
    data.layout = m_rawLayout;

    std::vector<MinecartData> activeCarts;
    for (const auto& mc : m_minecarts) {
        if (mc.hasStart() || mc.hasEnd()) {
            activeCarts.push_back(mc);
        }
    }
    data.minecarts = activeCarts;
    data.waves = m_waves;

    bool ok = LevelManager::saveLevel(m_currentLevelFileName, data);
    std::cout << "[MapEditor] saveMap: file=" << m_currentLevelFileName << ", name=" << m_currentLevelDisplayName << ", size=" << m_gridWidth << "x" << m_gridHeight << ", result=" << (ok ? "SUCCESS" : "FAIL") << std::endl;

    if (ok) {
        m_isDirty = false;
        m_statusMessage = "Map saved: " + m_currentLevelDisplayName + " (" + std::to_string(m_gridWidth) + "x" + std::to_string(m_gridHeight) + ")!";
        m_statusColor = glm::vec3(0.2f, 1.0f, 0.3f);
        showToast("Карта успешно сохранена!", glm::vec4(0.2f, 0.8f, 0.3f, 1.0f));
    } else {
        m_statusMessage = "Error saving map file!";
        m_statusColor = glm::vec3(1.0f, 0.3f, 0.3f);
        showToast("Ошибка сохранения файла!", glm::vec4(0.95f, 0.25f, 0.25f, 1.0f));
    }
    m_statusTimer = 3.5f;
}

void MapEditorState::clearMap() {
    m_spawners.clear();
    m_bases.clear();
    m_minecarts.clear();
    m_minecarts.push_back(MinecartData{});
    for (int y = 0; y < m_gridHeight; ++y) {
        for (int x = 0; x < m_gridWidth; ++x) {
            m_rawLayout[y][x] = 0;
            m_grid->setCellType(x, y, CellType::Ground);
        }
    }
    m_grid->saveOriginalGrid();
    recalculatePaths();
    m_statusMessage = "Map cleared to Ground.";
    m_statusColor = glm::vec3(0.9f, 0.9f, 0.3f);
    m_statusTimer = 2.0f;
    showToast("Карта очищена", glm::vec4(0.9f, 0.9f, 0.3f, 1.0f));
}

void MapEditorState::testMap() {
    if (m_spawners.empty() || m_bases.empty()) {
        m_statusMessage = "Cannot test: Place at least 1 Spawner and 1 Base!";
        m_statusColor = glm::vec3(1.0f, 0.3f, 0.3f);
        m_statusTimer = 4.0f;
        showToast("Нельзя тестировать: добавьте спавнер и базу!", glm::vec4(0.95f, 0.25f, 0.25f, 1.0f));
        return;
    }

    if (m_hasInvalidSpawner) {
        m_statusMessage = "Cannot test: One or more Spawners have no path to a Base!";
        m_statusColor = glm::vec3(1.0f, 0.2f, 0.2f);
        m_statusTimer = 4.0f;
        showToast("Ошибка валидации: спавнер заблокирован!", glm::vec4(0.95f, 0.25f, 0.25f, 1.0f));
        return;
    }

    saveMap();
    std::string testPath = "res/levels/" + m_currentLevelFileName;
    std::cout << "[MapEditor] Launching test gameplay with " << testPath << std::endl;
    m_suppressPlacementUntilRelease = true;
    m_stateManager.pushState(std::make_unique<GameplayState>(m_stateManager, m_width, m_height, m_renderer, m_textRenderer, testPath, /*isEditorTest=*/true, GameplayOrigin::EditorTest));
}

void MapEditorState::processInput(GLFWwindow* window, float dt) {
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    glm::vec2 mousePos(static_cast<float>(mouseX), static_cast<float>(mouseY));
    m_mousePos = mousePos;

    bool leftDown = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
    bool rightDown = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);

    if (m_diagInputFrames < 6) {
        std::cout << "[MapEditor] processInput frame " << m_diagInputFrames
                  << ": leftDown=" << leftDown << ", rightDown=" << rightDown
                  << ", m_isLeftMouseDown=" << m_isLeftMouseDown
                  << ", m_suppressClick=" << m_suppressClickUntilRelease
                  << ", m_isMapsModalOpen=" << (m_mapsBrowserModal.isOpen() ? "true" : "false")
                  << ", mousePos=(" << mousePos.x << "," << mousePos.y << ")" << std::endl;
        m_diagInputFrames++;
    }

    // Подавляем клики от предыдущего состояния до полного отпускания кнопок мыши
    if (m_suppressClickUntilRelease) {
        if (!leftDown && !rightDown) {
            std::cout << "[MapEditor] Mouse released -> m_suppressClickUntilRelease cleared" << std::endl;
            m_suppressClickUntilRelease = false;
            m_isLeftMouseDown = false;
            m_isRightMouseDown = false;
        } else {
            m_isLeftMouseDown = leftDown;
            m_isRightMouseDown = rightDown;
            return;
        }
    }

    // Если открыто модальное окно подтверждения выхода
    if (m_isExitModalOpen) {
        bool handled = processExitModalInput(window, mousePos, leftDown, dt);
        m_isLeftMouseDown = leftDown;
        m_isRightMouseDown = rightDown;
        if (handled) return;
    }

    // Если открыто модальное окно управления картами (список карт / переименование)
    if (m_mapsBrowserModal.isOpen()) {
        bool handled = m_mapsBrowserModal.handleInput(
            window, mousePos, leftDown, dt, m_width, m_height,
            /* onLoadLevel */ [this](const std::string& fileName) {
                loadLevelByName(fileName);
                m_statusMessage = "Loaded map: " + m_currentLevelDisplayName;
                m_statusColor = glm::vec3(0.3f, 0.9f, 1.0f);
                m_statusTimer = 3.0f;
            },
            /* onNewLevel */ [this]() {
                std::string newF = LevelManager::createNewLevel("custom_map");
                loadLevelByName(newF);
                m_statusMessage = "Created and loaded: " + m_currentLevelDisplayName;
                m_statusColor = glm::vec3(0.2f, 1.0f, 0.4f);
                m_statusTimer = 3.5f;
            },
            /* onRenameLevel */ [this](const std::string& oldFile, const std::string& newFile, const std::string& newName) {
                if (oldFile == m_currentLevelFileName) {
                    m_currentLevelFileName = newFile;
                    m_currentLevelDisplayName = newName;
                    m_editorSavePath = "res/levels/" + m_currentLevelFileName;
                }
                m_statusMessage = "Renamed: " + newName;
                m_statusColor = glm::vec3(0.2f, 1.0f, 0.3f);
                m_statusTimer = 3.5f;
                updateButtonLayout();
            },
            /* onReturnToOrigin */ [this]() {
                returnToOrigin();
            }
        );
        m_isLeftMouseDown = leftDown;
        m_isRightMouseDown = rightDown;
        m_suppressPlacementUntilRelease = true;
        if (handled) return;
    }

    // Если открыто модальное окно настроек вагонетки
    if (m_minecartModal.isOpen()) {
        bool handled = m_minecartModal.handleInput(window, mousePos, leftDown, m_width, m_height,
                                                   m_minecarts, m_isDirty,
                                                   [this]() { recalculateRailPath(); });
        m_isLeftMouseDown = leftDown;
        m_isRightMouseDown = rightDown;
        if (handled) return;
    }

    // Горячая клавиша Waves (W)
    bool keyW = (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS);
    if (keyW && !m_keyWPressedLastFrame) {
        m_waveModal.toggle(m_waves);
        if (m_waveModal.isOpen()) {
            m_statusMessage = "Wave Editor: Configure waves, sub-waves and pacing.";
            m_statusColor = glm::vec3(0.3f, 0.85f, 1.0f);
            m_statusTimer = 2.0f;
        }
    }
    m_keyWPressedLastFrame = keyW;

    // Горячая клавиша Escape
    bool keyEsc = (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS);
    if (keyEsc && !m_keyEscPressedLastFrame) {
        if (m_isExitModalOpen) {
            closeExitModal();
            m_keyEscPressedLastFrame = keyEsc;
            return;
        }
        if (m_mapsBrowserModal.isOpen()) {
            m_mapsBrowserModal.close();
            m_keyEscPressedLastFrame = keyEsc;
            return;
        }
        if (m_waveModal.isOpen()) {
            m_waveModal.close();
            m_keyEscPressedLastFrame = keyEsc;
            return;
        }
        if (m_minecartModal.isOpen()) {
            m_minecartModal.close();
            m_keyEscPressedLastFrame = keyEsc;
            return;
        }
        openExitModal();
        m_keyEscPressedLastFrame = keyEsc;
        return;
    }
    m_keyEscPressedLastFrame = keyEsc;

    // Если открыт редактор волн, перехватываем ввод
    if (m_waveModal.isOpen()) {
        bool handled = m_waveModal.handleInput(window, mousePos, leftDown, dt,
                                               m_width, m_height, getTopBarHeight(), getBottomDockHeight(),
                                               m_waves, m_isDirty);
        m_isLeftMouseDown = leftDown;
        m_isRightMouseDown = rightDown;
        if (handled) return;
    }

    bool isShift = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
    bool isAlt   = (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS);
    bool isCtrl  = (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS);

    // Переключатель видимости стрелок путей (V)
    bool keyV = (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS);
    if (keyV && !m_keyVPressedLastFrame) {
        m_showPathArrows = !m_showPathArrows;
        showToast(m_showPathArrows ? "Стрелки путей: ВКЛ" : "Стрелки путей: ВЫКЛ",
                  m_showPathArrows ? glm::vec4(0.3f, 0.85f, 0.4f, 1.0f) : glm::vec4(0.7f, 0.7f, 0.7f, 1.0f));
    }
    m_keyVPressedLastFrame = keyV;

    // Быстрое переключение категорий палитры (клавиша TAB или Ctrl+1 / Ctrl+2)
    bool keyTab = (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS);
    if (keyTab && !m_keyTabPressedLastFrame) {
        if (m_toolbarUI.getCategory() == PaletteCategory::Tiles) {
            m_toolbarUI.setCategory(PaletteCategory::SpecialObjects);
            showToast("Палитра: Спец-объекты");
        } else {
            m_toolbarUI.setCategory(PaletteCategory::Tiles);
            showToast("Палитра: Тайлы");
        }
        updateButtonLayout();
    }
    m_keyTabPressedLastFrame = keyTab;

    if (isCtrl) {
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
            if (m_toolbarUI.getCategory() != PaletteCategory::Tiles) {
                m_toolbarUI.setCategory(PaletteCategory::Tiles);
                updateButtonLayout();
                showToast("Палитра: Тайлы");
            }
        } else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
            if (m_toolbarUI.getCategory() != PaletteCategory::SpecialObjects) {
                m_toolbarUI.setCategory(PaletteCategory::SpecialObjects);
                updateButtonLayout();
                showToast("Палитра: Спец-объекты");
            }
        }
    } else if (!isAlt) {
        // Цифровые горячие клавиши внутри активной вкладки
        bool key1 = (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS);
        bool key2 = (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS);
        bool key3 = (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS);
        bool key4 = (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS);
        bool key5 = (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS);
        bool key6 = (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS);
        bool key7 = (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS);
        bool key8 = (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS);
        bool key0 = (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS);

        if (key0) {
            m_currentBrush = EditorBrush::Eraser;
        } else if (m_toolbarUI.getCategory() == PaletteCategory::Tiles) {
            if (key1) m_currentBrush = EditorBrush::Ground;
            if (key2) m_currentBrush = EditorBrush::Wall;
            if (key3) m_currentBrush = EditorBrush::Platform;
            if (key4) m_currentBrush = EditorBrush::Path;
            if (key5) m_currentBrush = EditorBrush::Chasm;
        } else { // SpecialObjects
            if (key1) m_currentBrush = EditorBrush::Spawner;
            if (key2) m_currentBrush = EditorBrush::Base;
            if (key3) m_currentBrush = EditorBrush::Rail;
            if (key4) m_currentBrush = EditorBrush::RailStart;
            if (key5) m_currentBrush = EditorBrush::RailEnd;
        }

        // Прямые клавиши 6, 7, 8 с переключением категории при необходимости
        if (key6) {
            m_currentBrush = EditorBrush::Base;
            if (m_toolbarUI.getCategory() != PaletteCategory::SpecialObjects) {
                m_toolbarUI.setCategory(PaletteCategory::SpecialObjects);
                updateButtonLayout();
            }
        }
        if (key7) {
            m_currentBrush = EditorBrush::Chasm;
            if (m_toolbarUI.getCategory() != PaletteCategory::Tiles) {
                m_toolbarUI.setCategory(PaletteCategory::Tiles);
                updateButtonLayout();
            }
        }
        if (key8) {
            m_currentBrush = EditorBrush::Rail;
            if (m_toolbarUI.getCategory() != PaletteCategory::SpecialObjects) {
                m_toolbarUI.setCategory(PaletteCategory::SpecialObjects);
                updateButtonLayout();
            }
        }
    }

    // Горячие клавиши переключения ID: [ и ] или - и =
    bool keyLeftBracket = (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS);
    if (keyLeftBracket && !m_keyLeftBracketLastFrame) {
        cycleSelectedId(-1);
    }
    m_keyLeftBracketLastFrame = keyLeftBracket;

    bool keyRightBracket = (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS);
    if (keyRightBracket && !m_keyRightBracketLastFrame) {
        cycleSelectedId(+1);
    }
    m_keyRightBracketLastFrame = keyRightBracket;

    // Горячая клавиша Save (Ctrl + S или S)
    bool keyS = (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS);
    if (keyS && !m_keySPressedLastFrame) {
        saveMap();
    }
    m_keySPressedLastFrame = keyS;

    // Горячая клавиша Test (T)
    bool keyT = (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS);
    if (keyT && !m_keyTPressedLastFrame) {
        testMap();
        return;
    }
    m_keyTPressedLastFrame = keyT;

    // Горячая клавиша Clear (C)
    bool keyC = (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS);
    if (keyC && !m_keyCPressedLastFrame) {
        clearMap();
    }
    m_keyCPressedLastFrame = keyC;

    bool clickedUI = false;

    // 1. Проверка клика по кнопкам тулбара и палитры через EditorToolbarUI
    EditorAction action = m_toolbarUI.handleInput(mousePos.x, mousePos.y, leftDown, m_isLeftMouseDown, isShift, buildEditorContext());
    if (action.type != EditorActionType::None) {
        clickedUI = true;
        m_suppressPlacementUntilRelease = true;
        switch (action.type) {
            case EditorActionType::SetBrush:
                m_currentBrush = action.brush;
                if (m_currentBrush == EditorBrush::Spawner || m_currentBrush == EditorBrush::Base ||
                    m_currentBrush == EditorBrush::Rail || m_currentBrush == EditorBrush::RailStart ||
                    m_currentBrush == EditorBrush::RailEnd) {
                    if (m_toolbarUI.getCategory() != PaletteCategory::SpecialObjects) {
                        m_toolbarUI.setCategory(PaletteCategory::SpecialObjects);
                        updateButtonLayout();
                    }
                } else {
                    if (m_toolbarUI.getCategory() != PaletteCategory::Tiles) {
                        m_toolbarUI.setCategory(PaletteCategory::Tiles);
                        updateButtonLayout();
                    }
                }
                break;
            case EditorActionType::SwitchPaletteCategory:
                updateButtonLayout();
                showToast(m_toolbarUI.getCategory() == PaletteCategory::Tiles ? "Палитра: Тайлы" : "Палитра: Спец-объекты");
                break;
            case EditorActionType::SaveMap:
                saveMap();
                break;
            case EditorActionType::TestMap:
                testMap();
                return;
            case EditorActionType::ClearMap:
                clearMap();
                break;
            case EditorActionType::Exit:
                openExitModal();
                m_isLeftMouseDown = leftDown;
                return;
            case EditorActionType::CycleSelectedId:
                cycleSelectedId(action.intParam);
                break;
            case EditorActionType::CyclePreset:
                cycleMapSizePreset();
                break;
            case EditorActionType::ResizeMap:
                resizeMap(m_gridWidth + action.intParam, m_gridHeight + action.intParam2);
                break;
            case EditorActionType::OpenRenameModal:
                m_mapsBrowserModal.openRename(m_currentLevelFileName, m_currentLevelDisplayName);
                m_suppressClickUntilRelease = true;
                break;
            case EditorActionType::NewMap: {
                std::string newF = LevelManager::createNewLevel("custom_map");
                loadLevelByName(newF);
                m_suppressClickUntilRelease = true;
                m_isLeftMouseDown = true;
                m_statusMessage = "Created map: " + m_currentLevelDisplayName;
                m_statusColor = glm::vec3(0.2f, 1.0f, 0.4f);
                m_statusTimer = 3.5f;
                break;
            }
            case EditorActionType::OpenMapsModal:
                m_mapsBrowserModal.open(m_currentLevelFileName, false);
                m_suppressClickUntilRelease = true;
                break;
            case EditorActionType::ToggleCampaign:
                m_isCampaign = !m_isCampaign;
                LevelManager::setLevelCampaign(m_currentLevelFileName, m_isCampaign);
                m_statusMessage = m_isCampaign ? "Map type: CAMPAIGN" : "Map type: TEST";
                m_statusColor = m_isCampaign ? glm::vec3(0.3f, 1.0f, 0.4f) : glm::vec3(1.0f, 0.7f, 0.2f);
                m_statusTimer = 2.5f;
                updateButtonLayout();
                break;
            case EditorActionType::ToggleWavesModal:
                m_waveModal.toggle(m_waves);
                break;
            case EditorActionType::ToggleMinecartModal:
                m_minecartModal.toggle();
                break;
            default:
                break;
        }
    }

    // 2. Если кликаем по рабочей области сетки
    if (!clickedUI && !m_toolbarUI.isHovered(mousePos.x, mousePos.y) && m_grid) {
        glm::ivec2 gridPos = m_grid->pixelToGrid(mousePos);
        bool inBounds = (gridPos.x >= 0 && gridPos.x < m_gridWidth && gridPos.y >= 0 && gridPos.y < m_gridHeight);

        // 2.1. ПИПЕТКА: Alt + клик ЛКМ по ячейке
        if (isAlt && leftDown && !m_isLeftMouseDown) {
            if (inBounds) {
                int gx = gridPos.x;
                int gy = gridPos.y;
                EditorBrush pickedBrush = EditorBrush::Ground;
                std::string tileName = "Земля";

                // Спавнеры
                bool foundObj = false;
                for (const auto& sp : m_spawners) {
                    if (sp.pos.x == gx && sp.pos.y == gy) {
                        pickedBrush = EditorBrush::Spawner;
                        tileName = "Спавнер";
                        m_selectedId = sp.targetBaseIndex;
                        foundObj = true;
                        break;
                    }
                }
                // Базы
                if (!foundObj) {
                    for (const auto& b : m_bases) {
                        if (b.x == gx && b.y == gy) {
                            pickedBrush = EditorBrush::Base;
                            tileName = "База";
                            m_selectedId = b.id;
                            foundObj = true;
                            break;
                        }
                    }
                }
                // Маркеры вагонетки
                if (!foundObj && !m_minecarts.empty()) {
                    const auto& mc = m_minecarts[0];
                    if (mc.hasStart() && mc.start.x == gx && mc.start.y == gy) {
                        pickedBrush = EditorBrush::RailStart;
                        tileName = "Депо";
                        foundObj = true;
                    } else if (mc.hasEnd() && mc.end.x == gx && mc.end.y == gy) {
                        pickedBrush = EditorBrush::RailEnd;
                        tileName = "Тупик";
                        foundObj = true;
                    }
                }
                // Типы тайлов из сырой сетки
                if (!foundObj) {
                    int raw = m_rawLayout[gy][gx];
                    switch (raw) {
                        case 0: pickedBrush = EditorBrush::Ground;   tileName = "Земля"; break;
                        case 1: pickedBrush = EditorBrush::Path;     tileName = "Дорога"; break;
                        case 2: pickedBrush = EditorBrush::Platform; tileName = "Платформа"; break;
                        case 3: pickedBrush = EditorBrush::Wall;     tileName = "Стена"; break;
                        case 4: pickedBrush = EditorBrush::Chasm;    tileName = "Шурф"; break;
                        case 5: pickedBrush = EditorBrush::Rail;     tileName = "Рельсы"; break;
                        default: pickedBrush = EditorBrush::Ground;  tileName = "Земля"; break;
                    }
                }

                m_currentBrush = pickedBrush;
                if (pickedBrush == EditorBrush::Spawner || pickedBrush == EditorBrush::Base ||
                    pickedBrush == EditorBrush::Rail || pickedBrush == EditorBrush::RailStart ||
                    pickedBrush == EditorBrush::RailEnd) {
                    m_toolbarUI.setCategory(PaletteCategory::SpecialObjects);
                } else {
                    m_toolbarUI.setCategory(PaletteCategory::Tiles);
                }
                updateButtonLayout();

                showToast("Выбран тайл: " + tileName, glm::vec4(0.3f, 0.8f, 1.0f, 1.0f));
                m_suppressPlacementUntilRelease = true;
            }
        }
        // 2.2. ПРЯМОУГОЛЬНАЯ ЗАЛИВКА: Shift + зажатый ЛКМ
        else if (isShift && !isAlt) {
            if (leftDown) {
                if (!m_isLeftMouseDown && inBounds) {
                    m_isBoxFilling = true;
                    m_boxStartCell = gridPos;
                    m_boxCurrentCell = gridPos;
                } else if (m_isBoxFilling && inBounds) {
                    m_boxCurrentCell = gridPos;
                }
            }
        }
        // 2.3. СТАНДАРТНАЯ РАБОТА С КИСТЬЮ
        else if (!m_suppressPlacementUntilRelease) {
            if (inBounds) {
                bool isSinglePointBrush = (m_currentBrush == EditorBrush::Spawner || m_currentBrush == EditorBrush::Base ||
                                           m_currentBrush == EditorBrush::RailStart || m_currentBrush == EditorBrush::RailEnd);
                if (leftDown) {
                    if (!isSinglePointBrush || !m_isLeftMouseDown) {
                        applyBrush(gridPos.x, gridPos.y, m_currentBrush);
                    }
                } else if (rightDown) {
                    eraseCell(gridPos.x, gridPos.y);
                }
            }
        }
    }

    // Завершение прямоугольной заливки при отпускании ЛКМ
    if (!leftDown && m_isLeftMouseDown && m_isBoxFilling) {
        if (m_boxStartCell.x >= 0 && m_boxCurrentCell.x >= 0) {
            int minX = std::max(0, std::min(m_boxStartCell.x, m_boxCurrentCell.x));
            int maxX = std::min(m_gridWidth - 1, std::max(m_boxStartCell.x, m_boxCurrentCell.x));
            int minY = std::max(0, std::min(m_boxStartCell.y, m_boxCurrentCell.y));
            int maxY = std::min(m_gridHeight - 1, std::max(m_boxStartCell.y, m_boxCurrentCell.y));
            int count = 0;
            for (int gy = minY; gy <= maxY; ++gy) {
                for (int gx = minX; gx <= maxX; ++gx) {
                    applyBrush(gx, gy, m_currentBrush);
                    count++;
                }
            }
            recalculatePaths();
            m_isDirty = true;
            showToast("Заливка: " + std::to_string(count) + " ячеек", glm::vec4(0.3f, 0.85f, 0.4f, 1.0f));
        }
        m_isBoxFilling = false;
        m_boxStartCell = glm::ivec2(-1);
        m_boxCurrentCell = glm::ivec2(-1);
    }

    if (!leftDown && !rightDown) {
        m_suppressPlacementUntilRelease = false;
    }

    m_isLeftMouseDown = leftDown;
    m_isRightMouseDown = rightDown;
}

void MapEditorState::update(float dt) {
    m_toolbarUI.update(dt);

    if (m_grid) {
        m_pathVisualizer.update(dt, m_grid->getCellSize());
    }

    if (m_waveModal.isOpen()) {
        m_waveModal.update(dt);
    }

    if (m_mapsBrowserModal.isOpen()) {
        m_mapsBrowserModal.update(dt);
    }

    if (m_statusTimer > 0.0f) {
        m_statusTimer -= dt;
        if (m_statusTimer <= 0.0f) {
            m_statusMessage = "";
            m_statusColor = glm::vec3(0.9f, 0.9f, 0.9f);
        }
    }

    if (m_toastTimer > 0.0f) {
        m_toastTimer -= dt;
        if (m_toastTimer <= 0.0f) {
            m_toastTimer = 0.0f;
            m_toastMessage = "";
        }
    }
}

void MapEditorState::render() {
    m_renderer->beginBatch();

    // 1. Темный космический фон рабочей зоны
    m_renderer->drawSprite(m_whiteTexture, glm::vec2(0.0f), glm::vec2(m_width, m_height), 0.0f, glm::vec3(0.09f, 0.10f, 0.13f));

    // 2. Игровая сетка (отцентрирована строго между верхним и нижним барами)
    if (m_grid) {
        m_grid->draw(m_renderer.get(), m_whiteTexture, glm::vec3(1.0f));
    }

    // 3. Стрелочки путей в реальном времени с цветом, соответствующим целевой базе!
    if (m_showPathArrows && m_grid && m_arrowTexture) {
        for (size_t i = 0; i < m_activePaths.size(); ++i) {
            if (!m_activePaths[i].empty() && i < m_spawners.size()) {
                int targetId = m_spawners[i].targetBaseIndex;
                glm::vec3 arrowColor = getIdColor(targetId);
                m_pathVisualizer.renderPathArrows(m_renderer.get(), m_arrowTexture, m_activePaths[i], *m_grid, arrowColor);
            }
        }
    }

    // 4. Отрисовка маркеров спавнеров и баз
    if (m_grid) {
        float cellSize = m_grid->getCellSize();

        // Базы
        for (const auto& b : m_bases) {
            glm::vec2 pPos = m_grid->gridToPixel(b.x, b.y);
            glm::vec3 bCol = getIdColor(b.id);
            m_renderer->drawSprite(m_whiteTexture, pPos + glm::vec2(2.0f), glm::vec2(cellSize - 4.0f), 0.0f, bCol);
            m_renderer->drawSprite(m_whiteTexture, pPos + glm::vec2(5.0f), glm::vec2(cellSize - 10.0f), 0.0f, bCol * 0.85f);
        }

        // Спавнеры
        for (size_t i = 0; i < m_spawners.size(); ++i) {
            const auto& sp = m_spawners[i];
            glm::vec2 pPos = m_grid->gridToPixel(sp.pos.x, sp.pos.y);
            bool isBlocked = (i >= m_activePaths.size() || m_activePaths[i].empty());

            glm::vec3 spColor;
            if (isBlocked) {
                spColor = glm::vec3(0.95f, 0.15f, 0.15f);
            } else {
                spColor = getIdColor(sp.targetBaseIndex);
            }

            m_renderer->drawSprite(m_whiteTexture, pPos + glm::vec2(2.0f), glm::vec2(cellSize - 4.0f), 0.0f, spColor);
            m_renderer->drawSprite(m_whiteTexture, pPos + glm::vec2(5.0f), glm::vec2(cellSize - 10.0f), 0.0f, spColor * 0.85f);
        }

        // Маркеры вагонетки (Депо/Старт и Тупик/Конец)
        if (!m_minecarts.empty()) {
            const auto& mc = m_minecarts[0];
            if (mc.hasStart()) {
                glm::vec2 pPos = m_grid->gridToPixel(mc.start.x, mc.start.y);
                glm::vec3 startCol = glm::vec3(0.95f, 0.70f, 0.15f); // Золотисто-помаранчевий
                m_renderer->drawSprite(m_whiteTexture, pPos + glm::vec2(2.0f), glm::vec2(cellSize - 4.0f), 0.0f, startCol);
                m_renderer->drawSprite(m_whiteTexture, pPos + glm::vec2(5.0f), glm::vec2(cellSize - 10.0f), 0.0f, startCol * 0.85f);
            }
            if (mc.hasEnd()) {
                glm::vec2 pPos = m_grid->gridToPixel(mc.end.x, mc.end.y);
                glm::vec3 endCol = glm::vec3(0.90f, 0.20f, 0.20f); // Червоний буферний
                m_renderer->drawSprite(m_whiteTexture, pPos + glm::vec2(2.0f), glm::vec2(cellSize - 4.0f), 0.0f, endCol);
                m_renderer->drawSprite(m_whiteTexture, pPos + glm::vec2(5.0f), glm::vec2(cellSize - 10.0f), 0.0f, endCol * 0.85f);
            }
        }

        // Підсвічування BFS-маршруту рейок (якщо валідний)
        if (m_isRailPathValid && !m_railPath.empty()) {
            float dotSize = std::max(4.0f, cellSize * 0.28f);
            float dotOff  = (cellSize - dotSize) * 0.5f;
            glm::vec3 pathCol = glm::vec3(0.15f, 0.90f, 0.80f); // бірюзовий
            for (const auto& pt : m_railPath) {
                glm::vec2 pPos = m_grid->gridToPixel(pt.x, pt.y);
                m_renderer->drawSprite(m_whiteTexture,
                    pPos + glm::vec2(dotOff),
                    glm::vec2(dotSize), 0.0f, pathCol);
            }
        }
    }

    // 4.2. Рамка предпросмотра прямоугольной заливки (Box Fill)
    if (m_isBoxFilling && m_grid && m_boxStartCell.x >= 0 && m_boxCurrentCell.x >= 0) {
        int minX = std::max(0, std::min(m_boxStartCell.x, m_boxCurrentCell.x));
        int maxX = std::min(m_gridWidth - 1, std::max(m_boxStartCell.x, m_boxCurrentCell.x));
        int minY = std::max(0, std::min(m_boxStartCell.y, m_boxCurrentCell.y));
        int maxY = std::min(m_gridHeight - 1, std::max(m_boxStartCell.y, m_boxCurrentCell.y));

        float cellSize = m_grid->getCellSize();
        glm::vec2 p1 = m_grid->gridToPixel(minX, minY);
        glm::vec2 p2 = m_grid->gridToPixel(maxX, maxY) + glm::vec2(cellSize);
        glm::vec2 boxPos = p1;
        glm::vec2 boxSize = p2 - p1;

        // Полупрозрачная подсветка области
        m_renderer->drawSpriteRGBA(m_whiteTexture, boxPos, boxSize, 0.0f, glm::vec4(0.25f, 0.65f, 1.0f, 0.25f));

        // Контурная рамка
        glm::vec4 frameCol = glm::vec4(0.35f, 0.85f, 1.0f, 0.90f);
        float borderThick = 2.5f;
        m_renderer->drawSpriteRGBA(m_whiteTexture, boxPos, glm::vec2(boxSize.x, borderThick), 0.0f, frameCol);
        m_renderer->drawSpriteRGBA(m_whiteTexture, boxPos + glm::vec2(0.0f, boxSize.y - borderThick), glm::vec2(boxSize.x, borderThick), 0.0f, frameCol);
        m_renderer->drawSpriteRGBA(m_whiteTexture, boxPos, glm::vec2(borderThick, boxSize.y), 0.0f, frameCol);
        m_renderer->drawSpriteRGBA(m_whiteTexture, boxPos + glm::vec2(boxSize.x - borderThick, 0.0f), glm::vec2(borderThick, boxSize.y), 0.0f, frameCol);
    }

    // 5. ВЕРХНИЙ ХЕДЕР-БАР И НИЖНИЙ ТУЛБАР (EditorToolbarUI)
    m_toolbarUI.render(m_renderer.get(), m_textRenderer, m_whiteTexture, buildEditorContext(), m_mousePos);

    // 6. ТЕКСТ (Клетки спавнеров и баз)
    if (m_textRenderer) {
        // Текст на клетках спавнеров и баз
        if (m_grid) {
            float cellSize = m_grid->getCellSize();

            for (const auto& b : m_bases) {
                glm::vec2 pPos = m_grid->gridToPixel(b.x, b.y);
                std::string baseLabel = "B#" + std::to_string(b.id);
                m_textRenderer->RenderText(baseLabel, pPos.x + cellSize * 0.10f, pPos.y + cellSize * 0.25f, 0.70f, glm::vec3(0.05f, 0.08f, 0.15f));
            }

            for (size_t i = 0; i < m_spawners.size(); ++i) {
                const auto& sp = m_spawners[i];
                glm::vec2 pPos = m_grid->gridToPixel(sp.pos.x, sp.pos.y);
                bool isBlocked = (i >= m_activePaths.size() || m_activePaths[i].empty());

                bool hasTargetBase = (sp.targetBaseIndex == -1);
                if (!hasTargetBase) {
                    for (const auto& b : m_bases) {
                        if (b.id == sp.targetBaseIndex) { hasTargetBase = true; break; }
                    }
                }

                std::string spText;
                if (isBlocked) {
                    spText = "STOP";
                } else if (sp.targetBaseIndex == -1) {
                    spText = "S:Auto";
                } else if (!hasTargetBase) {
                    spText = "S>?" + std::to_string(sp.targetBaseIndex);
                } else {
                    spText = "S>#" + std::to_string(sp.targetBaseIndex);
                }
                m_textRenderer->RenderText(spText, pPos.x + cellSize * 0.06f, pPos.y + cellSize * 0.25f, 0.62f, glm::vec3(0.05f, 0.08f, 0.12f));
            }

            // Текст на маркерах вагонетки (Депо/Старт и Тупик/Конец)
            if (!m_minecarts.empty()) {
                const auto& mc = m_minecarts[0];
                if (mc.hasStart()) {
                    glm::vec2 pPos = m_grid->gridToPixel(mc.start.x, mc.start.y);
                    std::string label = "START";
                    float fScale = std::clamp(cellSize / 64.0f * 0.52f, 0.35f, 0.65f);
                    float tw = m_textRenderer->CalculateTextWidth(label, fScale);
                    float tx = pPos.x + (cellSize - tw) * 0.5f;
                    float ty = pPos.y + (cellSize - fScale * 28.0f) * 0.5f + 1.0f;
                    m_textRenderer->RenderText(label, tx, ty, fScale, glm::vec3(0.15f, 0.10f, 0.03f));
                }
                if (mc.hasEnd()) {
                    glm::vec2 pPos = m_grid->gridToPixel(mc.end.x, mc.end.y);
                    std::string label = "END";
                    float fScale = std::clamp(cellSize / 64.0f * 0.56f, 0.35f, 0.68f);
                    float tw = m_textRenderer->CalculateTextWidth(label, fScale);
                    float tx = pPos.x + (cellSize - tw) * 0.5f;
                    float ty = pPos.y + (cellSize - fScale * 28.0f) * 0.5f + 1.0f;
                    m_textRenderer->RenderText(label, tx, ty, fScale, glm::vec3(1.0f, 0.95f, 0.95f));
                }
            }
        }
    }

    m_renderer->endBatch();

    // 8. МОДАЛЬНОЕ ОКНО РЕДАКТОРА ВОЛН (если активно)
    if (m_waveModal.isOpen()) {
        m_renderer->beginBatch();
        m_waveModal.render(m_renderer.get(), m_textRenderer, m_whiteTexture,
                           m_width, m_height, getTopBarHeight(), getBottomDockHeight(),
                           m_mousePos, m_waves);
        m_renderer->endBatch();
    }

    // 9. МОДАЛЬНОЕ ОКНО СПИСКА КАРТ И ПЕРЕИМЕНОВАНИЯ (если активно)
    if (m_mapsBrowserModal.isOpen()) {
        m_renderer->beginBatch();
        m_mapsBrowserModal.render(m_renderer.get(), m_textRenderer, m_whiteTexture, m_width, m_height, m_mousePos);
        m_renderer->endBatch();
    }

    // 11. МОДАЛЬНОЕ ОКНО ПОДТВЕРЖДЕНИЯ ВЫХОДА (если активно)
    if (m_isExitModalOpen) {
        m_renderer->beginBatch();
        renderExitModal();
        m_renderer->endBatch();
    }

    // 12. МОДАЛЬНЕ ВІКНО НАЛАШТУВАНЬ ВАГОНЕТКИ (якщо активне)
    if (m_minecartModal.isOpen()) {
        m_renderer->beginBatch();
        m_minecartModal.render(m_renderer.get(), m_textRenderer, m_whiteTexture,
                              m_width, m_height, m_mousePos,
                              m_minecarts, m_isRailPathValid, m_railPath.size());
        m_renderer->endBatch();
    }

    // 13. Всплывающее тост-уведомление (Toast Feedback)
    if (m_toastTimer > 0.0f && !m_toastMessage.empty()) {
        m_renderer->beginBatch();

        float scale = GetUIScale(m_width, m_height);
        float fontScale = std::clamp(0.52f * scale, 0.38f, 0.65f);
        float textW = m_textRenderer ? m_textRenderer->CalculateTextWidth(m_toastMessage, fontScale) : 180.0f;

        float padX = 22.0f * scale;
        float toastW = textW + padX * 2.0f;
        float toastH = 38.0f * scale;
        float toastX = (static_cast<float>(m_width) - toastW) * 0.5f;

        // Плавная прозрачность: появление в начале (первые 0.25с) и затухание в конце (последние 0.4с)
        float alpha = 1.0f;
        if (m_toastTimer < 0.4f) {
            alpha = m_toastTimer / 0.4f;
        } else if (m_toastTimer > 2.55f) {
            alpha = (2.8f - m_toastTimer) / 0.25f;
        }
        alpha = std::clamp(alpha, 0.0f, 1.0f);

        // Плавный выезд сверху
        float targetY = getTopBarHeight() + 12.0f * scale;
        float toastY = targetY - (1.0f - alpha) * 8.0f;

        // Тень
        m_renderer->drawSpriteRGBA(m_whiteTexture,
            glm::vec2(toastX + 3.0f, toastY + 4.0f),
            glm::vec2(toastW, toastH),
            0.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.40f * alpha));

        // Темная подложка
        m_renderer->drawSpriteRGBA(m_whiteTexture,
            glm::vec2(toastX, toastY),
            glm::vec2(toastW, toastH),
            0.0f, glm::vec4(0.10f, 0.12f, 0.16f, 0.95f * alpha));

        // Рамка цвета тоста
        glm::vec4 borderCol = glm::vec4(glm::vec3(m_toastColor), m_toastColor.a * alpha);
        m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(toastX, toastY), glm::vec2(toastW, 1.5f), 0.0f, borderCol);
        m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(toastX, toastY + toastH - 1.5f), glm::vec2(toastW, 1.5f), 0.0f, borderCol);
        m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(toastX, toastY), glm::vec2(1.5f, toastH), 0.0f, borderCol);
        m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(toastX + toastW - 1.5f, toastY), glm::vec2(1.5f, toastH), 0.0f, borderCol);

        // Акцентная цветная полоса слева
        m_renderer->drawSpriteRGBA(m_whiteTexture,
            glm::vec2(toastX + 3.0f, toastY + 3.0f),
            glm::vec2(5.0f, toastH - 6.0f),
            0.0f, borderCol);

        m_renderer->flush();

        if (m_textRenderer) {
            float tx = toastX + padX;
            float ty = toastY + (toastH - fontScale * 28.0f) * 0.5f + 1.0f;
            glm::vec3 txtCol = glm::vec3(1.0f) * alpha;
            m_textRenderer->RenderText(m_toastMessage, tx, ty, fontScale, txtCol);
        }

        m_renderer->endBatch();
    }
}

void MapEditorState::resize(int width, int height) {
    m_width = width;
    m_height = height;
    if (m_grid) {
        m_grid->updateCellSize(m_width, m_height, getBottomDockHeight() + 6.0f, getTopBarHeight() + 6.0f);
    }
    updateButtonLayout();
}















void MapEditorState::openExitModal() {
    if (!m_isDirty) {
        std::cout << "[MapEditor] openExitModal: Map is not dirty (unmodified), exiting directly to origin." << std::endl;
        returnToOrigin();
        return;
    }
    std::cout << "[MapEditor] openExitModal: exit confirmation dialog opened" << std::endl;
    m_isExitModalOpen = true;
    m_suppressPlacementUntilRelease = true;
    m_suppressClickUntilRelease = true;
    m_exitModalEscReleased = false;
}

void MapEditorState::closeExitModal() {
    std::cout << "[MapEditor] closeExitModal: exit dialog closed (staying in editor)" << std::endl;
    m_isExitModalOpen = false;
    m_suppressPlacementUntilRelease = true;
    m_exitModalEscReleased = false;
    m_keyEscPressedLastFrame = true;
}

MapEditorState::ExitModalLayout MapEditorState::getExitModalLayout() const {
    ExitModalLayout layout;
    float scale = GetUIScale(m_width, m_height);

    layout.fTitle = std::clamp(0.68f * scale, 0.48f, 0.88f);
    layout.fQuestion = std::clamp(0.66f * scale, 0.46f, 0.84f);
    layout.fSub = std::clamp(0.48f * scale, 0.35f, 0.64f);
    layout.fBtn = std::clamp(0.46f * scale, 0.34f, 0.60f);
    layout.fCross = std::clamp(0.55f * scale, 0.40f, 0.75f);

    std::string saveExitStr = LOC("EDITOR_EXIT_SAVE_AND_EXIT");
    std::string discardStr = LOC("EDITOR_EXIT_DISCARD");
    std::string cancelStr = LOC("EDITOR_EXIT_CANCEL");

    float w1 = m_textRenderer ? m_textRenderer->CalculateTextWidth(saveExitStr, layout.fBtn) : 130.0f;
    float w2 = m_textRenderer ? m_textRenderer->CalculateTextWidth(discardStr, layout.fBtn) : 140.0f;
    float w3 = m_textRenderer ? m_textRenderer->CalculateTextWidth(cancelStr, layout.fBtn) : 60.0f;

    float btnPad = std::clamp(14.0f * scale, 8.0f, 20.0f);
    float req1 = w1 + btnPad * 2.0f;
    float req2 = w2 + btnPad * 2.0f;
    float req3 = w3 + btnPad * 2.0f;
    float totalReq = req1 + req2 + req3;

    float sideMargin = std::clamp(18.0f * scale, 12.0f, 26.0f);
    float btnGap = std::clamp(12.0f * scale, 8.0f, 16.0f);

    float desiredW = std::max(540.0f * scale, totalReq + 2.0f * sideMargin + 2.0f * btnGap);
    layout.modalSize.x = std::clamp(desiredW, 380.0f, static_cast<float>(m_width) - 40.0f);
    layout.modalSize.y = std::clamp(220.0f * scale, 170.0f, static_cast<float>(m_height) - 40.0f);

    layout.modalPos = glm::vec2((static_cast<float>(m_width) - layout.modalSize.x) * 0.5f,
                                (static_cast<float>(m_height) - layout.modalSize.y) * 0.5f);

    layout.headerH = std::clamp(40.0f * scale, 30.0f, 56.0f);

    float crossSize = std::clamp(26.0f * scale, 20.0f, 36.0f);
    layout.btnCloseCrossSize = glm::vec2(crossSize, crossSize);
    layout.btnCloseCrossPos = glm::vec2(layout.modalPos.x + layout.modalSize.x - crossSize - std::clamp(8.0f * scale, 6.0f, 12.0f),
                                        layout.modalPos.y + (layout.headerH - crossSize) * 0.5f);

    float btnH = std::clamp(42.0f * scale, 32.0f, 54.0f);
    float btnMarginBottom = std::clamp(18.0f * scale, 12.0f, 24.0f);
    float btnY = layout.modalPos.y + layout.modalSize.y - btnH - btnMarginBottom;

    float availW = layout.modalSize.x - 2.0f * sideMargin - 2.0f * btnGap;
    float btn1W, btn2W, btn3W;
    if (availW >= totalReq) {
        float extra = availW - totalReq;
        btn1W = req1 + extra * (req1 / totalReq);
        btn2W = req2 + extra * (req2 / totalReq);
        btn3W = req3 + extra * (req3 / totalReq);
    } else {
        float ratio = availW / std::max(1.0f, totalReq);
        btn1W = req1 * ratio;
        btn2W = req2 * ratio;
        btn3W = req3 * ratio;
    }

    layout.btnSaveExitPos = glm::vec2(layout.modalPos.x + sideMargin, btnY);
    layout.btnSaveExitSize = glm::vec2(btn1W, btnH);

    layout.btnDiscardPos = glm::vec2(layout.btnSaveExitPos.x + btn1W + btnGap, btnY);
    layout.btnDiscardSize = glm::vec2(btn2W, btnH);

    layout.btnCancelPos = glm::vec2(layout.btnDiscardPos.x + btn2W + btnGap, btnY);
    layout.btnCancelSize = glm::vec2(btn3W, btnH);

    float contentTop = layout.modalPos.y + layout.headerH;
    float contentH = btnY - contentTop;
    layout.questionY = contentTop + contentH * 0.28f;
    layout.subY = contentTop + contentH * 0.62f;

    return layout;
}

bool MapEditorState::processExitModalInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, float dt) {
    if (!m_isExitModalOpen) return false;

    // 1. Подавление клика мыши, которым открыли модалку
    if (m_suppressPlacementUntilRelease) {
        if (!leftDown) {
            m_suppressPlacementUntilRelease = false;
        }
    }

    // 2. Горячая клавиша Escape:
    // При открытии m_exitModalEscReleased = false.
    // Когда пользователь отпустит Escape (если открыл окно по Escape), взводится m_exitModalEscReleased = true.
    // При следующем нажатии Escape — подтверждаем выход без сохранения!
    bool keyEsc = (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS);
    if (!keyEsc) {
        m_exitModalEscReleased = true;
        m_keyEscPressedLastFrame = false;
    } else if (m_exitModalEscReleased) {
        std::cout << "[MapEditor] Exit Modal: ESC pressed -> exiting without saving" << std::endl;
        returnToOrigin();
        return true;
    }

    // 3. Горячая клавиша Enter — сохранить и выйти
    bool keyEnter = (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_KP_ENTER) == GLFW_PRESS);
    if (keyEnter) {
        std::cout << "[MapEditor] Exit Modal: Enter pressed -> saving and exiting" << std::endl;
        saveMap();
        returnToOrigin();
        return true;
    }

    ExitModalLayout l = getExitModalLayout();

    // 4. Клики мыши обрабатываются только после отпускания кнопки мыши после открытия
    if (!m_suppressPlacementUntilRelease && leftDown && !m_isLeftMouseDown) {
        // Кнопка [ Сохранить и выйти ]
        if (isPointInRect(mousePos, l.btnSaveExitPos, l.btnSaveExitSize)) {
            std::cout << "[MapEditor] Exit Modal: Clicked [Save and Exit] -> saving and exiting" << std::endl;
            saveMap();
            returnToOrigin();
            return true;
        }

        // Кнопка [ Выйти без сохранения ]
        if (isPointInRect(mousePos, l.btnDiscardPos, l.btnDiscardSize)) {
            std::cout << "[MapEditor] Exit Modal: Clicked [Discard and Exit] -> exiting without saving" << std::endl;
            returnToOrigin();
            return true;
        }

        // Кнопка [ Отмена ]
        if (isPointInRect(mousePos, l.btnCancelPos, l.btnCancelSize)) {
            std::cout << "[MapEditor] Exit Modal: Clicked [Cancel]" << std::endl;
            closeExitModal();
            return true;
        }

        // Крестик [X]
        if (isPointInRect(mousePos, l.btnCloseCrossPos, l.btnCloseCrossSize)) {
            std::cout << "[MapEditor] Exit Modal: Clicked [X] close cross" << std::endl;
            closeExitModal();
            return true;
        }

        // Клик вне модального окна -> закрываем (Отмена)
        if (!isPointInRect(mousePos, l.modalPos, l.modalSize)) {
            std::cout << "[MapEditor] Exit Modal: Clicked outside modal -> cancelling" << std::endl;
            closeExitModal();
            return true;
        }
    }

    return true;
}

void MapEditorState::renderExitModal() {
    // Полупрозрачный фон-затемнение
    m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(0.0f), glm::vec2(m_width, m_height), 0.0f, glm::vec4(0.04f, 0.05f, 0.07f, 0.78f));

    ExitModalLayout l = getExitModalLayout();

    // Основная подложка и 2px обводка
    m_renderer->drawSprite(m_whiteTexture, l.modalPos, l.modalSize, 0.0f, glm::vec3(0.12f, 0.13f, 0.18f));
    m_renderer->drawSprite(m_whiteTexture, l.modalPos, glm::vec2(l.modalSize.x, 2.0f), 0.0f, glm::vec3(0.40f, 0.75f, 1.0f));
    m_renderer->drawSprite(m_whiteTexture, l.modalPos + glm::vec2(0.0f, l.modalSize.y - 2.0f), glm::vec2(l.modalSize.x, 2.0f), 0.0f, glm::vec3(0.40f, 0.75f, 1.0f));
    m_renderer->drawSprite(m_whiteTexture, l.modalPos, glm::vec2(2.0f, l.modalSize.y), 0.0f, glm::vec3(0.40f, 0.75f, 1.0f));
    m_renderer->drawSprite(m_whiteTexture, l.modalPos + glm::vec2(l.modalSize.x - 2.0f, 0.0f), glm::vec2(2.0f, l.modalSize.y), 0.0f, glm::vec3(0.40f, 0.75f, 1.0f));

    // Шапка
    m_renderer->drawSprite(m_whiteTexture, l.modalPos, glm::vec2(l.modalSize.x, l.headerH), 0.0f, glm::vec3(0.16f, 0.18f, 0.25f));
    m_renderer->drawSprite(m_whiteTexture, l.modalPos + glm::vec2(0.0f, l.headerH), glm::vec2(l.modalSize.x, 2.0f), 0.0f, glm::vec3(0.40f, 0.75f, 1.0f));

    // Кнопка-крестик [X]
    bool hovCross = isPointInRect(m_mousePos, l.btnCloseCrossPos, l.btnCloseCrossSize);
    m_renderer->drawSprite(m_whiteTexture, l.btnCloseCrossPos, l.btnCloseCrossSize, 0.0f, hovCross ? glm::vec3(0.85f, 0.25f, 0.25f) : glm::vec3(0.35f, 0.38f, 0.45f));
    m_renderer->drawSprite(m_whiteTexture, l.btnCloseCrossPos + glm::vec2(1.0f), l.btnCloseCrossSize - glm::vec2(2.0f), 0.0f, hovCross ? glm::vec3(0.35f, 0.12f, 0.12f) : glm::vec3(0.20f, 0.22f, 0.28f));

    // Кнопки действий
    // 1. [ Сохранить и выйти ]
    bool hovSaveExit = isPointInRect(m_mousePos, l.btnSaveExitPos, l.btnSaveExitSize);
    m_renderer->drawSprite(m_whiteTexture, l.btnSaveExitPos, l.btnSaveExitSize, 0.0f, hovSaveExit ? glm::vec3(0.35f, 0.95f, 0.50f) : glm::vec3(0.25f, 0.75f, 0.38f));
    m_renderer->drawSprite(m_whiteTexture, l.btnSaveExitPos + glm::vec2(2.0f), l.btnSaveExitSize - glm::vec2(4.0f), 0.0f, hovSaveExit ? glm::vec3(0.18f, 0.42f, 0.24f) : glm::vec3(0.13f, 0.30f, 0.18f));

    // 2. [ Выйти без сохранения ]
    bool hovDiscard = isPointInRect(m_mousePos, l.btnDiscardPos, l.btnDiscardSize);
    m_renderer->drawSprite(m_whiteTexture, l.btnDiscardPos, l.btnDiscardSize, 0.0f, hovDiscard ? glm::vec3(0.95f, 0.35f, 0.35f) : glm::vec3(0.75f, 0.25f, 0.25f));
    m_renderer->drawSprite(m_whiteTexture, l.btnDiscardPos + glm::vec2(2.0f), l.btnDiscardSize - glm::vec2(4.0f), 0.0f, hovDiscard ? glm::vec3(0.40f, 0.16f, 0.16f) : glm::vec3(0.28f, 0.12f, 0.12f));

    // 3. [ Отмена ]
    bool hovCancel = isPointInRect(m_mousePos, l.btnCancelPos, l.btnCancelSize);
    m_renderer->drawSprite(m_whiteTexture, l.btnCancelPos, l.btnCancelSize, 0.0f, hovCancel ? glm::vec3(0.60f, 0.65f, 0.75f) : glm::vec3(0.40f, 0.44f, 0.52f));
    m_renderer->drawSprite(m_whiteTexture, l.btnCancelPos + glm::vec2(2.0f), l.btnCancelSize - glm::vec2(4.0f), 0.0f, hovCancel ? glm::vec3(0.25f, 0.28f, 0.34f) : glm::vec3(0.18f, 0.20f, 0.25f));

    m_renderer->flush();

    // Тексты в модальном окне
    if (m_textRenderer) {
        // Заголовок в шапке
        std::string titleStr = LOC("EDITOR_EXIT_TITLE");
        float tW = m_textRenderer->CalculateTextWidth(titleStr, l.fTitle);
        m_textRenderer->RenderText(titleStr, l.modalPos.x + (l.modalSize.x - tW) * 0.5f,
                                  l.modalPos.y + (l.headerH - l.fTitle * 28.0f) * 0.5f + 2.0f,
                                  l.fTitle, glm::vec3(1.0f, 0.85f, 0.25f));

        // Крестик [X]
        float crossW = m_textRenderer->CalculateTextWidth("x", l.fCross);
        m_textRenderer->RenderText("x", l.btnCloseCrossPos.x + (l.btnCloseCrossSize.x - crossW) * 0.5f,
                                  l.btnCloseCrossPos.y + (l.btnCloseCrossSize.y - l.fCross * 28.0f) * 0.5f + 2.0f,
                                  l.fCross, glm::vec3(0.9f));

        // Основной вопрос
        std::string questionStr = LOC("EDITOR_EXIT_QUESTION");
        float qW = m_textRenderer->CalculateTextWidth(questionStr, l.fQuestion);
        m_textRenderer->RenderText(questionStr, l.modalPos.x + (l.modalSize.x - qW) * 0.5f, l.questionY, l.fQuestion, glm::vec3(1.0f, 1.0f, 1.0f));

        // Поясняющий подтекст
        std::string subStr = LOC("EDITOR_EXIT_SUB");
        float sW = m_textRenderer->CalculateTextWidth(subStr, l.fSub);
        m_textRenderer->RenderText(subStr, l.modalPos.x + (l.modalSize.x - sW) * 0.5f, l.subY, l.fSub, glm::vec3(0.85f, 0.65f, 0.65f));

        // Текст кнопки 1: Сохранить и выйти
        std::string saveExitStr = LOC("EDITOR_EXIT_SAVE_AND_EXIT");
        float seW = m_textRenderer->CalculateTextWidth(saveExitStr, l.fBtn);
        float seX = l.btnSaveExitPos.x + (l.btnSaveExitSize.x - seW) * 0.5f;
        float seY = l.btnSaveExitPos.y + (l.btnSaveExitSize.y - l.fBtn * 28.0f) * 0.5f + 2.0f;
        m_textRenderer->RenderText(saveExitStr, seX, seY, l.fBtn, glm::vec3(0.95f, 1.0f, 0.95f));

        // Текст кнопки 2: Выйти без сохранения
        std::string discardStr = LOC("EDITOR_EXIT_DISCARD");
        float dW = m_textRenderer->CalculateTextWidth(discardStr, l.fBtn);
        float dX = l.btnDiscardPos.x + (l.btnDiscardSize.x - dW) * 0.5f;
        float dY = l.btnDiscardPos.y + (l.btnDiscardSize.y - l.fBtn * 28.0f) * 0.5f + 2.0f;
        m_textRenderer->RenderText(discardStr, dX, dY, l.fBtn, glm::vec3(1.0f, 0.90f, 0.90f));

        // Текст кнопки 3: Отмена
        std::string cancelStr = LOC("EDITOR_EXIT_CANCEL");
        float cW = m_textRenderer->CalculateTextWidth(cancelStr, l.fBtn);
        float cX = l.btnCancelPos.x + (l.btnCancelSize.x - cW) * 0.5f;
        float cY = l.btnCancelPos.y + (l.btnCancelSize.y - l.fBtn * 28.0f) * 0.5f + 2.0f;
        m_textRenderer->RenderText(cancelStr, cX, cY, l.fBtn, glm::vec3(0.90f, 0.92f, 0.96f));
    }
}

