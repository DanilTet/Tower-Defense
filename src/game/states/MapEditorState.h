#pragma once
#include "IGameState.h"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include "../core/LevelManager.h"
#include "../world/Grid.h"
#include "../world/Pathfinder.h"
#include "../ui/PathRenderer.h"
#include "../editor/EditorMinecartModal.h"
#include "../editor/EditorWaveModal.h"
#include "../editor/EditorMapsBrowserModal.h"
#include "../editor/EditorHelpModal.h"
#include "../editor/EditorToolbarUI.h"
#include "../editor/EditorLevelSettingsModal.h"
#include "../editor/EditorExitModal.h"
#include "../editor/EditorEmitterInspector.h"
#include "../editor/EditorDecorInspector.h"
#include "../editor/EditorEntityInspector.h"
#include "../../particles/EnvironmentParticleManager.h"

class SpriteRenderer;
class TextRenderer;
class Texture2D;
class GameStateManager;

class MapEditorState : public IGameState {
private:
    GameStateManager& m_stateManager;
    int m_width;
    int m_height;
    std::shared_ptr<SpriteRenderer> m_renderer;
    TextRenderer* m_textRenderer;

    std::shared_ptr<Texture2D> m_whiteTexture;
    std::shared_ptr<Texture2D> m_uiTexture;
    std::shared_ptr<Texture2D> m_arrowTexture;

    // Сетка и данные карты
    int m_gridWidth = 20;
    int m_gridHeight = 12;
    float m_cellSize = 64.0f;
    std::unique_ptr<Grid> m_grid;
    std::unique_ptr<Pathfinder> m_pathfinder;
    PathVisualizer m_pathVisualizer;

    std::vector<SpawnerData> m_spawners;
    std::vector<BaseData> m_bases;
    std::vector<std::vector<int>> m_rawLayout; // 0: Ground, 1: Path, 2: Platform, 3: Scenery, 4: Chasm, 5: Rail
    std::vector<MinecartData> m_minecarts;
    std::vector<glm::ivec2> m_railPath;        // BFS-маршрут від start до end по рейках
    bool m_isRailPathValid = false;            // true якщо шлях знайдено
    std::vector<std::vector<glm::ivec2>> m_activePaths;
    bool m_hasInvalidSpawner = false;
    bool m_missingBaseWarning = false;
    int m_missingBaseId = -1;

    // Режим кисти и выбранный ID
    EditorBrush m_currentBrush = EditorBrush::Wall;
    int m_selectedId = -1; // -1 = Auto/Nearest, 0, 1, 2, 3, 4...

    // Данные и состояние редактора волн
    std::vector<WaveConfig> m_waves;
    EditorWaveModal m_waveModal;
    bool m_keyWPressedLastFrame = false;

    bool m_isLeftMouseDown = false;
    bool m_isRightMouseDown = false;
    bool m_keySPressedLastFrame = false;
    bool m_keyTPressedLastFrame = false;
    bool m_keyCPressedLastFrame = false;
    bool m_keyEscPressedLastFrame = false;
    bool m_keyLeftBracketLastFrame = false;
    bool m_keyRightBracketLastFrame = false;
    bool m_keyVPressedLastFrame = false;
    bool m_keyTabPressedLastFrame = false;
    bool m_keyFPressedLastFrame = false;
    bool m_keyHPressedLastFrame = false;
    bool m_keyF1PressedLastFrame = false;
    bool m_keyUPressedLastFrame = false;
    bool m_keyRPressedLastFrame = false;
    float m_defaultEmitterAngle = 90.0f; // Угол струи по умолчанию (90 deg = вниз)

    // Предпросмотр боевого интерфейса (Combat HUD Preview)
    HudPreviewMode m_hudPreviewMode = HudPreviewMode::None;
    void cycleHudPreviewMode();

    // Инструменты и отображение
    bool m_showPathArrows = true;
    bool m_isBoxFilling = false;
    glm::ivec2 m_boxStartCell = glm::ivec2(-1);
    glm::ivec2 m_boxCurrentCell = glm::ivec2(-1);

    // Интерактивная камера редактора (Zoom & Pan)
    float m_zoom = 1.0f; // диапазон [0.4f, 2.5f]
    glm::vec2 m_cameraPan = glm::vec2(0.0f);
    bool m_isPanning = false;
    glm::vec2 m_prevMousePos = glm::vec2(0.0f);

    glm::vec2 screenToWorld(glm::vec2 screenPos) const {
        return (screenPos - m_cameraPan) / m_zoom;
    }
    glm::vec2 worldToScreen(glm::vec2 worldPos) const {
        return worldPos * m_zoom + m_cameraPan;
    }

    EditorToolbarUI m_toolbarUI;

