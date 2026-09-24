#include "GameplayInputHandler.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "../world/GameWorld.h"
#include "../ui/BuildPanel.h"
#include "../ui/TowerMenuUI.h"
#include "../gameplay/BuildManager.h"
#include "../entities/Tower.h"

bool GameplayInputHandler::isKeyJustPressed(GLFWwindow* window, int key) {
    if (key < 0 || key >= 1024) return false;

    if (glfwGetKey(window, key) == GLFW_PRESS) {
        if (!m_keysProcessed[key]) {
            m_keysProcessed[key] = true;
            return true;
        }
    }
    else {
        m_keysProcessed[key] = false;
    }

    return false;
}

void GameplayInputHandler::processInput(
    GLFWwindow* window,
    float dt,
    int windowWidth,
    int windowHeight,
    GameWorld& world,
    Buildpanel& buildPanel,
    TowerMenuUI& towerMenuUI,
    BuildManager& buildManager,
    std::string& selectedTowerType,
    Tower*& selectedTowerOnMap,
    std::function<void()> onPauseRequested,
    std::function<void()> onNextWaveRequested)
{
    if (isKeyJustPressed(window, GLFW_KEY_ESCAPE)) {
        if (onPauseRequested) onPauseRequested();
        return;
    }

    if (isKeyJustPressed(window, GLFW_KEY_ENTER)) {
        if (onNextWaveRequested) onNextWaveRequested();
    }

    if (isKeyJustPressed(window, GLFW_KEY_M)) {
        world.playerStats.money += 99999;
    }

    if (isKeyJustPressed(window, GLFW_KEY_SPACE)) {
        world.spawnEnemy("Basic");
    }

    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    m_currentMousePos = glm::vec2(mouseX, mouseY);

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        selectedTowerType = "";
    }

    int mouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

    if (mouseState == GLFW_PRESS && !m_mousePressedLastFrame) {
        m_mousePressedLastFrame = true;

        if (buildPanel.checkClick(mouseX, mouseY, windowWidth, windowHeight, selectedTowerType)) {
            selectedTowerOnMap = nullptr;
            return;
        }

        if (towerMenuUI.processClick(mouseX, mouseY, selectedTowerOnMap, &buildManager, world, selectedTowerOnMap)) {
            return;
        }

        if (world.grid && world.entityManager) {
            glm::ivec2 clickedCell = world.grid->pixelToGrid(glm::vec2(mouseX, mouseY));
            Tower* clickedTower = world.entityManager->getTowerAt(clickedCell.x, clickedCell.y);

            if (clickedTower != nullptr) {
                selectedTowerOnMap = clickedTower;
                selectedTowerType = "";
                return;
            }
            else {
                selectedTowerOnMap = nullptr;
            }
        }

        buildManager.tryBuildOrUpgrade(
            m_currentMousePos,
            selectedTowerType,
            world
        );
    }
    else if (mouseState == GLFW_RELEASE) {
        m_mousePressedLastFrame = false;
    }
}
