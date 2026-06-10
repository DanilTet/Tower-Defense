#include "StatsPanel.h"
#include "core/WaveManager.h"
#include "renderer/TextRenderer.h"
#include <string>
#include "gameplay/PlayerStats.h"

void StatsPanel::drawStatsPanel(const PlayerStats& playerStats, WaveManager* waveManager, TextRenderer* textRenderer) {
	if (!textRenderer || !waveManager) return;

	textRenderer->RenderText("Деньги: " + std::to_string(playerStats.money), 25.0f, 25.0f, 1.0f, glm::vec3(1.0f, 0.9f, 0.2f));

	textRenderer->RenderText("База: " + std::to_string(playerStats.baseHealth) + " HP", 25.0f, 60.0f, 1.0f, glm::vec3(1.0f, 0.3f, 0.3f));

	//textRenderer->RenderText("Очки: " + std::to_string(stats.score), 25.0f, 130.0f, 1.0f, glm::vec3(0.2f, 0.8f, 1.0f));

	textRenderer->RenderText("Волна: " + std::to_string(waveManager->getCurrentWaveNumber()), 25.0f, 95.0f, 1.0f, glm::vec3(0.5f, 1.0f, 0.5f));

}