#include "MapEditorState.h"
#include "GameStateManager.h"
#include "MainMenuState.h"
#include "LevelSelectState.h"
#include "GameplayState.h"
#include "../renderer/SpriteRenderer.h"
#include "../renderer/TextRenderer.h"
#include "../renderer/BackgroundRenderer.h"
#include "../renderer/DecorationRenderer.h"
#include "../resources/ResourceManager.h"
#include "../textures/Texture2D.h"
#include "../world/PathService.h"
#include "../ui/UICommon.h"
#include "../ui/BuildPanel.h"
#include <glm/gtc/matrix_transform.hpp>
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
    ResourceManager::loadTexture("particleTexture", "res/textures/particle.png");

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
    glm::mat4 P_screen = glm::ortho(0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, -1.0f, 1.0f);
    if (m_renderer) m_renderer->setProjection(P_screen);
    if (m_textRenderer) m_textRenderer->updateProjection(P_screen);
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
        m_emitters = data.emitters;
        m_envParticleManager.setEmitters(m_emitters);
        m_decorations = data.decorations;
        m_selectedDecorationIndex = -1;
        m_isDraggingDecoration = false;
        m_selectedEntityIndex = -1;
        m_entityInspector.resetPosition();
        m_isCampaign = data.isCampaign;
        m_background = data.background.empty() ? "default" : data.background;
        m_cameraSettings = data.camera;
        m_isCameraConfigMode = false;
        m_tags = data.tags;
        m_startingMoney = data.startingMoney;
        m_startingHealth = data.startingHealth;
        m_allowedTowers = data.allowedTowers;
        m_maxUpgradeTier = data.maxUpgradeTier;
        m_towerMaxTiers = data.towerMaxTiers;
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
        sp.groupId = 0;
        m_spawners.push_back(sp);
        m_rawLayout.assign(12, std::vector<int>(20, 0));
        m_minecarts.clear();
        m_waves.clear();
        m_emitters.clear();
        m_envParticleManager.clear();
        m_decorations.clear();
        m_selectedDecorationIndex = -1;
        m_isDraggingDecoration = false;
        m_selectedEntityIndex = -1;
        m_entityInspector.resetPosition();
        m_isCampaign = false;
        m_background = "default";
        m_cameraSettings = LevelMapData::CameraSettings();
        m_isCameraConfigMode = false;
        m_tags = { "Тест" };
        m_startingMoney = 50;
        m_startingHealth = 20;
        m_allowedTowers = { "Basic", "Mercury", "Piston" };
        m_maxUpgradeTier = 3;
        m_towerMaxTiers = { { "Basic", 3 }, { "Mercury", 3 }, { "Piston", 3 } };
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
    updateGridDimensions();

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

    glm::vec2 screenCenter = glm::vec2(static_cast<float>(m_width) * 0.5f, static_cast<float>(m_height) * 0.5f);
    if (m_cameraSettings.isCustom && m_cameraSettings.targetTile.x >= 0.0f && m_cameraSettings.targetTile.y >= 0.0f) {
        m_zoom = std::clamp(m_cameraSettings.zoom, 0.4f, 2.5f);
        m_cameraPan = screenCenter - getWorldCamCenter() * m_zoom;
    } else {
        m_zoom = 1.0f;
        glm::vec2 defCenter = getDefaultWorldCenter();
        m_cameraPan = screenCenter - defCenter * m_zoom;
    }

    updateButtonLayout();
}

void MapEditorState::loadInitialMap() {
    loadLevelByName(m_currentLevelFileName);
}

void MapEditorState::resizeMap(int newW, int newH, bool expandFromStart) {
    newW = std::clamp(newW, 1, 250);
    newH = std::clamp(newH, 1, 250);
    if (newW == m_gridWidth && newH == m_gridHeight) return;

    int oldW = m_gridWidth;
    int oldH = m_gridHeight;
    int offsetX = expandFromStart ? (newW - oldW) : 0;
    int offsetY = expandFromStart ? (newH - oldH) : 0;

    // Сохраняем существующие тайлы с учетом смещения
    std::vector<std::vector<int>> newLayout(newH, std::vector<int>(newW, 0));
    for (int y = 0; y < oldH; ++y) {
        int ny = y + offsetY;
        if (ny < 0 || ny >= newH) continue;
        for (int x = 0; x < oldW; ++x) {
            int nx = x + offsetX;
            if (nx < 0 || nx >= newW) continue;
            newLayout[ny][nx] = m_rawLayout[y][x];
        }
    }
    m_rawLayout = newLayout;
    m_gridWidth = newW;
    m_gridHeight = newH;

    // Смещаем спавнеры и удаляем вышедшие за границы новой сетки
    for (auto& s : m_spawners) {
        s.pos += glm::ivec2(offsetX, offsetY);
    }
    m_spawners.erase(
        std::remove_if(m_spawners.begin(), m_spawners.end(),
                       [newW, newH](const SpawnerData& s) {
                           return s.pos.x < 0 || s.pos.x >= newW || s.pos.y < 0 || s.pos.y >= newH;
                       }),
        m_spawners.end()
    );

    // Смещаем базы и удаляем вышедшие за границы новой сетки
    for (auto& b : m_bases) {
        b.x += offsetX;
        b.y += offsetY;
    }
    m_bases.erase(
        std::remove_if(m_bases.begin(), m_bases.end(),
                       [newW, newH](const BaseData& b) {
                           return b.x < 0 || b.x >= newW || b.y < 0 || b.y >= newH;
                       }),
        m_bases.end()
    );

    // Смещаем вагонетки
    for (auto& mc : m_minecarts) {
        if (mc.start.x >= 0 && mc.start.y >= 0) {
            mc.start += glm::ivec2(offsetX, offsetY);
            if (mc.start.x < 0 || mc.start.x >= newW || mc.start.y < 0 || mc.start.y >= newH) {
                mc.start = glm::ivec2(-1, -1);
            }
        }
        if (mc.end.x >= 0 && mc.end.y >= 0) {
            mc.end += glm::ivec2(offsetX, offsetY);
            if (mc.end.x < 0 || mc.end.x >= newW || mc.end.y < 0 || mc.end.y >= newH) {
                mc.end = glm::ivec2(-1, -1);
            }
        }
    }

    // Смещаем эмиттеры частиц
    for (auto& e : m_emitters) {
        e.tilePos += glm::vec2(static_cast<float>(offsetX), static_cast<float>(offsetY));
    }
    m_emitters.erase(
        std::remove_if(m_emitters.begin(), m_emitters.end(),
                       [newW, newH](const ParticleEmitterConfig& e) {
                           return e.tilePos.x < -20.0f || e.tilePos.x > (newW + 20.0f) ||
                                  e.tilePos.y < -20.0f || e.tilePos.y > (newH + 20.0f);
                       }),
        m_emitters.end()
    );
    m_envParticleManager.setEmitters(m_emitters);

    // Смещаем декорации
    for (auto& d : m_decorations) {
        d.tilePos += glm::vec2(static_cast<float>(offsetX), static_cast<float>(offsetY));
    }

    // Смещаем кастомную камеру при наличии или при активном режиме настройки камеры
    if ((m_cameraSettings.isCustom || m_isCameraConfigMode) &&
        m_cameraSettings.targetTile.x >= 0.0f && m_cameraSettings.targetTile.y >= 0.0f) {
        if (expandFromStart) {
            m_cameraSettings.targetTile += glm::vec2(static_cast<float>(offsetX), static_cast<float>(offsetY));
        }
    }

    m_grid = std::make_unique<Grid>(m_gridWidth, m_gridHeight, m_cellSize);
    updateGridDimensions();

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

    // Обновляем мировую позицию декораций
    for (auto& d : m_decorations) {
        d.worldPos = m_grid->getOffset() + d.tilePos * m_grid->getCellSize();
    }

    m_grid->saveOriginalGrid();
    m_pathfinder = std::make_unique<Pathfinder>(m_gridWidth, m_gridHeight);
    recalculatePaths();

    m_isDirty = true;
    m_statusMessage = "Grid resized to " + std::to_string(m_gridWidth) + "x" + std::to_string(m_gridHeight);
    m_statusColor = glm::vec3(0.3f, 0.9f, 1.0f);
    m_statusTimer = 2.5f;

    updateButtonLayout();
}

