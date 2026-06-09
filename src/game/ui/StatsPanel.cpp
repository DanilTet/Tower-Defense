#include "StatsPanel.h"
#include "core/WaveManager.h"
#include "renderer/TextRenderer.h"
#include <string>

void StatsPanel::drawstatsPanel(int playerMoney, int baseHealth, WaveManager* waveManager, TextRenderer* textRenderer){
	if (!textRenderer || !waveManager) return;

	textRenderer->RenderText("Деньги: " + std::to_string(playerMoney), 25.0f, 25.0f, 1.0f, glm::vec3(1.0f, 0.9f, 0.2f));

	textRenderer->RenderText("База: " + std::to_string(baseHealth) + " HP", 25.0f, 60.0f, 1.0f, glm::vec3(1.0f, 0.3f, 0.3f));

	textRenderer->RenderText("Волна: " + std::to_string(waveManager->getCurrentWaveNumber()), 25.0f, 95.0f, 1.0f, glm::vec3(0.5f, 1.0f, 0.5f));

}