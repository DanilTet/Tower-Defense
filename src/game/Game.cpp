#include "Game.h"
#include "resources/ResourceManager.h"
#include "renderer/SpriteRenderer.h"
#include "textures/Texture2D.h"
#include <glm/gtc/matrix_transform.hpp>
#include "Grid.h"
#include "Enemy.h"
#include <vector>
#include <iostream>

Game::Game(int width, int height)
    : width(width), height(height), m_mousePressedLastFrame(false), m_gridOffset(100.0f, 100.0f) {
}

Game::~Game() {
    m_renderer.reset();
    m_gameGrid.reset();
}

void Game::init() {
    if (!ResourceManager::loadShader("spriteShader", "res/shaders/vertex_shader.vert", "res/shaders/fragment_shader.frag")) {
        std::cerr << "Failed to load shaders" << std::endl;
    }
    ResourceManager::loadTexture("towerTexture", "res/textures/test_sprite.png");

    ShaderProgram* shader = ResourceManager::getShader("spriteShader");
    std::shared_ptr<ShaderProgram> shaderPtr(shader, [](ShaderProgram*) {});
    m_renderer = std::make_unique<SpriteRenderer>(shaderPtr);

    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(this->width), static_cast<float>(this->height), 0.0f, -1.0f, 1.0f);
    m_renderer->setProjection(projection);

    shader->use();
    glUniform1i(glGetUniformLocation(shader->getId(), "u_texture"), 0);

    m_gameGrid = std::make_unique<Grid>(10, 7, 64.0f);
    m_gameGrid->updateCellSize(this->width, this->height);

    Texture2D* cellTex = ResourceManager::getTexture("towerTexture");
    m_cellTexture = std::shared_ptr<Texture2D>(cellTex, [](Texture2D*) {});

    //Движение врага
    m_levelPath = { {0, 0}, {3, 0}, {3, 3}, {6, 3}, {6, 6}, {9, 6} }; // маршрут врага по клеткам сетки
    
    spawnEnemy(100.0f); // спавн врага
    spawnEnemy(90.0f);
    spawnEnemy(42.0f);
}

void Game::processInput(GLFWwindow* window, float dt) {
    int mouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

    if (mouseState == GLFW_PRESS && !m_mousePressedLastFrame) {
        m_mousePressedLastFrame = true;
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        glm::ivec2 clickedCell = m_gameGrid->pixelToGrid(glm::vec2(mouseX, mouseY), m_gridOffset);

        if (m_gameGrid->canBuildAt(clickedCell.x, clickedCell.y)) {
            m_gameGrid->setCellType(clickedCell.x, clickedCell.y, CellType::Tower);
            std::cout << "Поставили башню в ячейку: " << clickedCell.x << ", " << clickedCell.y << std::endl;
        }
    }
    else if (mouseState == GLFW_RELEASE) {
        m_mousePressedLastFrame = false;
    }

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        spawnEnemy(130.0f);
    }
}

void Game::update(float dt) {
	// Обновляем всех врагов
    for (const auto& enemy : m_enemies) {
        if (enemy) {
            enemy->update(dt, *m_gameGrid);
        }
    }

	// Удаляем врагов, которые достигли конца пути
    m_enemies.erase(
        std::remove_if(m_enemies.begin(), m_enemies.end(),
            [](const std::unique_ptr<Enemy>& enemy) {
                return enemy->isReachedEnd();
            }),
        m_enemies.end()
    );
}

void Game::render() {
	// Рисуем сетку
    m_gameGrid->draw(m_renderer.get(), m_cellTexture, m_gridOffset, { 1.0f, 1.0f, 1.0f });

	// Рисуем всех врагов
    for (const auto& enemy : m_enemies) {
        if (enemy) {
            enemy->render(m_renderer.get(), m_cellTexture, m_gridOffset);
        }
    }
}

void Game::resize(int width, int height) {
    this->width = width;
    this->height = height;
    if (m_renderer) {
        glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, -1.0f, 1.0f);
        m_renderer->setProjection(projection);
    }
    if (m_gameGrid) {
        m_gameGrid->updateCellSize(width, height);
    }
}

// функция для спавна врага (принимает скорость)
void Game::spawnEnemy(float speed) {
	auto newEnemy = std::make_unique<Enemy>(m_levelPath, *m_gameGrid, speed); // спавн нового врага
    m_enemies.push_back(std::move(newEnemy));
}