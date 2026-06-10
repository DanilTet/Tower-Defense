#pragma once
#include <glm/glm.hpp>

class WaveManager;
class TextRenderer;
struct PlayerStats;

class StatsPanel {
public:
	void drawStatsPanel(const PlayerStats& playerStats, WaveManager* waveManager, TextRenderer* textRenderer);
};