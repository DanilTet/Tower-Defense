#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <string>

class SpriteRenderer;
class TextRenderer;
class Texture2D;

enum class EditorBrush {
    Ground = 0,    // 0: Земля (можно строить и ходить)
    Wall = 1,      // 3: Стена/Вода (нельзя строить, нельзя ходить)
    Platform = 2,  // 2: Платформа (можно строить, нельзя ходить)
    Path = 3,      // 1: Дорога (нельзя строить, можно ходить)
    Spawner = 4,   // Точка спавна врагов
    Base = 5,      // База игрока
    Eraser = 6,    // Ластик: стирает тайлы, спавнеры и базы до Земли
    Chasm = 7,     // 4: Шурф / Обрыв (нельзя строить, нельзя ходить)
    Rail = 8,      // 5: Рельсы (нельзя строить, можно ходить)
    RailStart = 9, // Маркер Депо / Старт вагонетки
    RailEnd = 10   // Маркер Тупик / Финиш вагонетки
};

enum class PaletteCategory {
    Tiles,
    SpecialObjects
};

enum class EditorActionType {
    None = 0,
    SetBrush,
    SaveMap,
    TestMap,
    ClearMap,
    Exit,
    CycleSelectedId,
    CyclePreset,
    ResizeMap,
    OpenRenameModal,
    NewMap,
    OpenMapsModal,
    ToggleCampaign,
    ToggleWavesModal,
    ToggleMinecartModal,
    SwitchPaletteCategory
};

struct EditorAction {
    EditorActionType type = EditorActionType::None;
    EditorBrush brush = EditorBrush::Wall;
    int intParam = 0;   // e.g. delta step for resize or cycleId
    int intParam2 = 0;  // secondary delta (e.g. height delta for resize)
};

struct EditorButton {
    glm::vec2 pos{0.0f};
    glm::vec2 size{0.0f};
    std::string label;
    EditorBrush brush = EditorBrush::Wall;
    bool isAction = false;
    int actionId = 0;
};

struct EditorContext {
    int screenWidth = 1280;
    int screenHeight = 720;
    std::string currentLevelDisplayName = "level_editor";
    std::string currentLevelFileName = "level_editor.json";
    bool isCampaign = false;
    int gridWidth = 20;
    int gridHeight = 12;
    EditorBrush currentBrush = EditorBrush::Wall;
    int selectedId = -1;
    std::string statusMessage = "";
    float statusTimer = 0.0f;
    glm::vec3 statusColor = glm::vec3(0.9f);
    bool hasInvalidSpawner = false;
    bool missingBaseWarning = false;
    int missingBaseId = -1;
    bool hasMinecartStartEnd = false;
    bool isRailPathValid = false;
    size_t railPathSize = 0;
    bool isWaveModalOpen = false;
    bool isMinecartModalOpen = false;
    PaletteCategory currentCategory = PaletteCategory::Tiles;
};

class EditorToolbarUI {
public:
    EditorToolbarUI() = default;

    void init();
    void update(float dt);

    void updateLayout(TextRenderer* textRenderer, const EditorContext& ctx);

    EditorAction handleInput(float mouseX, float mouseY, bool mousePressed, bool wasMousePressed, bool isShiftDown, const EditorContext& ctx);
    EditorAction handleInput(float mouseX, float mouseY, bool mousePressed, bool mouseReleased, EditorContext& ctx);

    void render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                const std::shared_ptr<Texture2D>& whiteTexture,
                const EditorContext& ctx, glm::vec2 mousePos);

    void render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                float screenWidth, float screenHeight, const EditorContext& ctx);

    bool isHovered(float mouseX, float mouseY) const;

    float getTopBarHeight() const { return m_topBarH; }
    float getBottomDockHeight() const { return m_dockH; }
    float getTabHeight() const { return m_tabH; }

    PaletteCategory getCategory() const { return m_currentCategory; }
    void setCategory(PaletteCategory cat) { m_currentCategory = cat; }

    static float calculateTopBarHeight(int width, int height);
    static float calculateBottomDockHeight(int width, int height);

    static glm::vec3 getIdColor(int id);

private:
    float m_topBarH = 42.0f;
    float m_dockH = 54.0f;
    float m_tabH = 28.0f;
    float m_topBarLeftEndX = 540.0f;
    float m_topFontScale = 0.50f;
    float m_bottomFontScale = 0.48f;
    int m_screenWidth = 1280;
    int m_screenHeight = 720;

    PaletteCategory m_currentCategory = PaletteCategory::Tiles;

    std::vector<EditorButton> m_topButtons;
    std::vector<EditorButton> m_bottomButtons;
    std::vector<EditorButton> m_tabButtons;

    static bool isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize);
};