    std::string m_editorSavePath = "res/levels/level_editor.json";
    std::string m_currentLevelFileName = "level_editor.json";
    std::string m_currentLevelDisplayName = "level_editor";
    bool m_isCampaign = false;
    std::string m_background = "default";
    LevelMapData::CameraSettings m_cameraSettings;
    bool m_isCameraConfigMode = false;
    std::vector<std::string> m_tags;
    int m_startingMoney = 50;
    int m_startingHealth = 20;
    std::vector<std::string> m_allowedTowers = { "Basic", "Mercury", "Piston" };
    int m_maxUpgradeTier = 3;
    std::unordered_map<std::string, int> m_towerMaxTiers = { { "Basic", 3 }, { "Mercury", 3 }, { "Piston", 3 } };
    std::vector<ParticleEmitterConfig> m_emitters;
    EnvironmentParticleManager m_envParticleManager;
    int m_selectedEmitterIndex = -1;
    bool m_isDraggingEmitter = false;
    glm::vec2 m_emitterDragOffset = glm::vec2(0.0f);

    std::vector<DecorationConfig> m_decorations;
    int m_selectedDecorationIndex = -1;
    bool m_isDraggingDecoration = false;
    glm::vec2 m_decorationDragOffset = glm::vec2(0.0f);
    float m_defaultDecorationRotation = 0.0f;

    bool m_suppressPlacementUntilRelease = true;
    bool m_suppressClickUntilRelease = true;

    // Модальні вікна керування картами
    EditorMapsBrowserModal m_mapsBrowserModal;
    EditorHelpModal m_helpModal;
    EditorExitModal m_exitModal;
    EditorLevelSettingsModal m_levelSettingsModal;

    // Флаг изменений
    bool m_isDirty = false;
    int m_diagInputFrames = 0;
    int m_diagRenderFrames = 0;

    std::string m_statusMessage = "";
    float m_statusTimer = 0.0f;
    glm::vec3 m_statusColor = glm::vec3(0.9f, 0.9f, 0.9f);

    // Всплывающие уведомления (Toast Feedback)
    std::string m_toastMessage = "";
    float m_toastTimer = 0.0f;
    glm::vec4 m_toastColor = glm::vec4(0.2f, 0.8f, 0.3f, 1.0f);
    void showToast(const std::string& msg, const glm::vec4& color = glm::vec4(0.2f, 0.8f, 0.3f, 1.0f));

    float getTopBarHeight() const;
    float getBottomDockHeight() const;
    EditorContext buildEditorContext() const;

    void updateButtonLayout();
    void updateGridDimensions();
    bool isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) const;
    void recalculatePaths();
    void recalculateRailPath(); // BFS-валідація маршруту рейок від start до end
    void applyBrush(int gridX, int gridY, EditorBrush brush);
    void eraseCell(int gridX, int gridY);
    int findNearestEmitterIndex(glm::vec2 worldPos, float maxDist = 24.0f) const;
    void eraseEmitterNear(glm::vec2 worldPos, float maxDist = 24.0f);
    int findNearestDecorationIndex(glm::vec2 worldPos, float maxDist = 24.0f) const;
    void eraseDecorationNear(glm::vec2 worldPos, float maxDist = 24.0f);
    void saveMap();
    void loadInitialMap();
    void loadLevelByName(const std::string& fileName);
    void updateCurrentLevelDisplayName();
    void testMap();
    void clearMap();
    void cycleSelectedId(int step);
    glm::vec2 getDefaultWorldCenter() const;
    glm::vec2 getWorldCamCenter() const;
    glm::vec2 getCameraViewportSize() const;
    glm::vec2 worldPixelToTile(glm::vec2 worldPos) const;
    glm::vec2 tileToWorldPixel(glm::vec2 tilePos) const;
    void resizeMap(int newW, int newH, bool expandFromStart = false);
    void shiftMap(int dx, int dy);
    void cycleMapSizePreset();

    // Модальне вікно налаштувань вагонетки
    EditorMinecartModal m_minecartModal;

    void openExitModal();
    void closeExitModal();
    void renderHudPreview();
    void renderHudPreviewText();
    void renderEmitters(SpriteRenderer* renderer, std::shared_ptr<Texture2D> texture);

    EditorEmitterInspector m_emitterInspector;

    EditorDecorInspector m_decorInspector;

    EditorEntityInspector m_entityInspector;
    int m_selectedEntityIndex = -1;
    EditorEntityType m_selectedEntityType = EditorEntityType::Spawner;

    struct KeyRepeatState {
        bool isDown = false;
        float holdTimer = 0.0f;
        float repeatTimer = 0.0f;
    };
    std::unordered_map<int, KeyRepeatState> m_keyStates;

    glm::vec2 m_mousePos = glm::vec2(0.0f);
    float m_cursorBlinkTimer = 0.0f;
    EditorOrigin m_origin = EditorOrigin::MainMenu;
    void returnToOrigin();

public:
    MapEditorState(GameStateManager& stateManager, int width, int height,
                   std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer,
                   const std::string& levelToLoad = "",
                   EditorOrigin origin = EditorOrigin::MainMenu);
    ~MapEditorState() override = default;

    glm::vec3 getIdColor(int id) const;

    void init() override;
    void cleanup() override;
    void processInput(GLFWwindow* window, float dt) override;
    void update(float dt) override;
    void render() override;
    void resize(int width, int height) override;
};

