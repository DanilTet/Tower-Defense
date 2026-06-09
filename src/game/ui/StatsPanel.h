#pragma once
#include <glm/glm.hpp>

class WaveManager;
class TextRenderer;

class StatsPanel {
public:
	void drawstatsPanel(int playerMoney, int baseHealth, WaveManager* waveManager, TextRenderer* textRenderer);
};