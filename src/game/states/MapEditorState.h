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
#include "../editor/EditorToolbarUI.h"

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

    // Инструменты и отображение
    bool m_showPathArrows = true;
    bool m_isBoxFilling = false;
    glm::ivec2 m_boxStartCell = glm::ivec2(-1);
    glm::ivec2 m_boxCurrentCell = glm::ivec2(-1);

    EditorToolbarUI m_toolbarUI;

    std::string m_editorSavePath = "res/levels/level_editor.json";
    std::string m_currentLevelFileName = "level_editor.json";
    std::string m_currentLevelDisplayName = "level_editor";
    bool m_isCampaign = false;
    std::vector<std::string> m_tags;
    bool m_suppressPlacementUntilRelease = true;
    bool m_suppressClickUntilRelease = true;

    // Модальні вікна керування картами
    EditorMapsBrowserModal m_mapsBrowserModal;
    bool m_isExitModalOpen = false;
    bool m_exitModalEscReleased = false;

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
    bool isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize) const;
    void recalculatePaths();
    void recalculateRailPath(); // BFS-валідація маршруту рейок від start до end
    void applyBrush(int gridX, int gridY, EditorBrush brush);
    void eraseCell(int gridX, int gridY);
    void saveMap();
    void loadInitialMap();
    void loadLevelByName(const std::string& fileName);
    void updateCurrentLevelDisplayName();
    void testMap();
    void clearMap();
    void cycleSelectedId(int step);
    void resizeMap(int newW, int newH);
    void cycleMapSizePreset();

    // Модальне вікно налаштувань вагонетки
    EditorMinecartModal m_minecartModal;

    struct ExitModalLayout {
        glm::vec2 modalPos;
        glm::vec2 modalSize;
        float headerH = 40.0f;
        glm::vec2 btnCloseCrossPos;
        glm::vec2 btnCloseCrossSize;
        glm::vec2 btnSaveExitPos;
        glm::vec2 btnSaveExitSize;
        glm::vec2 btnDiscardPos;
        glm::vec2 btnDiscardSize;
        glm::vec2 btnCancelPos;
        glm::vec2 btnCancelSize;
        float fTitle = 0.68f;
        float fQuestion = 0.66f;
        float fSub = 0.48f;
        float fBtn = 0.46f;
        float fCross = 0.55f;
        float questionY = 0.0f;
        float subY = 0.0f;
    };
    ExitModalLayout getExitModalLayout() const;


    void openExitModal();
    void closeExitModal();
    void renderExitModal();
    bool processExitModalInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, float dt);

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

