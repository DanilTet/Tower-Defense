#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include "../core/LevelManager.h"
#include "../core/WaveData.h"

class SpriteRenderer;
class TextRenderer;
class Texture2D;
struct GLFWwindow;

enum class WaveFocusedField {
    None,
    Count,
    Interval,
    Delay
};

class EditorWaveModal {
public:
    EditorWaveModal() = default;

    void open();
    void open(std::vector<WaveConfig>& waves);
    void open(LevelMapData& mapData);

    void close();

    void toggle();
    void toggle(std::vector<WaveConfig>& waves);
    void toggle(LevelMapData& mapData);

    bool isOpen() const { return m_isOpen; }

    void update(float dt);

    bool handleInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown, float dt,
                     int screenWidth, int screenHeight, float topBarHeight, float bottomDockHeight,
                     std::vector<WaveConfig>& waves, bool& isDirty);

    bool handleInput(float mouseX, float mouseY, bool mousePressed, LevelMapData& mapData, int key = -1, int action = -1);

    void render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                const std::shared_ptr<Texture2D>& whiteTexture,
                int screenWidth, int screenHeight, float topBarHeight, float bottomDockHeight,
                glm::vec2 mousePos, const std::vector<WaveConfig>& waves);

    void render(SpriteRenderer* renderer, TextRenderer* textRenderer, float screenWidth, float screenHeight, const LevelMapData& mapData);

    void commitFocusedInput(std::vector<WaveConfig>& waves);
    void startEditingField(int partIdx, WaveFocusedField field, const std::vector<WaveConfig>& waves);
    void cycleNextInputField(std::vector<WaveConfig>& waves);

    int getSelectedWaveIdx() const { return m_selectedWaveIdx; }
    void setSelectedWaveIdx(int idx) { m_selectedWaveIdx = idx; }

private:
    bool m_isOpen = false;
    bool m_wasLeftDown = false;
    LevelMapData* m_boundMapData = nullptr;

    int m_selectedWaveIdx = 0;
    int m_wavePartsScrollOffset = 0;

    WaveFocusedField m_focusedField = WaveFocusedField::None;
    int m_focusedPartIdx = -1;
    std::string m_inputText = "";
    bool m_fieldJustFocused = false;
    float m_cursorBlinkTimer = 0.0f;
    int m_openDropdownPartIdx = -1; // -1 = closed

    struct KeyRepeatState {
        bool isDown = false;
        float holdTimer = 0.0f;
        float repeatTimer = 0.0f;
    };
    std::unordered_map<int, KeyRepeatState> m_keyStates;

    static bool isPointInRect(glm::vec2 point, glm::vec2 rectPos, glm::vec2 rectSize);
    static std::string formatFloat1(float val);
};

