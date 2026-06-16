#include "StatsPanel.h"
#include "core/WaveManager.h"
#include "renderer/TextRenderer.h"
#include <string>
#include "gameplay/PlayerStats.h"
#include "UICommon.h"

void StatsPanel::drawStatsPanel(const PlayerStats& stats, WaveManager* waveSize, TextRenderer* textRenderer, int screenWidth, int screenHeight) {
	if (!textRenderer || !waveSize) return;
	// получаем масштаб для текущего окна
	float scale = GetUIScale(screenWidth, screenHeight);

	glm::vec2 panelSize(200.0f * scale, 60.0f * scale); // размер менюшки
	glm::vec2 offset(20.0f * scale, 20.0f * scale); // отступ

	glm::vec2 finalPos = CalculateAnchorPosition(UIAnchor::TopRight, offset, panelSize, screenWidth, screenHeight);

	float textScale = 1.0f * scale; // масштаб шрифта
	float step = 35.0f * scale; // расстояние между строками

	//textRenderer->RenderText("Очки: " + std::to_string(stats.score), 25.0f, 130.0f, 1.0f, glm::vec3(0.2f, 0.8f, 1.0f));

	textRenderer->RenderText("Деньги: " + std::to_string(stats.money), finalPos.x, finalPos.y, textScale, glm::vec3(1.0f, 0.9f, 0.2f));
	textRenderer->RenderText("База: " + std::to_string(stats.baseHealth) + " HP", finalPos.x, finalPos.y + step, textScale, glm::vec3(1.0f, 0.3f, 0.3f));
	textRenderer->RenderText("Волна: " + std::to_string(waveSize->getCurrentWaveNumber()), finalPos.x, finalPos.y + step * 2, textScale, glm::vec3(0.5f, 1.0f, 0.5f));

}