void MapEditorState::shiftMap(int dx, int dy) {
    if (dx == 0 && dy == 0) return;
    if (m_gridWidth <= 0 || m_gridHeight <= 0) return;

    // Сдвиг сетки тайлов
    std::vector<std::vector<int>> newLayout(m_gridHeight, std::vector<int>(m_gridWidth, 0));
    for (int y = 0; y < m_gridHeight; ++y) {
        int ny = y + dy;
        if (ny < 0 || ny >= m_gridHeight) continue;
        for (int x = 0; x < m_gridWidth; ++x) {
            int nx = x + dx;
            if (nx < 0 || nx >= m_gridWidth) continue;
            newLayout[ny][nx] = m_rawLayout[y][x];
        }
    }
    m_rawLayout = newLayout;

    // Сдвиг спавнеров
    for (auto& s : m_spawners) {
        s.pos += glm::ivec2(dx, dy);
    }
    m_spawners.erase(
        std::remove_if(m_spawners.begin(), m_spawners.end(),
                       [this](const SpawnerData& s) {
                           return s.pos.x < 0 || s.pos.x >= m_gridWidth || s.pos.y < 0 || s.pos.y >= m_gridHeight;
                       }),
        m_spawners.end()
    );

    // Сдвиг баз
    for (auto& b : m_bases) {
        b.x += dx;
        b.y += dy;
    }
    m_bases.erase(
        std::remove_if(m_bases.begin(), m_bases.end(),
                       [this](const BaseData& b) {
                           return b.x < 0 || b.x >= m_gridWidth || b.y < 0 || b.y >= m_gridHeight;
                       }),
        m_bases.end()
    );

    // Сдвиг вагонеток
    for (auto& mc : m_minecarts) {
        if (mc.start.x >= 0 && mc.start.y >= 0) {
            mc.start += glm::ivec2(dx, dy);
            if (mc.start.x < 0 || mc.start.x >= m_gridWidth || mc.start.y < 0 || mc.start.y >= m_gridHeight) {
                mc.start = glm::ivec2(-1, -1);
            }
        }
        if (mc.end.x >= 0 && mc.end.y >= 0) {
            mc.end += glm::ivec2(dx, dy);
            if (mc.end.x < 0 || mc.end.x >= m_gridWidth || mc.end.y < 0 || mc.end.y >= m_gridHeight) {
                mc.end = glm::ivec2(-1, -1);
            }
        }
    }

    // Сдвиг эмиттеров частиц
    for (auto& e : m_emitters) {
        e.tilePos += glm::vec2(static_cast<float>(dx), static_cast<float>(dy));
    }
    m_emitters.erase(
        std::remove_if(m_emitters.begin(), m_emitters.end(),
                       [this](const ParticleEmitterConfig& e) {
                           return e.tilePos.x < -20.0f || e.tilePos.x > (m_gridWidth + 20.0f) ||
                                  e.tilePos.y < -20.0f || e.tilePos.y > (m_gridHeight + 20.0f);
                       }),
        m_emitters.end()
    );
    m_envParticleManager.setEmitters(m_emitters);

    // Сдвиг декораций
    for (auto& d : m_decorations) {
        d.tilePos += glm::vec2(static_cast<float>(dx), static_cast<float>(dy));
        if (m_grid) {
            d.worldPos = m_grid->getOffset() + d.tilePos * m_grid->getCellSize();
        }
    }

    // Сдвиг кастомной камеры
    if (m_cameraSettings.isCustom && m_cameraSettings.targetTile.x >= 0.0f && m_cameraSettings.targetTile.y >= 0.0f) {
        m_cameraSettings.targetTile += glm::vec2(static_cast<float>(dx), static_cast<float>(dy));
    }

    // Перезаполняем ячейки Grid
    if (m_grid) {
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
    }

    recalculatePaths();
    m_isDirty = true;

    std::string dirStr = "";
    if (dx < 0) dirStr = "влево";
    else if (dx > 0) dirStr = "вправо";
    else if (dy < 0) dirStr = "вверх";
    else if (dy > 0) dirStr = "вниз";
    showToast("Карта сдвинута " + dirStr + " на 1 кл.", glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
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
    if (m_currentBrush == EditorBrush::Base) {
        if (m_selectedId < 0) m_selectedId = 0;
        if (step > 0) {
            m_selectedId = (m_selectedId >= 5) ? 0 : (m_selectedId + 1);
        } else if (step < 0) {
            m_selectedId = (m_selectedId <= 0) ? 5 : (m_selectedId - 1);
        }
    } else {
        if (step > 0) {
            m_selectedId = (m_selectedId >= 5) ? -1 : (m_selectedId + 1);
        } else if (step < 0) {
            m_selectedId = (m_selectedId <= -1) ? 5 : (m_selectedId - 1);
        }
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
    ctx.background = m_background;
    ctx.cameraIsCustom = m_cameraSettings.isCustom;
    ctx.cameraConfigMode = m_isCameraConfigMode;
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
    ctx.isHelpModalOpen = m_helpModal.isOpen();
    ctx.isLevelSettingsModalOpen = m_levelSettingsModal.isOpen();
    ctx.currentCategory = m_toolbarUI.getCategory();
    ctx.hudPreviewMode = m_hudPreviewMode;
    ctx.startingMoney = m_startingMoney;
    ctx.startingHealth = m_startingHealth;
    return ctx;
}

void MapEditorState::cycleHudPreviewMode() {
    switch (m_hudPreviewMode) {
        case HudPreviewMode::None:     m_hudPreviewMode = HudPreviewMode::Scale100; break;
        case HudPreviewMode::Scale100:  m_hudPreviewMode = HudPreviewMode::Scale125; break;
        case HudPreviewMode::Scale125:  m_hudPreviewMode = HudPreviewMode::Scale150; break;
        case HudPreviewMode::Scale150:  m_hudPreviewMode = HudPreviewMode::None;     break;
    }
    updateGridDimensions();
    updateButtonLayout();
    if (m_hudPreviewMode == HudPreviewMode::None) {
        showToast("HUD Preview: Выкл", glm::vec4(0.7f, 0.7f, 0.7f, 1.0f));
    } else if (m_hudPreviewMode == HudPreviewMode::Scale100) {
        showToast("HUD Preview: 100%", glm::vec4(1.0f, 0.85f, 0.25f, 1.0f));
    } else if (m_hudPreviewMode == HudPreviewMode::Scale125) {
        showToast("HUD Preview: 125%", glm::vec4(1.0f, 0.85f, 0.25f, 1.0f));
    } else if (m_hudPreviewMode == HudPreviewMode::Scale150) {
        showToast("HUD Preview: 150%", glm::vec4(1.0f, 0.85f, 0.25f, 1.0f));
    }
}

void MapEditorState::updateGridDimensions() {
    if (!m_grid) return;
    if (m_isCameraConfigMode || m_hudPreviewMode != HudPreviewMode::None) {
        // Синхронизация с боем: используем те же отступы сетки, что и в GameplayState
        float bottomMargin = Buildpanel::getBottomBarHeight(m_width, m_height);
        float topMargin = TimeControlUI::getTopMargin(m_width, m_height);
        m_grid->updateCellSize(m_width, m_height, bottomMargin, topMargin);
    } else {
        // Обычный режим редактора: центрирование между верхним хедером и нижним доком редактора
        m_grid->updateCellSize(m_width, m_height, getBottomDockHeight() + 6.0f, getTopBarHeight() + 6.0f);
    }

    // Актуализируем мировые позиции всех декораций из их tilePos
    for (auto& d : m_decorations) {
        d.worldPos = m_grid->getOffset() + d.tilePos * m_grid->getCellSize();
    }
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

    // Режим ластика:
    if (brush == EditorBrush::Eraser) {
        if (m_toolbarUI.getCategory() == PaletteCategory::Particles) {
            glm::vec2 worldPos = m_grid->getOffset() + glm::vec2(gridX + 0.5f, gridY + 0.5f) * m_grid->getCellSize();
            eraseEmitterNear(worldPos, m_grid->getCellSize() * 0.75f);
            return;
        }
        eraseCell(gridX, gridY);
        return;
    }

    // Режим эмиттеров частиц (Свищ пара, Капель, Искры, Дым, Туман)
    if (brush == EditorBrush::EmitterSteamJet || brush == EditorBrush::EmitterWaterDrip ||
        brush == EditorBrush::EmitterSparks || brush == EditorBrush::EmitterSmoke ||
        brush == EditorBrush::EmitterFog) {
        std::string eType = "steam_jet";
        std::string eName = "Свищ пара";
        float pMin = 6.0f, pMax = 14.0f, bDur = 2.5f;

        if (brush == EditorBrush::EmitterWaterDrip) {
            eType = "water_drip";
            eName = "Капель";
            pMin = 1.0f; pMax = 3.0f; bDur = 0.5f;
        } else if (brush == EditorBrush::EmitterSparks) {
            eType = "sparks";
            eName = "Искры";
            pMin = 4.0f; pMax = 10.0f; bDur = 0.8f;
        } else if (brush == EditorBrush::EmitterSmoke) {
            eType = "smoke";
            eName = "Дым";
            pMin = 3.0f; pMax = 8.0f; bDur = 3.0f;
        } else if (brush == EditorBrush::EmitterFog) {
            eType = "fog";
            eName = "Туман";
            pMin = 0.5f; pMax = 1.0f; bDur = 10.0f;
        }

        glm::vec2 targetCenter = glm::vec2(gridX + 0.5f, gridY + 0.5f);
        bool found = false;
        for (auto& em : m_emitters) {
            if (glm::distance(em.tilePos, targetCenter) < 0.6f) {
                em.type = eType;
                em.periodMin = pMin;
                em.periodMax = pMax;
                em.burstDuration = bDur;
                m_envParticleManager.setEmitters(m_emitters);
                found = true;
                break;
            }
        }

        if (!found) {
            ParticleEmitterConfig cfg;
            cfg.id = static_cast<int>(m_emitters.size()) + 1;
            cfg.type = eType;
            cfg.tilePos = targetCenter;
            cfg.angleDeg = m_defaultEmitterAngle;
            cfg.periodMin = pMin;
            cfg.periodMax = pMax;
            cfg.burstDuration = bDur;
            m_emitters.push_back(cfg);
            m_envParticleManager.setEmitters(m_emitters);
        }

        m_statusMessage = "Эмиттер [" + eName + "] установлен на (" + std::to_string(gridX) + ", " + std::to_string(gridY) + ")";
        m_statusColor = glm::vec3(0.5f, 0.85f, 1.0f);
        m_statusTimer = 2.0f;
        showToast("Эмиттер: " + eName, glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
        m_isDirty = true;
        return;
    }

    // Особый случай: клик по УЖЕ существующему спавнеру
    if (brush == EditorBrush::Spawner) {
        for (size_t i = 0; i < m_spawners.size(); ++i) {
            auto& sp = m_spawners[i];
            if (sp.pos.x == gridX && sp.pos.y == gridY) {
                m_selectedEntityType = EditorEntityType::Spawner;
                m_selectedEntityIndex = static_cast<int>(i);
                m_selectedDecorationIndex = -1;
                m_selectedEmitterIndex = -1;
                std::string idStr = (sp.targetBaseIndex == -1) ? "Auto" : ("#" + std::to_string(sp.targetBaseIndex));
                m_statusMessage = "Spawner at (" + std::to_string(gridX) + "," + std::to_string(gridY) + ") selected, target Base ID: " + idStr;
                m_statusColor = getIdColor(sp.targetBaseIndex);
                m_statusTimer = 2.5f;
                return;
            }
        }
    }

    // Особый случай: клик по УЖЕ существующей базе
    if (brush == EditorBrush::Base) {
        for (size_t i = 0; i < m_bases.size(); ++i) {
            auto& b = m_bases[i];
            if (b.x == gridX && b.y == gridY) {
                m_selectedEntityType = EditorEntityType::Base;
                m_selectedEntityIndex = static_cast<int>(i);
                m_selectedDecorationIndex = -1;
                m_selectedEmitterIndex = -1;
                m_statusMessage = "Base at (" + std::to_string(gridX) + "," + std::to_string(gridY) + ") selected, ID: #" + std::to_string(b.id);
                m_statusColor = getIdColor(b.id);
                m_statusTimer = 2.5f;
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
            m_spawners.push_back(SpawnerData(glm::ivec2(gridX, gridY), m_selectedId, m_selectedId, false, true));
            m_selectedEntityType = EditorEntityType::Spawner;
            m_selectedEntityIndex = static_cast<int>(m_spawners.size()) - 1;
            m_selectedDecorationIndex = -1;
            m_selectedEmitterIndex = -1;
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
                m_bases.push_back(BaseData(gridX, gridY, baseId, false, true));
                m_selectedEntityType = EditorEntityType::Base;
                m_selectedEntityIndex = static_cast<int>(m_bases.size()) - 1;
                m_selectedDecorationIndex = -1;
                m_selectedEmitterIndex = -1;
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

    if (m_selectedEntityIndex >= 0) {
        if (m_selectedEntityType == EditorEntityType::Spawner && m_selectedEntityIndex < static_cast<int>(m_spawners.size())) {
            if (m_spawners[m_selectedEntityIndex].pos.x == gridX && m_spawners[m_selectedEntityIndex].pos.y == gridY) {
                m_selectedEntityIndex = -1;
            }
        } else if (m_selectedEntityType == EditorEntityType::Base && m_selectedEntityIndex < static_cast<int>(m_bases.size())) {
            if (m_bases[m_selectedEntityIndex].x == gridX && m_bases[m_selectedEntityIndex].y == gridY) {
                m_selectedEntityIndex = -1;
            }
        }
    }

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

    m_emitters.erase(
        std::remove_if(m_emitters.begin(), m_emitters.end(),
                       [gridX, gridY](const ParticleEmitterConfig& e) {
                           return static_cast<int>(std::round(e.tilePos.x)) == gridX &&
                                  static_cast<int>(std::round(e.tilePos.y)) == gridY;
                       }),
        m_emitters.end()
    );
    m_envParticleManager.removeEmitterAt(gridX, gridY);

    m_rawLayout[gridY][gridX] = 0;
    m_grid->setCellType(gridX, gridY, CellType::Ground);
    m_grid->saveOriginalGrid();
    recalculatePaths();
    m_isDirty = true;
}

int MapEditorState::findNearestEmitterIndex(glm::vec2 worldPos, float maxDist) const {
    if (!m_grid || m_emitters.empty()) return -1;
    float cellSize = m_grid->getCellSize();
    int bestIdx = -1;
    float bestDistSq = maxDist * maxDist;

    for (size_t i = 0; i < m_emitters.size(); ++i) {
        glm::vec2 emWorld = m_grid->getOffset() + m_emitters[i].tilePos * cellSize;
        float dSq = glm::dot(worldPos - emWorld, worldPos - emWorld);
        if (dSq < bestDistSq) {
            bestDistSq = dSq;
            bestIdx = static_cast<int>(i);
        }
    }
    return bestIdx;
}

void MapEditorState::eraseEmitterNear(glm::vec2 worldPos, float maxDist) {
    int idx = findNearestEmitterIndex(worldPos, maxDist);
    if (idx >= 0 && idx < static_cast<int>(m_emitters.size())) {
        std::string eName = m_emitters[idx].type;
        m_emitters.erase(m_emitters.begin() + idx);
        m_envParticleManager.setEmitters(m_emitters);
        if (m_selectedEmitterIndex == idx) {
            m_selectedEmitterIndex = -1;
            m_isDraggingEmitter = false;
        } else if (m_selectedEmitterIndex > idx) {
            m_selectedEmitterIndex--;
        }
        m_isDirty = true;
        showToast("Эмиттер удален: " + eName, glm::vec4(1.0f, 0.4f, 0.4f, 1.0f));
    }
}

int MapEditorState::findNearestDecorationIndex(glm::vec2 worldPos, float maxDist) const {
    if (m_decorations.empty()) return -1;
    int bestIdx = -1;
    float bestDistSq = maxDist * maxDist;

    for (size_t i = 0; i < m_decorations.size(); ++i) {
        glm::vec2 decWorld = m_grid ? (m_grid->getOffset() + m_decorations[i].tilePos * m_grid->getCellSize()) : m_decorations[i].worldPos;
        glm::vec2 diff = worldPos - decWorld;
        float dSq = glm::dot(diff, diff);
        if (dSq < bestDistSq) {
            bestDistSq = dSq;
            bestIdx = static_cast<int>(i);
        }
    }
    return bestIdx;
}

void MapEditorState::eraseDecorationNear(glm::vec2 worldPos, float maxDist) {
    int idx = findNearestDecorationIndex(worldPos, maxDist);
    if (idx >= 0 && idx < static_cast<int>(m_decorations.size())) {
        std::string dName = m_decorations[idx].type;
        m_decorations.erase(m_decorations.begin() + idx);
        if (m_selectedDecorationIndex == idx) {
            m_selectedDecorationIndex = -1;
            m_isDraggingDecoration = false;
        } else if (m_selectedDecorationIndex > idx) {
            m_selectedDecorationIndex--;
        }
        m_isDirty = true;
        showToast("Декор удален: " + dName, glm::vec4(1.0f, 0.4f, 0.4f, 1.0f));
    }
}

void MapEditorState::saveMap() {
    LevelMapData data;
    data.name = m_currentLevelDisplayName;
    data.isCampaign = m_isCampaign;
    data.background = m_background;
    data.camera = m_cameraSettings;
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
    data.emitters = m_emitters;
    if (m_grid) {
        for (auto& dec : m_decorations) {
            dec.worldPos = m_grid->getOffset() + dec.tilePos * m_grid->getCellSize();
        }
    }
    data.decorations = m_decorations;
    data.startingMoney = m_startingMoney;
    data.startingHealth = m_startingHealth;
    data.allowedTowers = m_allowedTowers;
    data.maxUpgradeTier = m_maxUpgradeTier;
    data.towerMaxTiers = m_towerMaxTiers;

    bool ok = LevelManager::saveLevel(m_currentLevelFileName, data);
    std::cout << "[MapEditor] saveMap: file=" << m_currentLevelFileName << ", name=" << m_currentLevelDisplayName << ", size=" << m_gridWidth << "x" << m_gridHeight << ", money=" << m_startingMoney << ", hp=" << m_startingHealth << ", result=" << (ok ? "SUCCESS" : "FAIL") << std::endl;

    if (ok) {
        m_isDirty = false;
        m_statusMessage = "Map saved: " + m_currentLevelDisplayName + " (" + std::to_string(m_gridWidth) + "x" + std::to_string(m_gridHeight) + ") [$" + std::to_string(m_startingMoney) + " | " + std::to_string(m_startingHealth) + " HP]!";
        m_statusColor = glm::vec3(0.2f, 1.0f, 0.3f);
        showToast("Карта сохранена: $" + std::to_string(m_startingMoney) + " | " + std::to_string(m_startingHealth) + " HP", glm::vec4(0.2f, 0.8f, 0.3f, 1.0f));
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
    m_emitters.clear();
    m_envParticleManager.clear();
    m_decorations.clear();
    m_startingMoney = 50;
    m_startingHealth = 20;
    m_allowedTowers = { "Basic", "Mercury", "Piston" };
    m_maxUpgradeTier = 3;
    m_towerMaxTiers = { { "Basic", 3 }, { "Mercury", 3 }, { "Piston", 3 } };
    m_selectedDecorationIndex = -1;
    m_isDraggingDecoration = false;
    m_selectedEntityIndex = -1;
    m_entityInspector.resetPosition();
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
    if (m_exitModal.isOpen()) {
        int key = 0;
        bool keyReleased = false;
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_RELEASE) {
            key = GLFW_KEY_ESCAPE;
            keyReleased = true;
        } else if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            key = GLFW_KEY_ESCAPE;
            keyReleased = false;
        } else if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_KP_ENTER) == GLFW_PRESS) {
            key = GLFW_KEY_ENTER;
            keyReleased = false;
        }

        bool mouseClicked = leftDown && !m_isLeftMouseDown;
        ExitModalAction action = m_exitModal.handleInput(mousePos, mouseClicked, key, keyReleased);
        m_isLeftMouseDown = leftDown;
        m_isRightMouseDown = rightDown;

        if (action == ExitModalAction::SaveAndExit) {
            saveMap();
            returnToOrigin();
            return;
        } else if (action == ExitModalAction::DiscardAndExit) {
            returnToOrigin();
            return;
        } else if (action == ExitModalAction::Cancel) {
            return;
        }
        return;
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

    // Если открыто модальное окно настроек уровня и экономики
    if (m_levelSettingsModal.isOpen()) {
        bool keyEscCheck = (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS);
        bool handled = m_levelSettingsModal.handleInput(window, mousePos, leftDown, !leftDown && m_isLeftMouseDown,
                                                       keyEscCheck && !m_keyEscPressedLastFrame, dt,
                                                       m_width, m_height,
                                                       m_startingMoney, m_startingHealth,
                                                       m_allowedTowers, m_towerMaxTiers,
                                                       m_maxUpgradeTier, m_isDirty);
        if (keyEscCheck && !m_keyEscPressedLastFrame) {
            m_keyEscPressedLastFrame = keyEscCheck;
        }
        m_isLeftMouseDown = leftDown;
        m_isRightMouseDown = rightDown;
        m_suppressPlacementUntilRelease = true;
        if (handled) return;
    }

    // Если открыто модальное окно справки по управлению
    if (m_helpModal.isOpen()) {
        bool keyEscCheck = (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS);
        bool handled = m_helpModal.handleInput(window, mousePos, leftDown, !leftDown && m_isLeftMouseDown, keyEscCheck && !m_keyEscPressedLastFrame);
        if (keyEscCheck && !m_keyEscPressedLastFrame) {
            m_keyEscPressedLastFrame = keyEscCheck;
        }
        m_isLeftMouseDown = leftDown;
        m_isRightMouseDown = rightDown;
        m_suppressPlacementUntilRelease = true;
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

    // Горячая клавиша Справки (H или F1)
    bool keyH = (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS);
    bool keyF1 = (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS);
    if ((keyH && !m_keyHPressedLastFrame) || (keyF1 && !m_keyF1PressedLastFrame)) {
        m_helpModal.toggle();
        m_suppressPlacementUntilRelease = true;
    }
    m_keyHPressedLastFrame = keyH;
    m_keyF1PressedLastFrame = keyF1;

    // Горячая клавиша Escape
    bool keyEsc = (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS);
    if (keyEsc && !m_keyEscPressedLastFrame) {
        if (m_helpModal.isOpen()) {
            m_helpModal.close();
            m_keyEscPressedLastFrame = keyEsc;
            return;
        }
        if (m_selectedDecorationIndex >= 0) {
            m_selectedDecorationIndex = -1;
            m_isDraggingDecoration = false;
            m_keyEscPressedLastFrame = keyEsc;
            return;
        }
        if (m_selectedEmitterIndex >= 0) {
            m_selectedEmitterIndex = -1;
            m_isDraggingEmitter = false;
            m_keyEscPressedLastFrame = keyEsc;
            return;
        }
        if (m_selectedEntityIndex >= 0) {
            m_selectedEntityIndex = -1;
            m_keyEscPressedLastFrame = keyEsc;
            return;
        }
        if (m_currentBrush != EditorBrush::None) {
            m_currentBrush = EditorBrush::None;
            m_isDraggingDecoration = false;
            m_isDraggingEmitter = false;
            showToast("Кисть сброшена (нейтральный курсор)");
            m_keyEscPressedLastFrame = keyEsc;
            return;
        }
        if (m_exitModal.isOpen()) {
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
        if (m_levelSettingsModal.isOpen()) {
            m_levelSettingsModal.close();
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
        } else if (m_toolbarUI.getCategory() == PaletteCategory::SpecialObjects) {
            m_toolbarUI.setCategory(PaletteCategory::Particles);
            showToast("Палитра: Партиклы");
        } else if (m_toolbarUI.getCategory() == PaletteCategory::Particles) {
            m_toolbarUI.setCategory(PaletteCategory::Decorations);
            showToast("Палитра: Декор");
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
        } else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
            if (m_toolbarUI.getCategory() != PaletteCategory::Particles) {
                m_toolbarUI.setCategory(PaletteCategory::Particles);
                updateButtonLayout();
                showToast("Палитра: Партиклы");
            }
        } else if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {
            if (m_toolbarUI.getCategory() != PaletteCategory::Decorations) {
                m_toolbarUI.setCategory(PaletteCategory::Decorations);
                updateButtonLayout();
                showToast("Палитра: Декор");
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
        bool key9 = (glfwGetKey(window, GLFW_KEY_9) == GLFW_PRESS);
        bool key0 = (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS);

        if (key0) {
            m_currentBrush = EditorBrush::Eraser;
        } else if (m_toolbarUI.getCategory() == PaletteCategory::Tiles) {
            if (key1) m_currentBrush = EditorBrush::Ground;
            if (key2) m_currentBrush = EditorBrush::Wall;
            if (key3) m_currentBrush = EditorBrush::Platform;
            if (key4) m_currentBrush = EditorBrush::Path;
            if (key5) m_currentBrush = EditorBrush::Chasm;
        } else if (m_toolbarUI.getCategory() == PaletteCategory::SpecialObjects) {
            if (key1) m_currentBrush = EditorBrush::Spawner;
            if (key2) {
                m_currentBrush = EditorBrush::Base;
                if (m_selectedId < 0) {
                    m_selectedId = 0;
                    updateButtonLayout();
                }
            }
            if (key3) m_currentBrush = EditorBrush::Rail;
            if (key4) m_currentBrush = EditorBrush::RailStart;
            if (key5) m_currentBrush = EditorBrush::RailEnd;
        } else if (m_toolbarUI.getCategory() == PaletteCategory::Particles) {
            if (key1) m_currentBrush = EditorBrush::EmitterSteamJet;
            if (key2) m_currentBrush = EditorBrush::EmitterWaterDrip;
            if (key3) m_currentBrush = EditorBrush::EmitterSparks;
            if (key4) m_currentBrush = EditorBrush::EmitterSmoke;
            if (key5) m_currentBrush = EditorBrush::EmitterFog;
        } else if (m_toolbarUI.getCategory() == PaletteCategory::Decorations) {
            if (key1) m_currentBrush = EditorBrush::DecorBush;
            if (key2) m_currentBrush = EditorBrush::DecorGrass;
            if (key3) m_currentBrush = EditorBrush::DecorGrassField;
            if (key4) m_currentBrush = EditorBrush::DecorFlower;
            if (key5) m_currentBrush = EditorBrush::DecorStone;
            if (key6) m_currentBrush = EditorBrush::DecorHelmet;
            if (key7) m_currentBrush = EditorBrush::DecorPickaxe;
            if (key8) m_currentBrush = EditorBrush::DecorPuddle;
            if (key9) m_currentBrush = EditorBrush::DecorCrack;
            if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) m_currentBrush = EditorBrush::DecorFog;
        }

        // Прямые клавиши 6, 7, 8 с переключением категории при необходимости (если не на вкладке Декор)
        if (m_toolbarUI.getCategory() != PaletteCategory::Decorations) {
            if (key6) {
                m_currentBrush = EditorBrush::Base;
                if (m_selectedId < 0) {
                    m_selectedId = 0;
                }
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

    // Горячая клавиша Камеры (C)
    bool keyC = (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS);
    if (keyC && !m_keyCPressedLastFrame) {
        m_isCameraConfigMode = !m_isCameraConfigMode;
        if (m_isCameraConfigMode) {
            if (!m_cameraSettings.isCustom) {
                glm::vec2 worldCenter = screenToWorld(glm::vec2(static_cast<float>(m_width) * 0.5f, static_cast<float>(m_height) * 0.5f));
                m_cameraSettings.targetTile = worldPixelToTile(worldCenter);
                m_cameraSettings.zoom = m_zoom;
            }
            showToast("Режим камеры: сдвиньте/зуммируйте и нажмите [Зафиксировать камеру]", glm::vec4(0.35f, 0.85f, 1.0f, 1.0f));
        } else {
            showToast("Режим камеры выключен", glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));
        }
        updateGridDimensions();
        updateButtonLayout();
    }
    m_keyCPressedLastFrame = keyC;

    // Горячая клавиша предпросмотра боевого HUD (U)
    bool keyU = (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS);
    if (keyU && !m_keyUPressedLastFrame) {
        cycleHudPreviewMode();
    }
    m_keyUPressedLastFrame = keyU;

    // Горячая клавиша поворота направления струи эмиттера или декора (R)
    bool keyR = (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS);
    if (keyR && !m_keyRPressedLastFrame) {
        glm::vec2 mousePosLocal = glm::vec2(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
        glm::vec2 worldPos = screenToWorld(mousePosLocal);
        if (m_toolbarUI.getCategory() == PaletteCategory::Decorations) {
            int targetIdx = m_selectedDecorationIndex;
            if (targetIdx < 0 || targetIdx >= static_cast<int>(m_decorations.size())) {
                targetIdx = findNearestDecorationIndex(worldPos, 28.0f);
            }

            if (targetIdx >= 0 && targetIdx < static_cast<int>(m_decorations.size())) {
                m_decorations[targetIdx].rotation = std::fmod(m_decorations[targetIdx].rotation + 45.0f, 360.0f);
                if (m_decorations[targetIdx].rotation < 0.0f) m_decorations[targetIdx].rotation += 360.0f;
                m_selectedDecorationIndex = targetIdx;
                showToast("Угол декора: " + std::to_string(static_cast<int>(m_decorations[targetIdx].rotation)) + "°", glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
                m_isDirty = true;
            } else {
                m_defaultDecorationRotation = std::fmod(m_defaultDecorationRotation + 45.0f, 360.0f);
                if (m_defaultDecorationRotation < 0.0f) m_defaultDecorationRotation += 360.0f;
                showToast("Угол по умолчанию: " + std::to_string(static_cast<int>(m_defaultDecorationRotation)) + "°", glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
            }
        } else {
            int targetIdx = m_selectedEmitterIndex;
            if (targetIdx < 0 || targetIdx >= static_cast<int>(m_emitters.size())) {
                targetIdx = findNearestEmitterIndex(worldPos, 28.0f);
            }

            if (targetIdx >= 0 && targetIdx < static_cast<int>(m_emitters.size())) {
                m_emitters[targetIdx].angleDeg = std::fmod(m_emitters[targetIdx].angleDeg + 45.0f, 360.0f);
                if (m_emitters[targetIdx].angleDeg < 0.0f) m_emitters[targetIdx].angleDeg += 360.0f;
                m_envParticleManager.setEmitters(m_emitters);
                m_selectedEmitterIndex = targetIdx;
                showToast("Угол струи: " + std::to_string(static_cast<int>(m_emitters[targetIdx].angleDeg)) + "°", glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
                m_isDirty = true;
            } else {
                m_defaultEmitterAngle = std::fmod(m_defaultEmitterAngle + 45.0f, 360.0f);
                if (m_defaultEmitterAngle < 0.0f) m_defaultEmitterAngle += 360.0f;
                showToast("Угол струи по умолчанию: " + std::to_string(static_cast<int>(m_defaultEmitterAngle)) + "°", glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
            }
        }
    }
    m_keyRPressedLastFrame = keyR;

    // Стрелочки клавиатуры для ювелирного сдвига выделенного декора (1px / 8px со Shift)
    if (!isCtrl && m_toolbarUI.getCategory() == PaletteCategory::Decorations &&
        m_selectedDecorationIndex >= 0 && m_selectedDecorationIndex < static_cast<int>(m_decorations.size())) {
        float stepPx = isShift ? 8.0f : 1.0f;
        float cellSize = (m_grid && m_grid->getCellSize() > 0.001f) ? m_grid->getCellSize() : 64.0f;
        float stepTile = stepPx / cellSize;

        bool arrowMoved = false;
        int arrowKeys[4] = { GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_UP, GLFW_KEY_DOWN };
        glm::vec2 arrowDeltas[4] = {
            glm::vec2(-stepTile, 0.0f),
            glm::vec2(+stepTile, 0.0f),
            glm::vec2(0.0f, -stepTile),
            glm::vec2(0.0f, +stepTile)
        };

        for (int k = 0; k < 4; ++k) {
            int key = arrowKeys[k];
            bool isDown = (glfwGetKey(window, key) == GLFW_PRESS);
            auto& state = m_keyStates[key];
            bool trigger = false;
            if (isDown) {
                if (!state.isDown) {
                    trigger = true;
                    state.holdTimer = 0.0f;
                    state.repeatTimer = 0.0f;
                } else {
                    state.holdTimer += dt;
                    if (state.holdTimer >= 0.20f) {
                        state.repeatTimer += dt;
                        if (state.repeatTimer >= 0.03f) {
                            trigger = true;
                            state.repeatTimer = 0.0f;
                        }
                    }
                }
            }
            state.isDown = isDown;

            if (trigger) {
                m_decorations[m_selectedDecorationIndex].tilePos += arrowDeltas[k];
                if (m_grid) {
                    m_decorations[m_selectedDecorationIndex].worldPos = m_grid->getOffset() + m_decorations[m_selectedDecorationIndex].tilePos * m_grid->getCellSize();
                }
                arrowMoved = true;
            }
        }

        if (arrowMoved) {
            m_isDirty = true;
        }

        // Клавиша Delete / Backspace для удаления выделенного декора
        bool keyDel = (glfwGetKey(window, GLFW_KEY_DELETE) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS);
        if (keyDel) {
            std::string dName = m_decorations[m_selectedDecorationIndex].type;
            m_decorations.erase(m_decorations.begin() + m_selectedDecorationIndex);
            m_selectedDecorationIndex = -1;
            m_isDraggingDecoration = false;
            m_isDirty = true;
            showToast("Декор удален: " + dName, glm::vec4(1.0f, 0.4f, 0.4f, 1.0f));
        }
    }

    // Стрелочки клавиатуры для ювелирного сдвига выделенного эмиттера (1px / 8px со Shift)
    if (!isCtrl && m_toolbarUI.getCategory() == PaletteCategory::Particles &&
        m_selectedEmitterIndex >= 0 && m_selectedEmitterIndex < static_cast<int>(m_emitters.size())) {
        float stepPx = isShift ? 8.0f : 1.0f;
        float stepTile = stepPx / m_grid->getCellSize();

        bool arrowMoved = false;
        int arrowKeys[4] = { GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_UP, GLFW_KEY_DOWN };
        glm::vec2 arrowDeltas[4] = {
            glm::vec2(-stepTile, 0.0f),
            glm::vec2(+stepTile, 0.0f),
            glm::vec2(0.0f, -stepTile),
            glm::vec2(0.0f, +stepTile)
        };

        for (int k = 0; k < 4; ++k) {
            int key = arrowKeys[k];
            bool isDown = (glfwGetKey(window, key) == GLFW_PRESS);
            auto& state = m_keyStates[key];
            bool trigger = false;
            if (isDown) {
                if (!state.isDown) {
                    trigger = true;
                    state.holdTimer = 0.0f;
                    state.repeatTimer = 0.0f;
                } else {
                    state.holdTimer += dt;
                    if (state.holdTimer >= 0.20f) {
                        state.repeatTimer += dt;
                        if (state.repeatTimer >= 0.03f) {
                            trigger = true;
                            state.repeatTimer = 0.0f;
                        }
                    }
                }
            }
            state.isDown = isDown;

            if (trigger) {
                m_emitters[m_selectedEmitterIndex].tilePos += arrowDeltas[k];
                arrowMoved = true;
            }
        }

        if (arrowMoved) {
            m_envParticleManager.setEmitters(m_emitters);
            m_isDirty = true;
        }

        // Клавиша Delete / Backspace для удаления выделенного эмиттера
        bool keyDel = (glfwGetKey(window, GLFW_KEY_DELETE) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS);
        if (keyDel) {
            std::string eName = m_emitters[m_selectedEmitterIndex].type;
            m_emitters.erase(m_emitters.begin() + m_selectedEmitterIndex);
            m_envParticleManager.setEmitters(m_emitters);
            m_selectedEmitterIndex = -1;
            m_isDraggingEmitter = false;
            m_isDirty = true;
            showToast("Эмиттер удален: " + eName, glm::vec4(1.0f, 0.4f, 0.4f, 1.0f));
        }
    }

    // Сдвиг всей карты (тайлы, спавны, базы, вагонетки, эмиттеры, декор): Ctrl + Стрелочки (← / → / ↑ / ↓)
    if (isCtrl) {
        int shiftKeys[4] = { GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_UP, GLFW_KEY_DOWN };
        int shiftDx[4] = { -1, 1, 0, 0 };
        int shiftDy[4] = { 0, 0, -1, 1 };

        for (int k = 0; k < 4; ++k) {
            int key = shiftKeys[k];
            bool isDown = (glfwGetKey(window, key) == GLFW_PRESS);
            auto& state = m_keyStates[key];
            bool trigger = false;
            if (isDown) {
                if (!state.isDown) {
                    trigger = true;
                    state.holdTimer = 0.0f;
                    state.repeatTimer = 0.0f;
                } else {
                    state.holdTimer += dt;
                    if (state.holdTimer >= 0.25f) {
                        state.repeatTimer += dt;
                        if (state.repeatTimer >= 0.08f) {
                            trigger = true;
                            state.repeatTimer = 0.0f;
                        }
                    }
                }
            }
            state.isDown = isDown;

            if (trigger) {
                shiftMap(shiftDx[k], shiftDy[k]);
                break;
            }
        }
    }

    // Панорамирование камеры:
    // 1) В режиме настройки камеры (m_isCameraConfigMode): зажатие ПКМ или СКМ или Пробел+ЛКМ
    // 2) В обычном режиме: зажатие СКМ или Пробел+ЛКМ
    bool middleDown = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS);
    bool spaceDown  = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);
    bool rmbPanCamera = m_isCameraConfigMode && rightDown;
    bool panActive  = middleDown || (spaceDown && leftDown) || rmbPanCamera;

    if (panActive) {
        if (m_isPanning) {
            glm::vec2 mouseDelta = mousePos - m_prevMousePos;
            m_cameraPan += mouseDelta;
            if (m_isCameraConfigMode) {
                glm::vec2 worldCenter = screenToWorld(glm::vec2(static_cast<float>(m_width) * 0.5f, static_cast<float>(m_height) * 0.5f));
                m_cameraSettings.targetTile = worldPixelToTile(worldCenter);
                m_cameraSettings.zoom = m_zoom;
            }
        }
        m_isPanning = true;
        m_prevMousePos = mousePos;
        m_suppressPlacementUntilRelease = true;
    } else {
        m_isPanning = false;
        m_prevMousePos = mousePos;
    }

    // В режиме настройки камеры: плавное перемещение стрелочками клавиатуры (← / → / ↑ / ↓)
    if (m_isCameraConfigMode && !isCtrl) {
        int camArrowKeys[4] = { GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_UP, GLFW_KEY_DOWN };
        glm::vec2 camDeltas[4] = {
            glm::vec2(-1.0f, 0.0f),
            glm::vec2(1.0f, 0.0f),
            glm::vec2(0.0f, -1.0f),
            glm::vec2(0.0f, 1.0f)
        };
        float stepTiles = (isShift ? 2.5f : 0.5f);

        for (int k = 0; k < 4; ++k) {
            int key = camArrowKeys[k];
            bool isDown = (glfwGetKey(window, key) == GLFW_PRESS);
            auto& state = m_keyStates[key];
            bool trigger = false;
            if (isDown) {
                if (!state.isDown) {
                    trigger = true;
                    state.holdTimer = 0.0f;
                    state.repeatTimer = 0.0f;
                } else {
                    state.holdTimer += dt;
                    if (state.holdTimer >= 0.18f) {
                        state.repeatTimer += dt;
                        if (state.repeatTimer >= 0.03f) {
                            trigger = true;
                            state.repeatTimer = 0.0f;
                        }
                    }
                }
            }
            state.isDown = isDown;

            if (trigger) {
                m_cameraSettings.targetTile += camDeltas[k] * stepTiles;
                m_cameraSettings.zoom = m_zoom;
                glm::vec2 screenCenter = glm::vec2(static_cast<float>(m_width) * 0.5f, static_cast<float>(m_height) * 0.5f);
                m_cameraPan = screenCenter - getWorldCamCenter() * m_zoom;
            }
        }
    }

    // Зум камеры колёсиком мыши (зум в точку под курсором)
    if (!panActive && !m_toolbarUI.isHovered(mousePos.x, mousePos.y)) {
        float scrollY = InputManager::getScrollY();
        if (std::abs(scrollY) > 0.01f) {
            float zoomFactor = (scrollY > 0.0f) ? 1.12f : (1.0f / 1.12f);
            float oldZoom = m_zoom;
            float newZoom = std::clamp(m_zoom * zoomFactor, 0.4f, 2.5f);
            if (newZoom != oldZoom) {
                glm::vec2 worldPosUnderCursor = screenToWorld(mousePos);
                m_zoom = newZoom;
                m_cameraPan = mousePos - worldPosUnderCursor * m_zoom;
                if (m_isCameraConfigMode) {
                    glm::vec2 worldCenter = screenToWorld(glm::vec2(static_cast<float>(m_width) * 0.5f, static_cast<float>(m_height) * 0.5f));
                    m_cameraSettings.targetTile = worldPixelToTile(worldCenter);
                    m_cameraSettings.zoom = m_zoom;
                }
            }
        }
    }

    // Сброс камеры (клавиша F)
    bool keyF = (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS);
    if (keyF && !m_keyFPressedLastFrame) {
        m_zoom = 1.0f;
        glm::vec2 screenCenter = glm::vec2(static_cast<float>(m_width) * 0.5f, static_cast<float>(m_height) * 0.5f);
        glm::vec2 defCenter = getDefaultWorldCenter();
        m_cameraPan = screenCenter - defCenter * m_zoom;
        if (m_isCameraConfigMode) {
            m_cameraSettings.targetTile = glm::vec2(static_cast<float>(m_gridWidth) * 0.5f, static_cast<float>(m_gridHeight) * 0.5f);
            m_cameraSettings.zoom = m_zoom;
        }
        showToast("Камера центрирована (1.0x)", glm::vec4(0.3f, 0.85f, 1.0f, 1.0f));
    }
    m_keyFPressedLastFrame = keyF;

    bool clickedUI = false;

    // 0. Плавающая панель настроек эмиттера партиклов (Inspector)
    if (m_selectedEmitterIndex >= 0 && m_selectedEmitterIndex < static_cast<int>(m_emitters.size())) {
        glm::vec2 emWorld = m_grid ? (m_grid->getOffset() + m_emitters[m_selectedEmitterIndex].tilePos * m_grid->getCellSize()) : glm::vec2(0.0f);
        glm::vec2 emScreen = worldToScreen(emWorld);

        bool handled = m_emitterInspector.handleInput(
            mousePos, leftDown, m_isLeftMouseDown,
            m_emitters[m_selectedEmitterIndex], m_selectedEmitterIndex, emScreen,
            static_cast<float>(m_width), static_cast<float>(m_height),
            getTopBarHeight(), getBottomDockHeight(),
            /* onChanged */ [this]() {
                m_envParticleManager.setEmitters(m_emitters);
                m_envParticleManager.triggerBurstAt(m_selectedEmitterIndex);
                m_isDirty = true;
            },
            /* onDelete */ [this]() {
                m_emitters.erase(m_emitters.begin() + m_selectedEmitterIndex);
                m_selectedEmitterIndex = -1;
                m_isDraggingEmitter = false;
                m_emitterInspector.resetPosition();
                m_envParticleManager.setEmitters(m_emitters);
                m_isDirty = true;
                showToast("Эмиттер удален", glm::vec4(1.0f, 0.4f, 0.4f, 1.0f));
            },
            /* onClose */ [this]() {
                m_selectedEmitterIndex = -1;
                m_isDraggingEmitter = false;
            },
            /* showToast */ [this](const std::string& msg, const glm::vec4& color) {
                showToast(msg, color);
            }
        );

        if (handled) {
            clickedUI = true;
            m_isDraggingEmitter = false;
            m_isLeftMouseDown = leftDown;
            m_isRightMouseDown = rightDown;
            return;
        }
    }

    // 0.1. Плавающая панель настроек декора (Decoration Inspector)
    if (m_selectedDecorationIndex >= 0 && m_selectedDecorationIndex < static_cast<int>(m_decorations.size())) {
        const auto& dec = m_decorations[m_selectedDecorationIndex];
        glm::vec2 decActualWorld = m_grid ? (m_grid->getOffset() + dec.tilePos * m_grid->getCellSize()) : dec.worldPos;
        glm::vec2 decScreen = worldToScreen(decActualWorld);

        bool handled = m_decorInspector.handleInput(
            mousePos, leftDown, m_isLeftMouseDown,
            m_decorations[m_selectedDecorationIndex], m_selectedDecorationIndex, decScreen,
            static_cast<float>(m_width), static_cast<float>(m_height),
            getTopBarHeight(), getBottomDockHeight(),
            /* onChanged */ [this]() {
                m_isDirty = true;
            },
            /* onDelete */ [this]() {
                std::string dName = m_decorations[m_selectedDecorationIndex].type;
                m_decorations.erase(m_decorations.begin() + m_selectedDecorationIndex);
                m_selectedDecorationIndex = -1;
                m_isDraggingDecoration = false;
                m_decorInspector.resetPosition();
                m_isDirty = true;
                showToast("Декор удален: " + dName, glm::vec4(1.0f, 0.4f, 0.4f, 1.0f));
            },
            /* onClose */ [this]() {
                m_selectedDecorationIndex = -1;
                m_isDraggingDecoration = false;
            },
            /* showToast */ [this](const std::string& msg, const glm::vec4& color) {
                showToast(msg, color);
            }
        );

        if (handled) {
            clickedUI = true;
            m_isDraggingDecoration = false;
            m_isLeftMouseDown = leftDown;
            m_isRightMouseDown = rightDown;
            return;
        }
    }

    // 0.2. Плавающая панель инспектора спавнера и базы (Entity Inspector)
    if (m_selectedEntityIndex >= 0) {
        if (m_selectedEntityType == EditorEntityType::Spawner && m_selectedEntityIndex < static_cast<int>(m_spawners.size())) {
            glm::vec2 entityWorld = m_grid ? (m_grid->gridToPixel(m_spawners[m_selectedEntityIndex].pos.x, m_spawners[m_selectedEntityIndex].pos.y) + glm::vec2(m_grid->getCellSize() * 0.5f)) : glm::vec2(0.0f);
            glm::vec2 entityScreen = worldToScreen(entityWorld);

            bool handled = m_entityInspector.handleInput(
                mousePos, leftDown, m_isLeftMouseDown,
                m_spawners[m_selectedEntityIndex], m_selectedEntityIndex, entityScreen,
                static_cast<float>(m_width), static_cast<float>(m_height),
                getTopBarHeight(), getBottomDockHeight(),
                /* onChanged */ [this]() {
                    m_isDirty = true;
                    recalculatePaths();
                },
                /* onDelete */ [this]() {
                    m_spawners.erase(m_spawners.begin() + m_selectedEntityIndex);
                    m_selectedEntityIndex = -1;
                    m_entityInspector.resetPosition();
                    m_isDirty = true;
                    recalculatePaths();
                    showToast("Спавнер удален", glm::vec4(1.0f, 0.4f, 0.4f, 1.0f));
                },
                /* onClose */ [this]() {
                    m_selectedEntityIndex = -1;
                },
                /* showToast */ [this](const std::string& msg, const glm::vec4& color) {
                    showToast(msg, color);
                }
            );

            if (handled) {
                clickedUI = true;
                m_isLeftMouseDown = leftDown;
                m_isRightMouseDown = rightDown;
                return;
            }
        } else if (m_selectedEntityType == EditorEntityType::Base && m_selectedEntityIndex < static_cast<int>(m_bases.size())) {
            glm::vec2 entityWorld = m_grid ? (m_grid->gridToPixel(m_bases[m_selectedEntityIndex].x, m_bases[m_selectedEntityIndex].y) + glm::vec2(m_grid->getCellSize() * 0.5f)) : glm::vec2(0.0f);
            glm::vec2 entityScreen = worldToScreen(entityWorld);

            bool handled = m_entityInspector.handleInput(
                mousePos, leftDown, m_isLeftMouseDown,
                m_bases[m_selectedEntityIndex], m_selectedEntityIndex, entityScreen,
                static_cast<float>(m_width), static_cast<float>(m_height),
                getTopBarHeight(), getBottomDockHeight(),
                /* onChanged */ [this]() {
                    m_isDirty = true;
                    recalculatePaths();
                },
                /* onDelete */ [this]() {
                    m_bases.erase(m_bases.begin() + m_selectedEntityIndex);
                    m_selectedEntityIndex = -1;
                    m_entityInspector.resetPosition();
                    m_isDirty = true;
                    recalculatePaths();
                    showToast("База удалена", glm::vec4(1.0f, 0.4f, 0.4f, 1.0f));
                },
                /* onClose */ [this]() {
                    m_selectedEntityIndex = -1;
                },
                /* showToast */ [this](const std::string& msg, const glm::vec4& color) {
                    showToast(msg, color);
                }
            );

            if (handled) {
                clickedUI = true;
                m_isLeftMouseDown = leftDown;
                m_isRightMouseDown = rightDown;
                return;
            }
        } else {
            m_selectedEntityIndex = -1;
        }
    }

    // 1. Проверка клика по кнопкам тулбара и палитры через EditorToolbarUI
    EditorAction action = m_toolbarUI.handleInput(mousePos.x, mousePos.y, leftDown, m_isLeftMouseDown, isShift, buildEditorContext(), isAlt);
    if (action.type != EditorActionType::None) {
        clickedUI = true;
        m_suppressPlacementUntilRelease = true;
        switch (action.type) {
            case EditorActionType::SetBrush:
                m_currentBrush = action.brush;
                if (m_currentBrush == EditorBrush::Base && m_selectedId < 0) {
                    m_selectedId = 0;
                }
                if (m_currentBrush == EditorBrush::Spawner || m_currentBrush == EditorBrush::Base ||
                    m_currentBrush == EditorBrush::Rail || m_currentBrush == EditorBrush::RailStart ||
                    m_currentBrush == EditorBrush::RailEnd) {
                    if (m_toolbarUI.getCategory() != PaletteCategory::SpecialObjects) {
                        m_toolbarUI.setCategory(PaletteCategory::SpecialObjects);
                        updateButtonLayout();
                    }
                } else if (m_currentBrush == EditorBrush::EmitterSteamJet ||
                           m_currentBrush == EditorBrush::EmitterWaterDrip ||
                           m_currentBrush == EditorBrush::EmitterSparks ||
                           m_currentBrush == EditorBrush::EmitterSmoke ||
                           m_currentBrush == EditorBrush::EmitterFog) {
                    if (m_toolbarUI.getCategory() != PaletteCategory::Particles) {
                        m_toolbarUI.setCategory(PaletteCategory::Particles);
                        updateButtonLayout();
                    }
                } else if (m_currentBrush >= EditorBrush::DecorBush && m_currentBrush <= EditorBrush::DecorFog) {
                    if (m_toolbarUI.getCategory() != PaletteCategory::Decorations) {
                        m_toolbarUI.setCategory(PaletteCategory::Decorations);
                        updateButtonLayout();
                    }
                } else if (m_currentBrush != EditorBrush::Eraser) {
                    if (m_toolbarUI.getCategory() != PaletteCategory::Tiles) {
                        m_toolbarUI.setCategory(PaletteCategory::Tiles);
                        updateButtonLayout();
                    }
                }
                break;
            case EditorActionType::SwitchPaletteCategory:
                updateButtonLayout();
                if (m_toolbarUI.getCategory() == PaletteCategory::Tiles) {
                    showToast("Палитра: Тайлы");
                } else if (m_toolbarUI.getCategory() == PaletteCategory::SpecialObjects) {
                    showToast("Палитра: Спец-объекты");
                } else if (m_toolbarUI.getCategory() == PaletteCategory::Particles) {
                    showToast("Палитра: Партиклы");
                } else {
                    showToast("Палитра: Декор");
                }
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
                resizeMap(m_gridWidth + action.intParam, m_gridHeight + action.intParam2, action.isAltDown);
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
            case EditorActionType::ToggleBackground:
                m_background = (m_background == "pipes_canal") ? "default" : "pipes_canal";
                m_isDirty = true;
                showToast(m_background == "pipes_canal" ? "Фон: Дюкеры Канала" : "Фон: Стандартный");
                updateButtonLayout();
                break;
            case EditorActionType::ToggleCameraMode:
                m_isCameraConfigMode = !m_isCameraConfigMode;
                if (m_isCameraConfigMode) {
                    if (!m_cameraSettings.isCustom) {
                        glm::vec2 worldCenter = screenToWorld(glm::vec2(static_cast<float>(m_width) * 0.5f, static_cast<float>(m_height) * 0.5f));
                        m_cameraSettings.targetTile = worldPixelToTile(worldCenter);
                        m_cameraSettings.zoom = m_zoom;
                    }
                    showToast("Режим камеры: сдвиньте/зуммируйте и нажмите [Зафиксировать камеру]", glm::vec4(0.35f, 0.85f, 1.0f, 1.0f));
                } else {
                    showToast("Режим камеры выключен", glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));
                }
                updateGridDimensions();
                updateButtonLayout();
                break;
            case EditorActionType::LockCamera: {
                glm::vec2 worldCenter = screenToWorld(glm::vec2(static_cast<float>(m_width) * 0.5f, static_cast<float>(m_height) * 0.5f));
                m_cameraSettings.targetTile = worldPixelToTile(worldCenter);
                m_cameraSettings.zoom = m_zoom;
                m_cameraSettings.isCustom = true;
                m_isDirty = true;
                showToast("Стартовая камера сохранена", glm::vec4(0.35f, 1.0f, 0.55f, 1.0f));
                updateButtonLayout();
                break;
            }
            case EditorActionType::ResetCamera:
                m_cameraSettings.isCustom = false;
                m_isCameraConfigMode = false;
                m_isDirty = true;
                showToast("Камера сброшена на Авто", glm::vec4(1.0f, 0.8f, 0.35f, 1.0f));
                updateGridDimensions();
                updateButtonLayout();
                break;
            case EditorActionType::ToggleHelpModal:
                m_helpModal.toggle();
                m_isLeftMouseDown = true;
                m_suppressPlacementUntilRelease = true;
                return;
            case EditorActionType::ToggleHudPreview:
                cycleHudPreviewMode();
                break;
            case EditorActionType::OpenLevelSettingsModal:
                m_levelSettingsModal.open(m_startingMoney, m_startingHealth, m_allowedTowers, m_towerMaxTiers);
                m_suppressClickUntilRelease = true;
                break;
            case EditorActionType::ModifyStartingMoney:
            case EditorActionType::ModifyStartingHealth:
                break;
            default:
                break;
        }
    }

    // 2. Если кликаем по рабочей области сетки
    if (!clickedUI && !panActive && !m_isCameraConfigMode && !m_toolbarUI.isHovered(mousePos.x, mousePos.y) && m_grid) {
        glm::vec2 worldMouse = screenToWorld(mousePos);
        glm::ivec2 gridPos = m_grid->pixelToGrid(worldMouse);
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
                        case 0: pickedBrush = EditorBrush::Ground;   tileName = (m_background == "pipes_canal") ? "Асфальт" : "Земля"; break;
                        case 1: pickedBrush = EditorBrush::Path;     tileName = "Дорога"; break;
                        case 2: pickedBrush = EditorBrush::Platform; tileName = (m_background == "pipes_canal") ? "Трава/Обочина" : "Платформа"; break;
                        case 3: pickedBrush = EditorBrush::Wall;     tileName = "Стена"; break;
                        case 4: pickedBrush = EditorBrush::Chasm;    tileName = "Шурф"; break;
                        case 5: pickedBrush = EditorBrush::Rail;     tileName = "Рельсы"; break;
                        default: pickedBrush = EditorBrush::Ground;  tileName = (m_background == "pipes_canal") ? "Асфальт" : "Земля"; break;
                    }
                }

                // Декорации (если кликнули вблизи декора)
                int nearDec = findNearestDecorationIndex(worldMouse, 28.0f);
                if (nearDec >= 0 && nearDec < static_cast<int>(m_decorations.size())) {
                    const std::string& dt = m_decorations[nearDec].type;
                    if (dt == "bush") { pickedBrush = EditorBrush::DecorBush; tileName = "Куст"; }
                    else if (dt == "grass_tuft") { pickedBrush = EditorBrush::DecorGrass; tileName = "Пучок травы"; }
                    else if (dt == "grass_field") { pickedBrush = EditorBrush::DecorGrassField; tileName = "Поле травы"; }
                    else if (dt == "flower") { pickedBrush = EditorBrush::DecorFlower; tileName = "Цветы"; }
                    else if (dt == "stone") { pickedBrush = EditorBrush::DecorStone; tileName = "Камень"; }
                    else if (dt == "helmet") { pickedBrush = EditorBrush::DecorHelmet; tileName = "Каска"; }
                    else if (dt == "pickaxe") { pickedBrush = EditorBrush::DecorPickaxe; tileName = "Кирка"; }
                    else if (dt == "puddle") { pickedBrush = EditorBrush::DecorPuddle; tileName = "Лужа"; }
                    else if (dt == "crack") { pickedBrush = EditorBrush::DecorCrack; tileName = "Трещина"; }
                    else if (dt == "fog") { pickedBrush = EditorBrush::DecorFog; tileName = "Туман"; }
                    foundObj = true;
                }

                m_currentBrush = pickedBrush;
                if (pickedBrush == EditorBrush::Spawner || pickedBrush == EditorBrush::Base ||
                    pickedBrush == EditorBrush::Rail || pickedBrush == EditorBrush::RailStart ||
                    pickedBrush == EditorBrush::RailEnd) {
                    m_toolbarUI.setCategory(PaletteCategory::SpecialObjects);
                } else if (pickedBrush == EditorBrush::EmitterSteamJet || pickedBrush == EditorBrush::EmitterWaterDrip ||
                           pickedBrush == EditorBrush::EmitterSparks || pickedBrush == EditorBrush::EmitterSmoke ||
                           pickedBrush == EditorBrush::EmitterFog) {
                    m_toolbarUI.setCategory(PaletteCategory::Particles);
                } else if (pickedBrush >= EditorBrush::DecorBush && pickedBrush <= EditorBrush::DecorFog) {
                    m_toolbarUI.setCategory(PaletteCategory::Decorations);
                } else {
                    m_toolbarUI.setCategory(PaletteCategory::Tiles);
                }
                updateButtonLayout();

                showToast("Выбран тайл: " + tileName, glm::vec4(0.3f, 0.8f, 1.0f, 1.0f));
                m_suppressPlacementUntilRelease = true;
            }
        }
        // 2.2. ПРЯМОУГОЛЬНАЯ ЗАЛИВКА: Shift + зажатый ЛКМ (только для тайлов и спец-объектов)
        else if (isShift && !isAlt && m_toolbarUI.getCategory() != PaletteCategory::Particles && m_toolbarUI.getCategory() != PaletteCategory::Decorations) {
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
        // 2.3. СВОБОДНАЯ РАБОТА С ПАРТИКЛАМИ НА ВКЛАДКЕ [3] ПАРТИКЛЫ (ВНЕ СЕТКИ, DRAG, СЛОЁНЫЙ ЛАСТИК)
        else if (m_toolbarUI.getCategory() == PaletteCategory::Particles && !m_suppressPlacementUntilRelease) {
            float clickRadius = std::max(20.0f, m_grid->getCellSize() * 0.45f);

            // А. Перетаскивание (Drag) выделенного эмиттера зажатой мышью
            if (leftDown && m_isDraggingEmitter && m_selectedEmitterIndex >= 0 &&
                m_selectedEmitterIndex < static_cast<int>(m_emitters.size())) {
                glm::vec2 targetWorld = worldMouse + m_emitterDragOffset;
                m_emitters[m_selectedEmitterIndex].tilePos = (targetWorld - m_grid->getOffset()) / m_grid->getCellSize();
                m_envParticleManager.setEmitters(m_emitters);
                m_isDirty = true;
            }
            // Б. Одиночный клик ЛКМ
            else if (leftDown && !m_isLeftMouseDown) {
                // Слоёный ластик партиклов: удаляет ТОЛЬКО эмиттер под курсором, не трогая тайлы сетки!
                if (m_currentBrush == EditorBrush::Eraser) {
                    eraseEmitterNear(worldMouse, clickRadius);
                } else {
                    // Проверяем, кликнули ли по существующему эмиттеру для выделения / драга
                    int nearestIdx = findNearestEmitterIndex(worldMouse, clickRadius);
                    if (nearestIdx >= 0) {
                        m_selectedEmitterIndex = nearestIdx;
                        m_envParticleManager.triggerBurstAt(nearestIdx);
                        m_isDraggingEmitter = true;
                        glm::vec2 emWorld = m_grid->getOffset() + m_emitters[nearestIdx].tilePos * m_grid->getCellSize();
                        m_emitterDragOffset = emWorld - worldMouse;
                        showToast("Выделен эмиттер #" + std::to_string(nearestIdx + 1) + " [" + m_emitters[nearestIdx].type + "]", glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
                    } else if (m_currentBrush != EditorBrush::None) {
                        // Свободное размещение нового эмиттера ровно в точке курсора worldMouse
                        std::string eType = "steam_jet";
                        std::string eName = "Свищ пара";
                        float pMin = 5.0f, pMax = 12.0f, bDur = 2.2f, spd = 180.0f, pScl = 1.0f;

                        if (m_currentBrush == EditorBrush::EmitterWaterDrip) {
                            eType = "water_drip"; eName = "Капель"; pMin = 2.0f; pMax = 5.0f; bDur = 1.2f; spd = 120.0f; pScl = 1.0f;
                        } else if (m_currentBrush == EditorBrush::EmitterSparks) {
                            eType = "sparks"; eName = "Искры"; pMin = 4.0f; pMax = 10.0f; bDur = 1.5f; spd = 220.0f; pScl = 1.0f;
                        } else if (m_currentBrush == EditorBrush::EmitterSmoke) {
                            eType = "smoke"; eName = "Дым"; pMin = 3.0f; pMax = 8.0f; bDur = 2.8f; spd = 140.0f; pScl = 1.2f;
                        } else if (m_currentBrush == EditorBrush::EmitterFog) {
                            eType = "fog"; eName = "Туман"; pMin = 0.5f; pMax = 1.0f; bDur = 10.0f; spd = 35.0f; pScl = 1.6f;
                        }

                        ParticleEmitterConfig cfg;
                        cfg.id = static_cast<int>(m_emitters.size()) + 1;
                        cfg.type = eType;
                        if (eType == "fog") cfg.loopContinuous = true;
                        cfg.tilePos = (worldMouse - m_grid->getOffset()) / m_grid->getCellSize();
                        cfg.angleDeg = m_defaultEmitterAngle;
                        cfg.speed = spd;
                        cfg.particleScale = pScl;
                        cfg.periodMin = pMin;
                        cfg.periodMax = pMax;
                        cfg.burstDuration = bDur;

                        m_emitters.push_back(cfg);
                        m_envParticleManager.setEmitters(m_emitters);
                        m_selectedEmitterIndex = static_cast<int>(m_emitters.size()) - 1;
                        m_envParticleManager.triggerBurstAt(m_selectedEmitterIndex);
                        m_isDraggingEmitter = false;
                        m_emitterDragOffset = glm::vec2(0.0f);
                        m_isDirty = true;
                        showToast("Установлен эмиттер: " + eName, glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
                    } else {
                        m_selectedEmitterIndex = -1;
                    }
                }
            }
            // В. Клик ПКМ на вкладке партиклов: поворот струи на 45° или слоёный ластик
            else if (rightDown && !m_isRightMouseDown) {
                int nearestIdx = findNearestEmitterIndex(worldMouse, clickRadius);
                if (nearestIdx >= 0) {
                    if (m_currentBrush == EditorBrush::Eraser) {
                        eraseEmitterNear(worldMouse, clickRadius);
                    } else {
                        m_emitters[nearestIdx].angleDeg = std::fmod(m_emitters[nearestIdx].angleDeg + 45.0f, 360.0f);
                        if (m_emitters[nearestIdx].angleDeg < 0.0f) m_emitters[nearestIdx].angleDeg += 360.0f;
                        m_envParticleManager.setEmitters(m_emitters);
                        m_selectedEmitterIndex = nearestIdx;
                        m_envParticleManager.triggerBurstAt(nearestIdx);
                        showToast("Угол струи: " + std::to_string(static_cast<int>(m_emitters[nearestIdx].angleDeg)) + "°", glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
                        m_isDirty = true;
                    }
                } else {
                    m_selectedEmitterIndex = -1;
                    if (m_currentBrush != EditorBrush::None) {
                        m_currentBrush = EditorBrush::None;
                        showToast("Кисть сброшена (нейтральный курсор)");
                    }
                }
            }
        }
        // 2.4. СВОБОДНАЯ РАБОТА С ДЕКОРОМ НА ВКЛАДКЕ [4] ДЕКОР (ВНЕ СЕТКИ, DRAG & DROP, СЛОЁНЫЙ ЛАСТИК)
        else if (m_toolbarUI.getCategory() == PaletteCategory::Decorations && !m_suppressPlacementUntilRelease) {
            float clickRadius = std::max(22.0f, m_grid->getCellSize() * 0.45f);

            // А. Перетаскивание (Drag & Drop) выделенного декора зажатой мышью
            if (leftDown && m_isDraggingDecoration && m_selectedDecorationIndex >= 0 &&
                m_selectedDecorationIndex < static_cast<int>(m_decorations.size())) {
                glm::vec2 targetWorld = worldMouse + m_decorationDragOffset;
                if (m_grid && m_grid->getCellSize() > 0.001f) {
                    m_decorations[m_selectedDecorationIndex].tilePos = (targetWorld - m_grid->getOffset()) / m_grid->getCellSize();
                    m_decorations[m_selectedDecorationIndex].worldPos = targetWorld;
                } else {
                    m_decorations[m_selectedDecorationIndex].worldPos = targetWorld;
                }
                m_isDirty = true;
            }
            // Б. Одиночный клик ЛКМ
            else if (leftDown && !m_isLeftMouseDown) {
                // Слоёный ластик декора: удаляет ТОЛЬКО декор под курсором, не трогая тайлы сетки!
                if (m_currentBrush == EditorBrush::Eraser) {
                    eraseDecorationNear(worldMouse, clickRadius);
                } else {
                    // Проверяем, кликнули ли по существующему декору для выделения / драга
                    int nearestIdx = findNearestDecorationIndex(worldMouse, clickRadius);
                    if (nearestIdx >= 0) {
                        m_selectedDecorationIndex = nearestIdx;
                        m_isDraggingDecoration = true;
                        glm::vec2 decActualWorld = m_grid ? (m_grid->getOffset() + m_decorations[nearestIdx].tilePos * m_grid->getCellSize()) : m_decorations[nearestIdx].worldPos;
                        m_decorationDragOffset = decActualWorld - worldMouse;
                        showToast("Выделен декор #" + std::to_string(m_decorations[nearestIdx].id) + " [" + m_decorations[nearestIdx].type + "]", glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
                    } else if (m_currentBrush != EditorBrush::None) {
                        // Свободное размещение нового декора ровно в мировой точке курсора worldMouse
                        std::string dType = "bush";
                        std::string dName = "Куст";
                        bool isDestructible = true;

                        if (m_currentBrush == EditorBrush::DecorGrass) {
                            dType = "grass_tuft"; dName = "Пучок травы";
                        } else if (m_currentBrush == EditorBrush::DecorGrassField) {
                            dType = "grass_field"; dName = "Поле травы";
                        } else if (m_currentBrush == EditorBrush::DecorFlower) {
                            dType = "flower"; dName = "Цветы";
                        } else if (m_currentBrush == EditorBrush::DecorStone) {
                            dType = "stone"; dName = "Камень";
                        } else if (m_currentBrush == EditorBrush::DecorHelmet) {
                            dType = "helmet"; dName = "Каска";
                        } else if (m_currentBrush == EditorBrush::DecorPickaxe) {
                            dType = "pickaxe"; dName = "Кирка";
                        } else if (m_currentBrush == EditorBrush::DecorPuddle) {
                            dType = "puddle"; dName = "Лужа";
                        } else if (m_currentBrush == EditorBrush::DecorCrack) {
                            dType = "crack"; dName = "Трещина";
                        } else if (m_currentBrush == EditorBrush::DecorFog) {
                            dType = "fog"; dName = "Туман"; isDestructible = false;
                        }

                        DecorationConfig cfg;
                        cfg.id = static_cast<int>(m_decorations.size()) + 1;
                        cfg.type = dType;
                        if (dType == "fog") {
                            cfg.scale = 1.4f;
                            cfg.opacity = 0.45f;
                            cfg.destructible = false;
                        }
                        if (m_grid && m_grid->getCellSize() > 0.001f) {
                            if (m_currentBrush == EditorBrush::DecorGrassField) {
                                glm::vec2 hoverTile = (worldMouse - m_grid->getOffset()) / m_grid->getCellSize();
                                cfg.tilePos = glm::floor(hoverTile) + glm::vec2(0.5f);
                                cfg.worldPos = m_grid->getOffset() + cfg.tilePos * m_grid->getCellSize();
                            } else {
                                cfg.tilePos = (worldMouse - m_grid->getOffset()) / m_grid->getCellSize();
                                cfg.worldPos = worldMouse;
                            }
                        } else {
                            cfg.tilePos = glm::vec2(0.0f);
                            cfg.worldPos = worldMouse;
                        }
                        cfg.scale = 1.0f;
                        cfg.scaleX = 1.0f;
                        cfg.scaleY = 1.0f;
                        cfg.rotation = m_defaultDecorationRotation;
                        cfg.destructible = isDestructible;

                        m_decorations.push_back(cfg);
                        m_selectedDecorationIndex = static_cast<int>(m_decorations.size()) - 1;
                        m_isDraggingDecoration = false;
                        m_decorationDragOffset = glm::vec2(0.0f);
                        m_isDirty = true;
                        showToast("Установлен декор: " + dName, glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
                    } else {
                        // Клик в пустоту нейтральным курсором снимает выделение
                        m_selectedDecorationIndex = -1;
                    }
                }
            }
            // В. Клик ПКМ на вкладке декора: поворот декора на 45° или слоёный ластик
            else if (rightDown && !m_isRightMouseDown) {
                int nearestIdx = findNearestDecorationIndex(worldMouse, clickRadius);
                if (nearestIdx >= 0) {
                    if (m_currentBrush == EditorBrush::Eraser) {
                        eraseDecorationNear(worldMouse, clickRadius);
                    } else {
                        m_decorations[nearestIdx].rotation = std::fmod(m_decorations[nearestIdx].rotation + 45.0f, 360.0f);
                        if (m_decorations[nearestIdx].rotation < 0.0f) m_decorations[nearestIdx].rotation += 360.0f;
                        m_selectedDecorationIndex = nearestIdx;
                        showToast("Угол декора: " + std::to_string(static_cast<int>(m_decorations[nearestIdx].rotation)) + "°", glm::vec4(0.4f, 0.85f, 1.0f, 1.0f));
                        m_isDirty = true;
                    }
                } else {
                    // Клик ПКМ в пустоту: сброс выделения и кисти в нейтральный курсор
                    m_selectedDecorationIndex = -1;
                    if (m_currentBrush != EditorBrush::None) {
                        m_currentBrush = EditorBrush::None;
                        showToast("Кисть сброшена (нейтральный курсор)");
                    }
                }
            }
        }
        // 2.5. СТАНДАРТНАЯ РАБОТА С СЕТКОЙ (ТАЙЛЫ И СПЕЦ-ОБЪЕКТЫ)
        else if (!m_suppressPlacementUntilRelease) {
            if (inBounds) {
                // Если мы в категории спец-объектов или нейтральном курсоре - проверяем выбор существующего спавнера/базы
                if (leftDown && !m_isLeftMouseDown) {
                    bool isSpecialCategory = (m_toolbarUI.getCategory() == PaletteCategory::SpecialObjects);
                    int clickedSpawner = -1;
                    for (int i = 0; i < static_cast<int>(m_spawners.size()); ++i) {
                        if (m_spawners[i].pos.x == gridPos.x && m_spawners[i].pos.y == gridPos.y) {
                            clickedSpawner = i;
                            break;
                        }
                    }
                    int clickedBase = -1;
                    for (int i = 0; i < static_cast<int>(m_bases.size()); ++i) {
                        if (m_bases[i].x == gridPos.x && m_bases[i].y == gridPos.y) {
                            clickedBase = i;
                            break;
                        }
                    }

                    if (clickedSpawner >= 0 && (isSpecialCategory || m_currentBrush == EditorBrush::None || m_currentBrush == EditorBrush::Spawner)) {
                        m_selectedEntityType = EditorEntityType::Spawner;
                        m_selectedEntityIndex = clickedSpawner;
                        m_selectedDecorationIndex = -1;
                        m_selectedEmitterIndex = -1;
                        showToast("Выбран спавнер S" + (m_spawners[clickedSpawner].targetBaseIndex == -1 ? ":Auto" : ">#" + std::to_string(m_spawners[clickedSpawner].targetBaseIndex)), glm::vec4(0.95f, 0.85f, 0.2f, 1.0f));
                        m_isLeftMouseDown = leftDown;
                        return;
                    } else if (clickedBase >= 0 && (isSpecialCategory || m_currentBrush == EditorBrush::None || m_currentBrush == EditorBrush::Base)) {
                        m_selectedEntityType = EditorEntityType::Base;
                        m_selectedEntityIndex = clickedBase;
                        m_selectedDecorationIndex = -1;
                        m_selectedEmitterIndex = -1;
                        showToast("Выбрана база B#" + std::to_string(m_bases[clickedBase].id), glm::vec4(0.3f, 0.8f, 1.0f, 1.0f));
                        m_isLeftMouseDown = leftDown;
                        return;
                    } else if (m_currentBrush == EditorBrush::None) {
                        m_selectedEntityIndex = -1;
                    }
                }

                bool isSinglePointBrush = (m_currentBrush == EditorBrush::Spawner || m_currentBrush == EditorBrush::Base ||
                                           m_currentBrush == EditorBrush::RailStart || m_currentBrush == EditorBrush::RailEnd);
                if (leftDown) {
                    if (m_currentBrush != EditorBrush::None && (!isSinglePointBrush || !m_isLeftMouseDown)) {
                        applyBrush(gridPos.x, gridPos.y, m_currentBrush);
                    }
                } else if (rightDown) {
                    if (m_rawLayout[gridPos.y][gridPos.x] == 0 && m_currentBrush != EditorBrush::None) {
                        // Клик ПКМ по пустой земле с активной кистью: сброс кисти в нейтральный курсор
                        if (!m_isRightMouseDown) {
                            m_currentBrush = EditorBrush::None;
                            showToast("Кисть сброшена (нейтральный курсор)");
                        }
                    } else {
                        eraseCell(gridPos.x, gridPos.y);
                    }
                }
            } else if (rightDown && !m_isRightMouseDown && m_currentBrush != EditorBrush::None) {
                // Клик ПКМ вне сетки: сброс активной кисти в нейтральный курсор
                m_currentBrush = EditorBrush::None;
                showToast("Кисть сброшена (нейтральный курсор)");
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

    if (!leftDown) {
        m_isDraggingEmitter = false;
        m_isDraggingDecoration = false;
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
        m_envParticleManager.update(dt, *m_grid);
    }

    if (m_waveModal.isOpen()) {
        m_waveModal.update(dt);
    }

    if (m_mapsBrowserModal.isOpen()) {
        m_mapsBrowserModal.update(dt);
    }

    if (m_levelSettingsModal.isOpen()) {
        m_levelSettingsModal.update(dt);
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
    glm::mat4 P_screen = glm::ortho(0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, -1.0f, 1.0f);

    // 0. Экранный фон окна за пределами игровой сцены
    m_renderer->setProjection(P_screen);
    if (m_textRenderer) m_textRenderer->updateProjection(P_screen);
    m_renderer->beginBatch();
    m_renderer->drawSprite(m_whiteTexture, glm::vec2(0.0f), glm::vec2(m_width, m_height), 0.0f, glm::vec3(0.08f, 0.09f, 0.12f));
    m_renderer->endBatch();

    // 1. Установка матрицы камеры мира (Zoom & Pan)
    glm::mat4 V = glm::mat4(1.0f);
    V = glm::translate(V, glm::vec3(m_cameraPan, 0.0f));
    V = glm::scale(V, glm::vec3(m_zoom, m_zoom, 1.0f));
    glm::mat4 cameraProj = P_screen * V;

    m_renderer->setProjection(cameraProj);
    m_renderer->beginBatch();

    // 1.1. Отрисовка фона игровой зоны (в мировых координатах)
    BackgroundRenderer::render(
        m_renderer.get(),
        m_whiteTexture,
        m_background,
        m_grid.get(),
        static_cast<float>(m_width),
        static_cast<float>(m_height)
    );

    // 2. Игровая сетка (отцентрирована строго между верхним и нижним барами)
    if (m_grid) {
        m_grid->draw(m_renderer.get(), m_whiteTexture, glm::vec3(1.0f), m_background);
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
        for (size_t bi = 0; bi < m_bases.size(); ++bi) {
            const auto& b = m_bases[bi];
            glm::vec2 pPos = m_grid->gridToPixel(b.x, b.y);
            glm::vec3 bCol = getIdColor(b.id);
            m_renderer->drawSprite(m_whiteTexture, pPos + glm::vec2(2.0f), glm::vec2(cellSize - 4.0f), 0.0f, bCol);
            m_renderer->drawSprite(m_whiteTexture, pPos + glm::vec2(5.0f), glm::vec2(cellSize - 10.0f), 0.0f, bCol * 0.85f);

            // Рамка выделения, если база выбрана в инспекторе
            if (m_selectedEntityIndex == static_cast<int>(bi) && m_selectedEntityType == EditorEntityType::Base) {
                float selB = 2.5f;
                glm::vec4 selCol(1.0f, 1.0f, 1.0f, 0.95f);
                m_renderer->drawSpriteRGBA(m_whiteTexture, pPos, glm::vec2(cellSize, selB), 0.0f, selCol);
                m_renderer->drawSpriteRGBA(m_whiteTexture, pPos + glm::vec2(0.0f, cellSize - selB), glm::vec2(cellSize, selB), 0.0f, selCol);
                m_renderer->drawSpriteRGBA(m_whiteTexture, pPos, glm::vec2(selB, cellSize), 0.0f, selCol);
                m_renderer->drawSpriteRGBA(m_whiteTexture, pPos + glm::vec2(cellSize - selB, 0.0f), glm::vec2(selB, cellSize), 0.0f, selCol);
            }
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

            // Рамка выделения, если спавнер выбран в инспекторе
            if (m_selectedEntityIndex == static_cast<int>(i) && m_selectedEntityType == EditorEntityType::Spawner) {
                float selB = 2.5f;
                glm::vec4 selCol(1.0f, 1.0f, 1.0f, 0.95f);
                m_renderer->drawSpriteRGBA(m_whiteTexture, pPos, glm::vec2(cellSize, selB), 0.0f, selCol);
                m_renderer->drawSpriteRGBA(m_whiteTexture, pPos + glm::vec2(0.0f, cellSize - selB), glm::vec2(cellSize, selB), 0.0f, selCol);
                m_renderer->drawSpriteRGBA(m_whiteTexture, pPos, glm::vec2(selB, cellSize), 0.0f, selCol);
                m_renderer->drawSpriteRGBA(m_whiteTexture, pPos + glm::vec2(cellSize - selB, 0.0f), glm::vec2(selB, cellSize), 0.0f, selCol);
            }
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

    // 4.3. ВИДОИСКАТЕЛЬ СТАРТОВОЙ КАМЕРЫ (Camera Viewport Frame)
    if (m_isCameraConfigMode || m_cameraSettings.isCustom || m_hudPreviewMode != HudPreviewMode::None) {
        float camZoom = std::clamp(m_cameraSettings.zoom, 0.4f, 2.5f);
        glm::vec2 camCenter = getWorldCamCenter();

        // Размер видоискателя игрока (соответствует охвату экрана при боевом зуме: screen / camZoom)
        glm::vec2 frameSize = getCameraViewportSize();
        float frameW = frameSize.x;
        float frameH = frameSize.y;
        glm::vec2 framePos = camCenter - glm::vec2(frameW * 0.5f, frameH * 0.5f);

        // Полупрозрачная белая подложка видоискателя
        float fillAlpha = m_isCameraConfigMode ? 0.08f : 0.04f;
        m_renderer->drawSpriteRGBA(m_whiteTexture, framePos, frameSize, 0.0f, glm::vec4(1.0f, 1.0f, 1.0f, fillAlpha));

        // Контурная рамка видоискателя
        glm::vec4 frameCol = m_isCameraConfigMode ? glm::vec4(1.0f, 1.0f, 1.0f, 0.95f) : glm::vec4(0.85f, 0.92f, 1.0f, 0.65f);
        float borderThick = 2.5f;
        m_renderer->drawSpriteRGBA(m_whiteTexture, framePos, glm::vec2(frameSize.x, borderThick), 0.0f, frameCol);
        m_renderer->drawSpriteRGBA(m_whiteTexture, framePos + glm::vec2(0.0f, frameSize.y - borderThick), glm::vec2(frameSize.x, borderThick), 0.0f, frameCol);
        m_renderer->drawSpriteRGBA(m_whiteTexture, framePos, glm::vec2(borderThick, frameSize.y), 0.0f, frameCol);
        m_renderer->drawSpriteRGBA(m_whiteTexture, framePos + glm::vec2(frameSize.x - borderThick, 0.0f), glm::vec2(borderThick, frameSize.y), 0.0f, frameCol);

        // Уголки видоискателя (Camera Viewport Brackets)
        float cornerLen = 28.0f;
        float cornerThick = 4.0f;
        glm::vec4 cornerCol = m_isCameraConfigMode ? glm::vec4(0.35f, 0.90f, 1.0f, 1.0f) : glm::vec4(1.0f, 1.0f, 1.0f, 0.85f);
        // Top-Left
        m_renderer->drawSpriteRGBA(m_whiteTexture, framePos, glm::vec2(cornerLen, cornerThick), 0.0f, cornerCol);
        m_renderer->drawSpriteRGBA(m_whiteTexture, framePos, glm::vec2(cornerThick, cornerLen), 0.0f, cornerCol);
        // Top-Right
        m_renderer->drawSpriteRGBA(m_whiteTexture, framePos + glm::vec2(frameSize.x - cornerLen, 0.0f), glm::vec2(cornerLen, cornerThick), 0.0f, cornerCol);
        m_renderer->drawSpriteRGBA(m_whiteTexture, framePos + glm::vec2(frameSize.x - cornerThick, 0.0f), glm::vec2(cornerThick, cornerLen), 0.0f, cornerCol);
        // Bottom-Left
        m_renderer->drawSpriteRGBA(m_whiteTexture, framePos + glm::vec2(0.0f, frameSize.y - cornerThick), glm::vec2(cornerLen, cornerThick), 0.0f, cornerCol);
        m_renderer->drawSpriteRGBA(m_whiteTexture, framePos + glm::vec2(0.0f, frameSize.y - cornerLen), glm::vec2(cornerThick, cornerLen), 0.0f, cornerCol);
        // Bottom-Right
        m_renderer->drawSpriteRGBA(m_whiteTexture, framePos + glm::vec2(frameSize.x - cornerLen, frameSize.y - cornerThick), glm::vec2(cornerLen, cornerThick), 0.0f, cornerCol);
        m_renderer->drawSpriteRGBA(m_whiteTexture, framePos + glm::vec2(frameSize.x - cornerThick, frameSize.y - cornerLen), glm::vec2(cornerThick, cornerLen), 0.0f, cornerCol);

        // Прицел центра (Center Crosshair)
        float crossLen = 12.0f;
        m_renderer->drawSpriteRGBA(m_whiteTexture, camCenter - glm::vec2(crossLen * 0.5f, 1.0f), glm::vec2(crossLen, 2.0f), 0.0f, cornerCol);
        m_renderer->drawSpriteRGBA(m_whiteTexture, camCenter - glm::vec2(1.0f, crossLen * 0.5f), glm::vec2(2.0f, crossLen), 0.0f, cornerCol);

        // Текстовая плашка сверху рамки
        std::string camTitle = m_isCameraConfigMode ? "CAMERA VIEWPORT (НАСТРОЙКА)" :
            (m_hudPreviewMode != HudPreviewMode::None ? "CAMERA VIEWPORT (БОЕВОЙ HUD)" : "CAMERA VIEWPORT (РУЧНАЯ)");
        float fScale = 0.55f;
        float tw = m_textRenderer ? m_textRenderer->CalculateTextWidth(camTitle, fScale) : 260.0f;
        float badgeW = tw + 20.0f;
        float badgeH = 24.0f;
        glm::vec2 badgePos(framePos.x + 10.0f, framePos.y - badgeH - 4.0f);
        m_renderer->drawSpriteRGBA(m_whiteTexture, badgePos, glm::vec2(badgeW, badgeH), 0.0f, glm::vec4(0.08f, 0.10f, 0.14f, 0.85f));
        m_renderer->drawSpriteRGBA(m_whiteTexture, badgePos, glm::vec2(3.0f, badgeH), 0.0f, cornerCol);
    }

    // 4.4. ПРЕДПРОСМОТР БОЕВОГО ИНТЕРФЕЙСА (HUD Preview Overlay в мировых координатах видоискателя)
    renderHudPreview();

    // 4.45. ДЕКОРАТИВНЫЕ ПРОП-ОБЪЕКТЫ (ПОД ЭМИТТЕРАМИ/ПАРТИКЛАМИ, ПОВЕРХ ТАЙЛОВ)
    if (m_grid) {
        DecorationRenderer::renderDecorations(m_renderer.get(), m_whiteTexture, m_decorations, *m_grid, m_selectedDecorationIndex, false);
    } else {
        DecorationRenderer::renderDecorations(m_renderer.get(), m_whiteTexture, m_decorations, m_selectedDecorationIndex, 64.0f, false);
    }

    // Предпросмотр установки декора под курсором на вкладке [4] Декор (для обычных декораций)
    if (m_toolbarUI.getCategory() == PaletteCategory::Decorations && !m_isDraggingDecoration &&
        m_currentBrush != EditorBrush::Eraser && m_currentBrush != EditorBrush::None && m_currentBrush != EditorBrush::DecorFog) {
        std::string previewType = "bush";
        if (m_currentBrush == EditorBrush::DecorGrass) previewType = "grass_tuft";
        else if (m_currentBrush == EditorBrush::DecorGrassField) previewType = "grass_field";
        else if (m_currentBrush == EditorBrush::DecorFlower) previewType = "flower";
        else if (m_currentBrush == EditorBrush::DecorStone) previewType = "stone";
        else if (m_currentBrush == EditorBrush::DecorHelmet) previewType = "helmet";
        else if (m_currentBrush == EditorBrush::DecorPickaxe) previewType = "pickaxe";
        else if (m_currentBrush == EditorBrush::DecorPuddle) previewType = "puddle";
        else if (m_currentBrush == EditorBrush::DecorCrack) previewType = "crack";

        glm::vec2 worldMouse = screenToWorld(m_mousePos);
        if (m_currentBrush == EditorBrush::DecorGrassField && m_grid && m_grid->getCellSize() > 0.001f) {
            glm::vec2 hoverTile = (worldMouse - m_grid->getOffset()) / m_grid->getCellSize();
            worldMouse = m_grid->getOffset() + (glm::floor(hoverTile) + glm::vec2(0.5f)) * m_grid->getCellSize();
        }
        float previewCellSize = m_grid ? m_grid->getCellSize() : 64.0f;
        DecorationRenderer::renderPreview(m_renderer.get(), m_whiteTexture, previewType, worldMouse, m_defaultDecorationRotation, 1.0f, 1.0f, 1.0f, 0.5f, previewCellSize);
    }

    // 4.5. РЕНДЕРИНГ МАРКЕРОВ ЭМИТТЕРОВ ЧАСТИЦ
    renderEmitters(m_renderer.get(), m_whiteTexture);

    // 4.6. ЖИВЫЕ ЧАСТИЦЫ ЭМИТТЕРОВ ОКРУЖЕНИЯ
    m_envParticleManager.render(m_renderer.get(), m_whiteTexture, *m_grid);

    // 4.7. ДЕКОРАЦИИ ВЕРХНЕГО СЛОЯ (ТУМАН / ТЬМА) — поверх частиц, эмиттеров, предпросмотра
    if (m_grid) {
        DecorationRenderer::renderDecorations(m_renderer.get(), m_whiteTexture, m_decorations, *m_grid, m_selectedDecorationIndex, true);
    } else {
        DecorationRenderer::renderDecorations(m_renderer.get(), m_whiteTexture, m_decorations, m_selectedDecorationIndex, 64.0f, true);
    }

    // Предпросмотр установки тумана под курсором на вкладке [4] Декор (поверх частиц)
    if (m_toolbarUI.getCategory() == PaletteCategory::Decorations && !m_isDraggingDecoration &&
        m_currentBrush == EditorBrush::DecorFog) {
        glm::vec2 worldMouse = screenToWorld(m_mousePos);
        float previewCellSize = m_grid ? m_grid->getCellSize() : 64.0f;
        DecorationRenderer::renderPreview(m_renderer.get(), m_whiteTexture, "fog", worldMouse, m_defaultDecorationRotation, 1.0f, 1.0f, 1.0f, 0.5f, previewCellSize);
    }

    m_renderer->endBatch();

    // 5. Восстанавливаем проекцию экрана для элементов пользовательского интерфейса (UI) и экранного текста
    m_renderer->setProjection(P_screen);
    if (m_textRenderer) m_textRenderer->updateProjection(P_screen);

    // 5.0. Иконки скрытия в бою (перечеркнутый глаз) для баз и спавнеров
    if (m_whiteTexture && m_grid) {
        float cellSize = m_grid->getCellSize();
        float screenCellSize = cellSize * m_zoom;

        m_renderer->beginBatch();

        for (const auto& b : m_bases) {
            if (!b.visibleInGame) {
                glm::vec2 worldPos = m_grid->gridToPixel(b.x, b.y);
                glm::vec2 screenTopLeft = worldToScreen(worldPos);

                if (screenTopLeft.x + screenCellSize < 0.0f || screenTopLeft.x > m_width ||
                    screenTopLeft.y + screenCellSize < 0.0f || screenTopLeft.y > m_height) {
                    continue;
                }

                float iconW = std::clamp(16.0f * m_zoom, 10.0f, 22.0f);
                float iconH = iconW * 0.70f;
                glm::vec2 iconPos(screenTopLeft.x + screenCellSize - iconW - 3.0f * m_zoom, screenTopLeft.y + 3.0f * m_zoom);

                m_renderer->drawSpriteRGBA(m_whiteTexture, iconPos, glm::vec2(iconW, iconH), 0.0f, glm::vec4(0.04f, 0.06f, 0.08f, 0.80f));
                m_renderer->drawSpriteRGBA(m_whiteTexture, iconPos + glm::vec2(iconW * 0.15f, iconH * 0.15f), glm::vec2(iconW * 0.70f, iconH * 0.70f), 0.0f, glm::vec4(0.85f, 0.90f, 0.95f, 0.75f));
                m_renderer->drawSpriteRGBA(m_whiteTexture, iconPos + glm::vec2(iconW * 0.35f, iconH * 0.28f), glm::vec2(iconW * 0.30f, iconH * 0.44f), 0.0f, glm::vec4(0.12f, 0.15f, 0.22f, 0.90f));
                m_renderer->drawSpriteRGBA(m_whiteTexture, iconPos + glm::vec2(iconW * 0.10f, iconH * 0.80f), glm::vec2(iconW * 0.95f, std::max(1.5f, 2.0f * m_zoom)), -35.0f, glm::vec4(1.0f, 0.32f, 0.32f, 0.95f));
            }
        }

        for (const auto& sp : m_spawners) {
            if (!sp.visibleInGame) {
                glm::vec2 worldPos = m_grid->gridToPixel(sp.pos.x, sp.pos.y);
                glm::vec2 screenTopLeft = worldToScreen(worldPos);

                if (screenTopLeft.x + screenCellSize < 0.0f || screenTopLeft.x > m_width ||
                    screenTopLeft.y + screenCellSize < 0.0f || screenTopLeft.y > m_height) {
                    continue;
                }

                float iconW = std::clamp(16.0f * m_zoom, 10.0f, 22.0f);
                float iconH = iconW * 0.70f;
                glm::vec2 iconPos(screenTopLeft.x + screenCellSize - iconW - 3.0f * m_zoom, screenTopLeft.y + 3.0f * m_zoom);

                m_renderer->drawSpriteRGBA(m_whiteTexture, iconPos, glm::vec2(iconW, iconH), 0.0f, glm::vec4(0.04f, 0.06f, 0.08f, 0.80f));
                m_renderer->drawSpriteRGBA(m_whiteTexture, iconPos + glm::vec2(iconW * 0.15f, iconH * 0.15f), glm::vec2(iconW * 0.70f, iconH * 0.70f), 0.0f, glm::vec4(0.85f, 0.90f, 0.95f, 0.75f));
                m_renderer->drawSpriteRGBA(m_whiteTexture, iconPos + glm::vec2(iconW * 0.35f, iconH * 0.28f), glm::vec2(iconW * 0.30f, iconH * 0.44f), 0.0f, glm::vec4(0.12f, 0.15f, 0.22f, 0.90f));
                m_renderer->drawSpriteRGBA(m_whiteTexture, iconPos + glm::vec2(iconW * 0.10f, iconH * 0.80f), glm::vec2(iconW * 0.95f, std::max(1.5f, 2.0f * m_zoom)), -35.0f, glm::vec4(1.0f, 0.32f, 0.32f, 0.95f));
            }
        }

        m_renderer->endBatch();
    }

    // 5.1. ТЕКСТ НА КЛЕТКАХ СЕТКИ (в экранных координатах с учетом зума и панорамирования)
    if (m_textRenderer && m_grid) {
        float cellSize = m_grid->getCellSize();
        float screenCellSize = cellSize * m_zoom;

        // Базы B#ID
        for (const auto& b : m_bases) {
            glm::vec2 worldPos = m_grid->gridToPixel(b.x, b.y);
            glm::vec2 screenTopLeft = worldToScreen(worldPos);

            if (screenTopLeft.x + screenCellSize < 0.0f || screenTopLeft.x > m_width ||
                screenTopLeft.y + screenCellSize < 0.0f || screenTopLeft.y > m_height) {
                continue;
            }

            std::string baseLabel = "B#" + std::to_string(b.id);
            float fontScale = std::clamp(0.70f * m_zoom, 0.25f, 0.75f);
            float tw = m_textRenderer->CalculateTextWidth(baseLabel, fontScale);
            float th = fontScale * 26.0f;
            float tx = screenTopLeft.x + (screenCellSize - tw) * 0.5f;
            float ty = screenTopLeft.y + (screenCellSize - th) * 0.5f + 1.0f;
            m_textRenderer->RenderText(baseLabel, tx, ty, fontScale, glm::vec3(0.05f, 0.08f, 0.15f));
        }

        // Спавнеры S>#ID / STOP / S:Auto
        for (size_t i = 0; i < m_spawners.size(); ++i) {
            const auto& sp = m_spawners[i];
            glm::vec2 worldPos = m_grid->gridToPixel(sp.pos.x, sp.pos.y);
            glm::vec2 screenTopLeft = worldToScreen(worldPos);

            if (screenTopLeft.x + screenCellSize < 0.0f || screenTopLeft.x > m_width ||
                screenTopLeft.y + screenCellSize < 0.0f || screenTopLeft.y > m_height) {
                continue;
            }

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

            float fontScale = std::clamp(0.62f * m_zoom, 0.22f, 0.70f);
            float tw = m_textRenderer->CalculateTextWidth(spText, fontScale);
            float th = fontScale * 26.0f;
            float tx = screenTopLeft.x + (screenCellSize - tw) * 0.5f;
            float ty = screenTopLeft.y + (screenCellSize - th) * 0.5f + 1.0f;
            m_textRenderer->RenderText(spText, tx, ty, fontScale, glm::vec3(0.05f, 0.08f, 0.12f));
        }

        // Маркеры вагонетки (START / END)
        if (!m_minecarts.empty()) {
            const auto& mc = m_minecarts[0];
            if (mc.hasStart()) {
                glm::vec2 worldPos = m_grid->gridToPixel(mc.start.x, mc.start.y);
                glm::vec2 screenTopLeft = worldToScreen(worldPos);
                if (screenTopLeft.x + screenCellSize >= 0.0f && screenTopLeft.x <= m_width &&
                    screenTopLeft.y + screenCellSize >= 0.0f && screenTopLeft.y <= m_height) {
                    std::string label = "START";
                    float baseScale = std::clamp(cellSize / 64.0f * 0.52f, 0.35f, 0.65f);
                    float fontScale = std::clamp(baseScale * m_zoom, 0.20f, 0.65f);
                    float tw = m_textRenderer->CalculateTextWidth(label, fontScale);
                    float th = fontScale * 26.0f;
                    float tx = screenTopLeft.x + (screenCellSize - tw) * 0.5f;
                    float ty = screenTopLeft.y + (screenCellSize - th) * 0.5f + 1.0f;
                    m_textRenderer->RenderText(label, tx, ty, fontScale, glm::vec3(0.15f, 0.10f, 0.03f));
                }
            }
            if (mc.hasEnd()) {
                glm::vec2 worldPos = m_grid->gridToPixel(mc.end.x, mc.end.y);
                glm::vec2 screenTopLeft = worldToScreen(worldPos);
                if (screenTopLeft.x + screenCellSize >= 0.0f && screenTopLeft.x <= m_width &&
                    screenTopLeft.y + screenCellSize >= 0.0f && screenTopLeft.y <= m_height) {
                    std::string label = "END";
                    float baseScale = std::clamp(cellSize / 64.0f * 0.56f, 0.35f, 0.68f);
                    float fontScale = std::clamp(baseScale * m_zoom, 0.20f, 0.68f);
                    float tw = m_textRenderer->CalculateTextWidth(label, fontScale);
                    float th = fontScale * 26.0f;
                    float tx = screenTopLeft.x + (screenCellSize - tw) * 0.5f;
                    float ty = screenTopLeft.y + (screenCellSize - th) * 0.5f + 1.0f;
                    m_textRenderer->RenderText(label, tx, ty, fontScale, glm::vec3(1.0f, 0.95f, 0.95f));
                }
            }
        }

        // Подписи эмиттеров частиц (ПАР, КАП, ИСК, ДЫМ и градус)
        for (const auto& em : m_emitters) {
            glm::vec2 worldPos = m_grid->getOffset() + em.tilePos * cellSize;
            glm::vec2 screenCenter = worldToScreen(worldPos);
            glm::vec2 screenTopLeft = screenCenter - glm::vec2(screenCellSize * 0.5f);
            if (screenTopLeft.x + screenCellSize >= 0.0f && screenTopLeft.x <= m_width &&
                screenTopLeft.y + screenCellSize >= 0.0f && screenTopLeft.y <= m_height) {
                std::string tag = "ПАР";
                if (em.type == "water_drip") tag = "КАП";
                else if (em.type == "sparks") tag = "ИСК";
                else if (em.type == "smoke") tag = "ДЫМ";
                else if (em.type == "fog") tag = "ТУМ";

                float baseScale = std::clamp(cellSize / 64.0f * 0.42f, 0.28f, 0.55f);
                float fontScale = std::clamp(baseScale * m_zoom, 0.18f, 0.55f);
                float tw = m_textRenderer->CalculateTextWidth(tag, fontScale);
                float th = fontScale * 26.0f;
                float tx = screenTopLeft.x + (screenCellSize - tw) * 0.5f;
                float ty = screenTopLeft.y + (screenCellSize - th) * 0.5f - 4.0f * m_zoom;
                m_textRenderer->RenderText(tag, tx, ty, fontScale, glm::vec3(1.0f, 1.0f, 1.0f));

                std::string degStr = std::to_string(static_cast<int>(em.angleDeg)) + "°";
                float degScale = fontScale * 0.82f;
                float dtw = m_textRenderer->CalculateTextWidth(degStr, degScale);
                float dtx = screenTopLeft.x + (screenCellSize - dtw) * 0.5f;
                float dty = ty + th + 1.0f;
                m_textRenderer->RenderText(degStr, dtx, dty, degScale, glm::vec3(0.85f, 0.95f, 1.0f));
            }
        }

        // Подпись видоискателя камеры (Camera Viewport Badge)
        if (m_isCameraConfigMode || m_cameraSettings.isCustom || m_hudPreviewMode != HudPreviewMode::None) {
            float camZoom = std::clamp(m_cameraSettings.zoom, 0.4f, 2.5f);
            glm::vec2 camCenter = getWorldCamCenter();
            glm::vec2 frameSize = getCameraViewportSize();
            glm::vec2 framePos = camCenter - frameSize * 0.5f;
            glm::vec2 badgePos(framePos.x + 10.0f, framePos.y - 24.0f - 4.0f);
            glm::vec2 screenBadgePos = worldToScreen(badgePos);

            std::string camTitle = m_isCameraConfigMode ? "CAMERA VIEWPORT (НАСТРОЙКА)" :
                (m_hudPreviewMode != HudPreviewMode::None ? "CAMERA VIEWPORT (БОЕВОЙ HUD)" : "CAMERA VIEWPORT (РУЧНАЯ)");
            float badgeFontScale = std::clamp(0.55f * m_zoom, 0.28f, 0.70f);
            m_textRenderer->RenderText(camTitle, screenBadgePos.x + 8.0f * m_zoom, screenBadgePos.y + 3.0f * m_zoom, badgeFontScale, glm::vec3(0.95f, 0.95f, 1.0f));
        }

        // Текстовые метки боевого интерфейса внутри видоискателя
        renderHudPreviewText();
    }

    // ВЕРХНИЙ ХЕДЕР-БАР И НИЖНИЙ ТУЛБАР (EditorToolbarUI)
    m_renderer->beginBatch();
    m_toolbarUI.render(m_renderer.get(), m_textRenderer, m_whiteTexture, buildEditorContext(), m_mousePos);
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
    if (m_exitModal.isOpen()) {
        m_renderer->beginBatch();
        m_exitModal.render(m_renderer.get(), m_textRenderer, m_whiteTexture,
                           static_cast<float>(m_width), static_cast<float>(m_height), m_mousePos);
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

    // 12.2. МОДАЛЬНОЕ ОКНО СПРАВКИ ПО УПРАВЛЕНИЮ (если активно)
    if (m_helpModal.isOpen()) {
        m_renderer->beginBatch();
        m_helpModal.render(m_renderer.get(), m_textRenderer, m_whiteTexture, m_width, m_height);
        m_renderer->endBatch();
    }

    // 12.2b. МОДАЛЬНОЕ ОКНО НАСТРОЕК УРОВНЯ И ЭКОНОМИКИ (если активно)
    if (m_levelSettingsModal.isOpen()) {
        m_renderer->beginBatch();
        m_levelSettingsModal.render(m_renderer.get(), m_textRenderer, m_whiteTexture, m_width, m_height, m_mousePos);
        m_renderer->endBatch();
    }

    // 12.3. ВСПЛЫВАЮЩАЯ ПАНЕЛЬ НАСТРОЕК ВЫБРАННОГО ЭМИТТЕРА ПАРТИКЛОВ
    if (m_selectedEmitterIndex >= 0 && m_selectedEmitterIndex < static_cast<int>(m_emitters.size())) {
        glm::vec2 emWorld = m_grid ? (m_grid->getOffset() + m_emitters[m_selectedEmitterIndex].tilePos * m_grid->getCellSize()) : glm::vec2(0.0f);
        glm::vec2 emScreen = worldToScreen(emWorld);

        m_renderer->beginBatch();
        m_emitterInspector.render(m_renderer.get(), m_textRenderer, m_whiteTexture,
                                  m_emitters[m_selectedEmitterIndex], m_selectedEmitterIndex, emScreen,
                                  static_cast<float>(m_width), static_cast<float>(m_height),
                                  getTopBarHeight(), getBottomDockHeight(), m_mousePos);
        m_renderer->endBatch();
    }

    // 12.4. ВСПЛЫВАЮЩАЯ ПАНЕЛЬ НАСТРОЕК ВЫБРАННОГО ДЕКОРА
    if (m_selectedDecorationIndex >= 0 && m_selectedDecorationIndex < static_cast<int>(m_decorations.size())) {
        const auto& dec = m_decorations[m_selectedDecorationIndex];
        glm::vec2 decActualWorld = m_grid ? (m_grid->getOffset() + dec.tilePos * m_grid->getCellSize()) : dec.worldPos;
        glm::vec2 decScreen = worldToScreen(decActualWorld);

        m_renderer->beginBatch();
        m_decorInspector.render(m_renderer.get(), m_textRenderer, m_whiteTexture,
                                dec, m_selectedDecorationIndex, decScreen,
                                static_cast<float>(m_width), static_cast<float>(m_height),
                                getTopBarHeight(), getBottomDockHeight(), m_mousePos);
        m_renderer->endBatch();
    }

    // 12.5. ВСПЛЫВАЮЩАЯ ПАНЕЛЬ НАСТРОЕК ВЫБРАННОГО СПАВНЕРА ИЛИ БАЗЫ (Entity Inspector)
    if (m_selectedEntityIndex >= 0) {
        if (m_selectedEntityType == EditorEntityType::Spawner && m_selectedEntityIndex < static_cast<int>(m_spawners.size())) {
            glm::vec2 entWorld = m_grid ? (m_grid->gridToPixel(m_spawners[m_selectedEntityIndex].pos.x, m_spawners[m_selectedEntityIndex].pos.y) + glm::vec2(m_grid->getCellSize() * 0.5f)) : glm::vec2(0.0f);
            glm::vec2 entScreen = worldToScreen(entWorld);

            m_renderer->beginBatch();
            m_entityInspector.render(m_renderer.get(), m_textRenderer, m_whiteTexture,
                                     m_spawners[m_selectedEntityIndex], m_selectedEntityIndex, entScreen,
                                     static_cast<float>(m_width), static_cast<float>(m_height),
                                     getTopBarHeight(), getBottomDockHeight(), m_mousePos);
            m_renderer->endBatch();
        } else if (m_selectedEntityType == EditorEntityType::Base && m_selectedEntityIndex < static_cast<int>(m_bases.size())) {
            glm::vec2 entWorld = m_grid ? (m_grid->gridToPixel(m_bases[m_selectedEntityIndex].x, m_bases[m_selectedEntityIndex].y) + glm::vec2(m_grid->getCellSize() * 0.5f)) : glm::vec2(0.0f);
            glm::vec2 entScreen = worldToScreen(entWorld);

            m_renderer->beginBatch();
            m_entityInspector.render(m_renderer.get(), m_textRenderer, m_whiteTexture,
                                     m_bases[m_selectedEntityIndex], m_selectedEntityIndex, entScreen,
                                     static_cast<float>(m_width), static_cast<float>(m_height),
                                     getTopBarHeight(), getBottomDockHeight(), m_mousePos);
            m_renderer->endBatch();
        }
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

glm::vec2 MapEditorState::getDefaultWorldCenter() const {
    if (m_grid) {
        return m_grid->getOffset() + glm::vec2(m_gridWidth, m_gridHeight) * m_grid->getCellSize() * 0.5f;
    }
    return glm::vec2(static_cast<float>(m_width) * 0.5f, static_cast<float>(m_height) * 0.5f);
}

glm::vec2 MapEditorState::getWorldCamCenter() const {
    if (!m_grid) return getDefaultWorldCenter();
    glm::vec2 targetTile = m_cameraSettings.targetTile;
    if (targetTile.x < 0.0f || targetTile.y < 0.0f) {
        targetTile = glm::vec2(static_cast<float>(m_gridWidth) * 0.5f,
                               static_cast<float>(m_gridHeight) * 0.5f);
    }
    return m_grid->getOffset() + targetTile * m_grid->getCellSize();
}

glm::vec2 MapEditorState::getCameraViewportSize() const {
    float camZoom = std::clamp(m_cameraSettings.zoom, 0.4f, 2.5f);
    return glm::vec2(static_cast<float>(m_width) / camZoom,
                     static_cast<float>(m_height) / camZoom);
}

glm::vec2 MapEditorState::worldPixelToTile(glm::vec2 worldPos) const {
    if (!m_grid) return glm::vec2(0.0f);
    float cellSize = m_grid->getCellSize();
    if (cellSize <= 0.001f) return glm::vec2(0.0f);
    return (worldPos - m_grid->getOffset()) / cellSize;
}

glm::vec2 MapEditorState::tileToWorldPixel(glm::vec2 tilePos) const {
    if (!m_grid) return tilePos;
    return m_grid->getOffset() + tilePos * m_grid->getCellSize();
}

void MapEditorState::resize(int width, int height) {
    glm::vec2 oldScreenCenter = glm::vec2(static_cast<float>(m_width) * 0.5f, static_cast<float>(m_height) * 0.5f);
    glm::vec2 worldFocus = screenToWorld(oldScreenCenter);

    m_width = width;
    m_height = height;
    updateGridDimensions();
    updateButtonLayout();

    // Сохраняем фокус камеры на том же мировом объекте/точке при изменении размера окна
    glm::vec2 newScreenCenter = glm::vec2(static_cast<float>(m_width) * 0.5f, static_cast<float>(m_height) * 0.5f);
    m_cameraPan = newScreenCenter - worldFocus * m_zoom;
}















void MapEditorState::openExitModal() {
    if (!m_isDirty) {
        std::cout << "[MapEditor] openExitModal: Map is not dirty (unmodified), exiting directly to origin." << std::endl;
        returnToOrigin();
        return;
    }
    std::cout << "[MapEditor] openExitModal: exit confirmation dialog opened" << std::endl;
    m_exitModal.open(true);
    m_suppressPlacementUntilRelease = true;
    m_suppressClickUntilRelease = true;
}

void MapEditorState::closeExitModal() {
    std::cout << "[MapEditor] closeExitModal: exit dialog closed (staying in editor)" << std::endl;
    m_exitModal.close();
    m_suppressPlacementUntilRelease = true;
    m_keyEscPressedLastFrame = true;
}

void MapEditorState::renderHudPreview() {
    if (m_hudPreviewMode == HudPreviewMode::None) return;

    float previewScale = 1.0f;
    if (m_hudPreviewMode == HudPreviewMode::Scale100) {
        previewScale = 1.0f;
    } else if (m_hudPreviewMode == HudPreviewMode::Scale125) {
        previewScale = 1.25f;
    } else if (m_hudPreviewMode == HudPreviewMode::Scale150) {
        previewScale = 1.50f;
    }

    // Параметры видоискателя стартовой камеры
    float camZoom = std::clamp(m_cameraSettings.zoom, 0.4f, 2.5f);
    glm::vec2 camCenter = getWorldCamCenter();
    glm::vec2 frameSize = getCameraViewportSize();
    float frameW = frameSize.x;
    float frameH = frameSize.y;
    glm::vec2 framePos = camCenter - glm::vec2(frameW * 0.5f, frameH * 0.5f);

    // Масштаб элементов HUD в мировых координатах видоискателя
    float s = previewScale / camZoom;

    // Вспомогательные функции для рисования пунктирных линий (контуров безопасных зон)
    auto drawDashedHLine = [&](float x1, float x2, float y, glm::vec4 col) {
        float dashLen = 8.0f * (1.0f / camZoom);
        float gapLen = 6.0f * (1.0f / camZoom);
        float lineThick = 2.0f * (1.0f / camZoom);
        float curX = x1;
        while (curX < x2) {
            float curLen = std::min(dashLen, x2 - curX);
            m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(curX, y - lineThick * 0.5f), glm::vec2(curLen, lineThick), 0.0f, col);
            curX += dashLen + gapLen;
        }
    };

    auto drawDashedVLine = [&](float x, float y1, float y2, glm::vec4 col) {
        float dashLen = 8.0f * (1.0f / camZoom);
        float gapLen = 6.0f * (1.0f / camZoom);
        float lineThick = 2.0f * (1.0f / camZoom);
        float curY = y1;
        while (curY < y2) {
            float curLen = std::min(dashLen, y2 - curY);
            m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(x - lineThick * 0.5f, curY), glm::vec2(lineThick, curLen), 0.0f, col);
            curY += dashLen + gapLen;
        }
    };

    auto drawDashedRect = [&](glm::vec2 pos, glm::vec2 size, glm::vec4 col) {
        drawDashedHLine(pos.x, pos.x + size.x, pos.y, col);
        drawDashedHLine(pos.x, pos.x + size.x, pos.y + size.y, col);
        drawDashedVLine(pos.x, pos.y, pos.y + size.y, col);
        drawDashedVLine(pos.x + size.x, pos.y, pos.y + size.y, col);
    };

    glm::vec4 warnColor(1.0f, 0.78f, 0.15f, 0.95f);

    // =========================================================================
    // 1. ВЕРХНИЙ ПРАВЫЙ БЛОК: Панель ресурсов, тайм-контроль, старт волны
    // Привязка к правому верхнему углу белой рамки видоискателя
    // =========================================================================
    float hudPanelW = TimeControlUI::PANEL_W * s;
    float hudPanelH = TimeControlUI::PANEL_H * s;
    float hudPanelX = framePos.x + frameW - hudPanelW - TimeControlUI::MARGIN_RIGHT * s;
    float hudPanelY = framePos.y + TimeControlUI::TOP_Y * s;

    // Полупрозрачный силуэт подложки верхнего правого HUD
    m_renderer->drawSpriteRGBA(m_whiteTexture,
        glm::vec2(hudPanelX, hudPanelY),
        glm::vec2(hudPanelW, hudPanelH),
        0.0f,
        glm::vec4(0.08f, 0.10f, 0.15f, 0.75f));

    // Верхняя акцентная полоска
    m_renderer->drawSpriteRGBA(m_whiteTexture,
        glm::vec2(hudPanelX, hudPanelY),
        glm::vec2(hudPanelW, 2.0f * s),
        0.0f,
        glm::vec4(0.40f, 0.75f, 1.0f, 0.9f));

    // Индикаторы статов (HP, Gold, Wave)
    float padX = TimeControlUI::PANEL_PAD_X * s;
    float padY = TimeControlUI::PANEL_PAD_Y * s;
    float statsInnerW = hudPanelW - padX * 2.0f;
    float badgeGap = 4.0f * s;
    float b0w = std::floor((statsInnerW - badgeGap * 2.0f) * 0.28f);
    float b1w = std::floor((statsInnerW - badgeGap * 2.0f) * 0.36f);
    float b2w = (statsInnerW - badgeGap * 2.0f) - b0w - b1w;
    float statsH = TimeControlUI::STATS_ROW_H * s;

    // HP
    m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(hudPanelX + padX, hudPanelY + padY), glm::vec2(b0w, statsH), 0.0f, glm::vec4(0.14f, 0.17f, 0.24f, 0.85f));
    m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(hudPanelX + padX, hudPanelY + padY), glm::vec2(3.0f * s, statsH), 0.0f, glm::vec4(0.92f, 0.28f, 0.28f, 1.0f));

    // Gold
    m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(hudPanelX + padX + b0w + badgeGap, hudPanelY + padY), glm::vec2(b1w, statsH), 0.0f, glm::vec4(0.14f, 0.17f, 0.24f, 0.85f));
    m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(hudPanelX + padX + b0w + badgeGap, hudPanelY + padY), glm::vec2(3.0f * s, statsH), 0.0f, glm::vec4(1.0f, 0.82f, 0.22f, 1.0f));

    // Wave
    m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(hudPanelX + padX + b0w + badgeGap + b1w + badgeGap, hudPanelY + padY), glm::vec2(b2w, statsH), 0.0f, glm::vec4(0.14f, 0.17f, 0.24f, 0.85f));
    m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(hudPanelX + padX + b0w + badgeGap + b1w + badgeGap, hudPanelY + padY), glm::vec2(3.0f * s, statsH), 0.0f, glm::vec4(0.35f, 0.75f, 1.0f, 1.0f));

    // Кнопки управления временем (||, 1x, 2x, 4x)
    float btnY = hudPanelY + padY + statsH + TimeControlUI::ROW_GAP * s;
    float timeBtnW = TimeControlUI::TIME_BTN_W * s;
    float btnH = TimeControlUI::BTN_H * s;
    float btnGap = TimeControlUI::BTN_GAP * s;
    for (int i = 0; i < 4; ++i) {
        float bx = hudPanelX + padX + i * (timeBtnW + btnGap);
        m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(bx, btnY), glm::vec2(timeBtnW, btnH), 0.0f, glm::vec4(0.15f, 0.18f, 0.26f, 0.85f));
    }

    // Кнопка вызова волны (Start / Call Wave)
    float waveBtnX = hudPanelX + padX + 4.0f * (timeBtnW + btnGap);
    float waveBtnW = std::max(0.0f, (hudPanelX + hudPanelW - padX) - waveBtnX);
    m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(waveBtnX, btnY), glm::vec2(waveBtnW, btnH), 0.0f, glm::vec4(0.18f, 0.45f, 0.24f, 0.90f));

    // =========================================================================
    // 2. ВЕРХНЯЯ ЛЕВАЯ КНОПКА: [ В редактор / Меню ]
    // Привязка к левому верхнему углу белой рамки видоискателя
    // =========================================================================
    float topLeftBtnX = framePos.x + 20.0f * s;
    float topLeftBtnY = framePos.y + 20.0f * s;
    float topLeftBtnW = 150.0f * s;
    float topLeftBtnH = 40.0f * s;

    m_renderer->drawSpriteRGBA(m_whiteTexture,
        glm::vec2(topLeftBtnX, topLeftBtnY),
        glm::vec2(topLeftBtnW, topLeftBtnH),
        0.0f,
        glm::vec4(0.11f, 0.14f, 0.18f, 0.75f));
    m_renderer->drawSpriteRGBA(m_whiteTexture,
        glm::vec2(topLeftBtnX, topLeftBtnY),
        glm::vec2(4.0f * s, topLeftBtnH),
        0.0f,
        glm::vec4(1.0f, 0.80f, 0.20f, 0.9f));

    // =========================================================================
    // 3. НИЖНЯЯ ПАНЕЛЬ СЛОТОВ БАШЕН (BuildPanel)
    // Привязка строго по центру нижней кромки белой рамки видоискателя
    // =========================================================================
    size_t towerCount = 3;
    float basePanelW = (Buildpanel::DOCK_PADDING * 2.0f) + (towerCount * Buildpanel::CARD_WIDTH) + ((towerCount - 1) * Buildpanel::DOCK_GAP);
    float dockW = basePanelW * s;
    float dockH = Buildpanel::UI_PANEL_HEIGHT * s;
    float dockX = framePos.x + (frameW - dockW) * 0.5f;
    float bottomMargin = Buildpanel::DOCK_BOTTOM_MARGIN * s;
    float dockY = framePos.y + frameH - dockH - bottomMargin;

    // Плавающий остров дока (подложка под карточки)
    m_renderer->drawSpriteRGBA(m_whiteTexture,
        glm::vec2(dockX, dockY),
        glm::vec2(dockW, dockH),
        0.0f,
        glm::vec4(0.06f, 0.08f, 0.11f, 0.75f));

    // Тонкая кайма острова
    float islandBorderT = 1.5f * s;
    glm::vec4 islandBorder(0.22f, 0.28f, 0.38f, 0.65f);
    m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(dockX, dockY), glm::vec2(dockW, islandBorderT), 0.0f, islandBorder);
    m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(dockX, dockY + dockH - islandBorderT), glm::vec2(dockW, islandBorderT), 0.0f, islandBorder);
    m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(dockX, dockY), glm::vec2(islandBorderT, dockH), 0.0f, islandBorder);
    m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(dockX + dockW - islandBorderT, dockY), glm::vec2(islandBorderT, dockH), 0.0f, islandBorder);

    // Карточки башен внутри BuildPanel
    for (size_t i = 0; i < towerCount; ++i) {
        float cardX = dockX + (Buildpanel::DOCK_PADDING * s) + i * ((Buildpanel::CARD_WIDTH + Buildpanel::DOCK_GAP) * s);
        float cardY = dockY + (Buildpanel::DOCK_PADDING * s);
        float cardW = Buildpanel::CARD_WIDTH * s;
        float cardH = Buildpanel::CARD_HEIGHT * s;

        // Плашка карточки (RGBA 0.1, 0.12, 0.16, 0.85)
        m_renderer->drawSpriteRGBA(m_whiteTexture,
            glm::vec2(cardX, cardY),
            glm::vec2(cardW, cardH),
            0.0f,
            glm::vec4(0.10f, 0.12f, 0.16f, 0.85f));

        // Тонкая кайма карточки
        float cardBorderT = 1.5f * s;
        glm::vec4 cardBorder(0.24f, 0.32f, 0.44f, 0.85f);
        m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(cardX, cardY), glm::vec2(cardW, cardBorderT), 0.0f, cardBorder);
        m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(cardX, cardY + cardH - cardBorderT), glm::vec2(cardW, cardBorderT), 0.0f, cardBorder);
        m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(cardX, cardY), glm::vec2(cardBorderT, cardH), 0.0f, cardBorder);
        m_renderer->drawSpriteRGBA(m_whiteTexture, glm::vec2(cardX + cardW - cardBorderT, cardY), glm::vec2(cardBorderT, cardH), 0.0f, cardBorder);

        // Хоткей карточки
        m_renderer->drawSpriteRGBA(m_whiteTexture,
            glm::vec2(cardX + 4.0f * s, cardY + 4.0f * s),
            glm::vec2(14.0f * s, 8.0f * s),
            0.0f,
            glm::vec4(0.85f, 0.70f, 0.25f, 0.70f));

        // Силуэт иконки башни
        float iconSize = Buildpanel::UI_ICON_SIZE * s;
        float iconX = cardX + (cardW - iconSize) * 0.5f;
        float iconY = cardY + 18.0f * s;
        m_renderer->drawSpriteRGBA(m_whiteTexture,
            glm::vec2(iconX, iconY),
            glm::vec2(iconSize, iconSize),
            0.0f,
            glm::vec4(0.18f, 0.22f, 0.30f, 0.85f));

        // Силуэт ценника башни
        m_renderer->drawSpriteRGBA(m_whiteTexture,
            glm::vec2(cardX + 10.0f * s, cardY + cardH - 16.0f * s),
            glm::vec2(cardW - 20.0f * s, 10.0f * s),
            0.0f,
            glm::vec4(0.25f, 0.75f, 0.35f, 0.85f));
    }

    // =========================================================================
    // 4. КОНТУРЫ БЕЗОПАСНЫХ ЗОН (SAFE AREA): Желтый пунктир вокруг реальных блоков
    // =========================================================================
    drawDashedRect(glm::vec2(hudPanelX, hudPanelY), glm::vec2(hudPanelW, hudPanelH), warnColor);
    drawDashedRect(glm::vec2(topLeftBtnX, topLeftBtnY), glm::vec2(topLeftBtnW, topLeftBtnH), warnColor);
    drawDashedRect(glm::vec2(dockX, dockY), glm::vec2(dockW, dockH), warnColor);
}

void MapEditorState::renderHudPreviewText() {
    if (m_hudPreviewMode == HudPreviewMode::None || !m_textRenderer) return;

    float previewScale = 1.0f;
    std::string scaleText = "100%";
    if (m_hudPreviewMode == HudPreviewMode::Scale100) {
        previewScale = 1.0f;
        scaleText = "100%";
    } else if (m_hudPreviewMode == HudPreviewMode::Scale125) {
        previewScale = 1.25f;
        scaleText = "125%";
    } else if (m_hudPreviewMode == HudPreviewMode::Scale150) {
        previewScale = 1.50f;
        scaleText = "150%";
    }

    float camZoom = std::clamp(m_cameraSettings.zoom, 0.4f, 2.5f);
    glm::vec2 camCenter = getWorldCamCenter();
    glm::vec2 frameSize = getCameraViewportSize();
    float frameW = frameSize.x;
    float frameH = frameSize.y;
    glm::vec2 framePos = camCenter - glm::vec2(frameW * 0.5f, frameH * 0.5f);
    float s = previewScale / camZoom;

    // Мировые координаты элементов
    float hudPanelW = TimeControlUI::PANEL_W * s;
    float hudPanelH = TimeControlUI::PANEL_H * s;
    float hudPanelX = framePos.x + frameW - hudPanelW - TimeControlUI::MARGIN_RIGHT * s;
    float hudPanelY = framePos.y + TimeControlUI::TOP_Y * s;

    float padX = TimeControlUI::PANEL_PAD_X * s;
    float padY = TimeControlUI::PANEL_PAD_Y * s;
    float statsH = TimeControlUI::STATS_ROW_H * s;
    float btnY = hudPanelY + padY + statsH + TimeControlUI::ROW_GAP * s;
    float timeBtnW = TimeControlUI::TIME_BTN_W * s;
    float btnGap = TimeControlUI::BTN_GAP * s;
    float waveBtnX = hudPanelX + padX + 4.0f * (timeBtnW + btnGap);

    float topLeftBtnX = framePos.x + 20.0f * s;
    float topLeftBtnY = framePos.y + 20.0f * s;

    size_t towerCount = 3;
    float basePanelW = (Buildpanel::DOCK_PADDING * 2.0f) + (towerCount * Buildpanel::CARD_WIDTH) + ((towerCount - 1) * Buildpanel::DOCK_GAP);
    float dockW = basePanelW * s;
    float dockH = Buildpanel::UI_PANEL_HEIGHT * s;
    float dockX = framePos.x + (frameW - dockW) * 0.5f;
    float bottomMargin = Buildpanel::DOCK_BOTTOM_MARGIN * s;
    float dockY = framePos.y + frameH - dockH - bottomMargin;

    // Перевод мировых координат в экранные с учетом зума редактора
    glm::vec2 sTopLeft = worldToScreen(glm::vec2(topLeftBtnX, topLeftBtnY));
    glm::vec2 sHudPanel = worldToScreen(glm::vec2(hudPanelX, hudPanelY));
    glm::vec2 sWaveBtn = worldToScreen(glm::vec2(waveBtnX, btnY));
    glm::vec2 sDock = worldToScreen(glm::vec2(dockX, dockY));

    float fontScale = std::clamp(0.40f * previewScale * (m_zoom / camZoom), 0.22f, 0.70f);

    // Метка верхней левой кнопки
    m_textRenderer->RenderText("[ В РЕДАКТОР ]", sTopLeft.x + 16.0f * s * m_zoom, sTopLeft.y + 12.0f * s * m_zoom, fontScale, glm::vec3(0.95f, 0.95f, 1.0f));

    // Метка верхнего правого HUD
    std::string topLabel = "БОЕВОЙ HUD (" + scaleText + ")";
    float tlw = m_textRenderer->CalculateTextWidth(topLabel, fontScale);
    float screenHudPanelW = hudPanelW * m_zoom;
    m_textRenderer->RenderText(topLabel, sHudPanel.x + (screenHudPanelW - tlw) * 0.5f, sHudPanel.y + hudPanelH * m_zoom + 6.0f * s * m_zoom, fontScale, glm::vec3(1.0f, 0.82f, 0.20f));

    // Метка кнопки старта волны
    float miniFont = std::clamp(0.32f * previewScale * (m_zoom / camZoom), 0.18f, 0.55f);
    m_textRenderer->RenderText("СТАРТ", sWaveBtn.x + 8.0f * s * m_zoom, sWaveBtn.y + 8.0f * s * m_zoom, miniFont, glm::vec3(0.95f, 1.0f, 0.95f));

    // Метка нижнего центрального дока строительства
    std::string dockLabel = "ДОК БАШЕН (" + scaleText + ")";
    float dlw = m_textRenderer->CalculateTextWidth(dockLabel, fontScale);
    float screenDockW = dockW * m_zoom;
    m_textRenderer->RenderText(dockLabel, sDock.x + (screenDockW - dlw) * 0.5f, sDock.y - (fontScale * 26.0f) - 4.0f * s * m_zoom, fontScale, glm::vec3(1.0f, 0.82f, 0.20f));
}

void MapEditorState::renderEmitters(SpriteRenderer* renderer, std::shared_ptr<Texture2D> texture) {
    if (!renderer || !texture || !m_grid || m_emitters.empty()) return;

    float cellSize = m_grid->getCellSize();

    for (size_t i = 0; i < m_emitters.size(); ++i) {
        const auto& em = m_emitters[i];
        bool isSelected = (static_cast<int>(i) == m_selectedEmitterIndex);
        glm::vec2 center = m_grid->getOffset() + em.tilePos * cellSize;

        // Цветовая схема по пресету
        glm::vec4 bgCol(0.18f, 0.40f, 0.60f, 0.85f);
        glm::vec4 borderCol(0.65f, 0.90f, 1.0f, 1.0f);
        glm::vec4 arrowCol(0.85f, 0.96f, 1.0f, 1.0f);

        if (em.type == "water_drip") {
            bgCol = glm::vec4(0.12f, 0.28f, 0.55f, 0.85f);
            borderCol = glm::vec4(0.40f, 0.75f, 1.0f, 1.0f);
            arrowCol = glm::vec4(0.60f, 0.88f, 1.0f, 1.0f);
        } else if (em.type == "sparks") {
            bgCol = glm::vec4(0.55f, 0.30f, 0.10f, 0.85f);
            borderCol = glm::vec4(1.0f, 0.85f, 0.25f, 1.0f);
            arrowCol = glm::vec4(1.0f, 0.95f, 0.50f, 1.0f);
        } else if (em.type == "smoke") {
            bgCol = glm::vec4(0.24f, 0.24f, 0.28f, 0.85f);
            borderCol = glm::vec4(0.85f, 0.85f, 0.90f, 1.0f);
            arrowCol = glm::vec4(0.95f, 0.95f, 1.0f, 1.0f);
        } else if (em.type == "fog") {
            bgCol = glm::vec4(0.20f, 0.28f, 0.25f, 0.85f);
            borderCol = glm::vec4(0.65f, 0.85f, 0.75f, 1.0f);
            arrowCol = glm::vec4(0.80f, 0.95f, 0.85f, 1.0f);
        }

        if (isSelected) {
            borderCol = glm::vec4(1.0f, 0.92f, 0.25f, 1.0f);
            arrowCol = glm::vec4(1.0f, 1.0f, 0.60f, 1.0f);
        }

        // Базовый круглый/квадратный значок в точке эмиттера
        float badgeSize = std::max(18.0f, cellSize * 0.44f);
        glm::vec2 badgeTopLeft = center - glm::vec2(badgeSize * 0.5f);

        // Внешнее свечение при выделении
        if (isSelected) {
            float haloSize = badgeSize + 8.0f;
            glm::vec2 haloTopLeft = center - glm::vec2(haloSize * 0.5f);
            float haloT = 2.0f;
            glm::vec4 haloCol(1.0f, 0.85f, 0.20f, 0.90f);
            renderer->drawSpriteRGBA(texture, haloTopLeft, glm::vec2(haloSize, haloT), 0.0f, haloCol);
            renderer->drawSpriteRGBA(texture, haloTopLeft + glm::vec2(0.0f, haloSize - haloT), glm::vec2(haloSize, haloT), 0.0f, haloCol);
            renderer->drawSpriteRGBA(texture, haloTopLeft, glm::vec2(haloT, haloSize), 0.0f, haloCol);
            renderer->drawSpriteRGBA(texture, haloTopLeft + glm::vec2(haloSize - haloT, 0.0f), glm::vec2(haloT, haloSize), 0.0f, haloCol);
        }

        // Тень значка
        renderer->drawSpriteRGBA(texture, badgeTopLeft + glm::vec2(1.5f), glm::vec2(badgeSize), 0.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.45f));
        // Тело значка
        renderer->drawSpriteRGBA(texture, badgeTopLeft, glm::vec2(badgeSize), 0.0f, bgCol);
        // Кайма
        float borderT = isSelected ? 3.0f : 2.0f;
        renderer->drawSpriteRGBA(texture, badgeTopLeft, glm::vec2(badgeSize, borderT), 0.0f, borderCol);
        renderer->drawSpriteRGBA(texture, badgeTopLeft + glm::vec2(0.0f, badgeSize - borderT), glm::vec2(badgeSize, borderT), 0.0f, borderCol);
        renderer->drawSpriteRGBA(texture, badgeTopLeft, glm::vec2(borderT, badgeSize), 0.0f, borderCol);
        renderer->drawSpriteRGBA(texture, badgeTopLeft + glm::vec2(badgeSize - borderT, 0.0f), glm::vec2(borderT, badgeSize), 0.0f, borderCol);

        // Стрелка направления струи angleDeg
        float rad = glm::radians(em.angleDeg);
        glm::vec2 dir(std::cos(rad), std::sin(rad));
        glm::vec2 perp(-dir.y, dir.x);

        float arrowLen = badgeSize * 0.75f;
        glm::vec2 tip = center + dir * arrowLen;

        // Линия стрелки (сегментами)
        for (float t = 0.2f; t <= 1.0f; t += 0.2f) {
            glm::vec2 pt = center + dir * (arrowLen * t);
            renderer->drawSpriteRGBA(texture, pt - glm::vec2(1.5f), glm::vec2(3.0f), 0.0f, arrowCol);
        }
        // Наконечник стрелки
        renderer->drawSpriteRGBA(texture, tip - glm::vec2(2.5f), glm::vec2(5.0f), 0.0f, borderCol);
        glm::vec2 wing1 = tip - dir * 5.0f + perp * 4.0f;
        glm::vec2 wing2 = tip - dir * 5.0f - perp * 4.0f;
        renderer->drawSpriteRGBA(texture, wing1 - glm::vec2(1.5f), glm::vec2(3.0f), 0.0f, borderCol);
        renderer->drawSpriteRGBA(texture, wing2 - glm::vec2(1.5f), glm::vec2(3.0f), 0.0f, borderCol);
    }
}
