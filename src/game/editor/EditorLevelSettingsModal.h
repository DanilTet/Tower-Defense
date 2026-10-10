#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

class SpriteRenderer;
class TextRenderer;
class Texture2D;
struct GLFWwindow;

class EditorLevelSettingsModal {
public:
    enum class FocusedField {
        None,
        Money,
        Health
    };

    struct KeyState {
        bool isDown = false;
        float holdTimer = 0.0f;
        float repeatTimer = 0.0f;
    };

    EditorLevelSettingsModal() = default;

    void open(int currentMoney, int currentHealth,
              const std::vector<std::string>& allowedTowers,
              const std::unordered_map<std::string, int>& towerMaxTiers);
    void close();
    void toggle(int currentMoney, int currentHealth,
                const std::vector<std::string>& allowedTowers,
                const std::unordered_map<std::string, int>& towerMaxTiers);
    bool isOpen() const { return m_isOpen; }

    void update(float dt);

    bool handleInput(GLFWwindow* window, glm::vec2 mousePos, bool leftDown,
                     bool leftReleased, bool isEscJustPressed, float dt,
                     int screenWidth, int screenHeight,
                     int& outMoney, int& outHealth,
                     std::vector<std::string>& outAllowedTowers,
                     std::unordered_map<std::string, int>& outTowerMaxTiers,
                     int& outMaxUpgradeTier, bool& outDirty);

    void render(SpriteRenderer* renderer, TextRenderer* textRenderer,
                const std::shared_ptr<Texture2D>& whiteTexture,
                int screenWidth, int screenHeight, glm::vec2 mousePos);

private:
    bool checkKeyRepeat(GLFWwindow* window, int key, float dt);
    static bool isPointInRect(glm::vec2 p, glm::vec2 pos, glm::vec2 size);

    bool m_isOpen = false;
    bool m_wasLeftDown = false;
    bool m_justOpened = false;

    std::string m_moneyStr = "50";
    std::string m_healthStr = "20";
    std::vector<std::string> m_allowedTowers = { "Basic", "Mercury", "Piston" };
    std::unordered_map<std::string, int> m_towerMaxTiers = { { "Basic", 3 }, { "Mercury", 3 }, { "Piston", 3 } };

    FocusedField m_focusedField = FocusedField::None;
    float m_cursorBlinkTimer = 0.0f;

    std::unordered_map<int, KeyState> m_keyStates;
};

