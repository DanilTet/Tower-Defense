#include "GameplayState.h"
#include "GameStateManager.h"
#include "PauseState.h"
#include "../resources/ResourceManager.h"
#include "../audio/AudioManager.h"
#include "../core/ConfigManager.h"
#include "../core/LevelManager.h"
#include "../core/EventBus.h"
#include "../core/SaveManager.h"
#include "MainMenuState.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include "GameOverState.h"
#include "VictoryState.h"

GameplayState::GameplayState(GameStateManager& stateManager, int windowWidth, int windowHeight, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer, std::string levelPath)
    : m_stateManager(stateManager),
    width(windowWidth),
    height(windowHeight),
    m_renderer(renderer),
    m_textRenderer(textRenderer),
    m_mousePressedLastFrame(false),
    m_selectedTowerType(""),
    m_currentLevelPath(levelPath)
{
}

void GameplayState::cleanup() {
    EventBus::clear();
}

void GameplayState::init() {
    // загрузка конфигураций
    ConfigManager::loadConfigs("res/configs/towers.json", "res/configs/enemies.json", "res/configs/particles.json");
    ConfigManager::loadTextureConfig("res/levels/textures.json");

    // загрузка атласов
    ResourceManager::loadTexture("mainAtlas", "res/textures/mainAtlas.png");
    ResourceManager::loadTexture("enemyAtlas", "res/textures/enemyAtlas.png");

    // ЗАГРУЗКА ФАЙЛОВ ТЕКСТУРОК в VRAM
    ResourceManager::loadTexture("towerTexture", "res/textures/test_sprite.png"); // текстурка башни
    ResourceManager::loadTexture("grassTexture", "res/textures/spr_grass_02.png"); // текстурка тайла траві
    ResourceManager::loadTexture("radiusTexture", "res/textures/radius2.png"); // радиус атаки башни
    ResourceManager::loadTexture("arrowTexture", "res/textures/pathArrow.png"); // стрелочка пути
    ResourceManager::loadTexture("particleTexture", "res/textures/particle.png"); // партикл
    ResourceManager::loadTexture("uiBaseTexture", "res/textures/ui_space.png"); // для менюшек белый квадрат

    // Получаем указатель на текстура из ResourceManager
    Texture2D* radTex = ResourceManager::getTexture("radiusTexture"); // радиус атаки башни
    Texture2D* partTex = ResourceManager::getTexture("particleTexture"); // партикл

    // указатель на тексту оборачиваем его в shared_ptr, чтобы управлять временем жизни текстуры
    m_radiusTexture = std::shared_ptr<Texture2D>(radTex, [](Texture2D*) {}); // радиус атаки башни
    m_particleTexture = std::shared_ptr<Texture2D>(partTex, [](Texture2D*) {}); // партикл

    // атласы
    Texture2D* mainAtlasTex = ResourceManager::getTexture("mainAtlas");
    m_mainAtlas = std::shared_ptr<Texture2D>(mainAtlasTex, [](Texture2D*) {});

    Texture2D* enemyAtlasTex = ResourceManager::getTexture("enemyAtlas");
    m_enemyAtlas = std::shared_ptr<Texture2D>(enemyAtlasTex, [](Texture2D*) {});

    m_world = std::make_unique<GameWorld>();

    // если есть строка сохранения то загружаем его 
    if (!m_saveToLoad.empty()) {
        bool success = loadSavedGame(m_saveToLoad);
        m_saveToLoad = "";

        if (!success) {
            std::cerr << "[ERROR] Файл сохранения не найден! Загрузка отменена." << std::endl;
            m_isValid = false;
            return;
        }
        else {
            return;
        }
    }

    // ИНИЦИАЛИЗАЦИЯ ИГРОВОГО МИРА
    m_world->loadLevel(m_currentLevelPath, this->width, this->height);

    // АУДИО
    AudioManager::playMusic("res/sounds/background.mp3"); // врубаем имбовый трек

    // ИНТЕРФЕЙС
    m_buildPanel = std::make_unique<Buildpanel>(); // инициализация панельки для строительства
    m_buildPanel->initPanelData();
    m_placementUI = std::make_unique<PlacementUI>(); // инициализация голограммы для строительства
    m_pathVisualizer = std::make_unique<PathVisualizer>(); // инициализация стрелочек пути
    m_statsPanel = std::make_unique<StatsPanel>(); // инициализация панельки здровья деняг и всякого такого
    m_towerMenuUI = std::make_unique<TowerMenuUI>(); // инициализация панельки при клике на башню

    // Гейплей
    m_buildManager = std::make_unique<BuildManager>(); // инициализация абгрейдера и строителя башен

    // КРУТАЯ ШИНА СОБЫТИЙ
    EventBus::clear(); // очищаем старые подписки

    // тут слушаем смерть врага
    EventBus::subscribe(EventType::EnemyDied, [this](const Event& e) {
        this->m_world->playerStats.money += e.value1;
        this->m_world->playerStats.score += e.value2;
        if (!e.textData.empty()) {
            AudioManager::playSound(e.textData.c_str(), 0.1f);
        }
    });

    // тут слушаем прорыв врага на базу
    EventBus::subscribe(EventType::EnemyReachedBase, [this](const Event& e) {
        this->m_world->playerStats.baseHealth -= e.value1;
        if (this->m_world->playerStats.baseHealth <= 0) {
            this->m_world->playerStats.baseHealth = 0;
            std::cout << "GAME OVER!" << std::endl;
        }
    });

    // тут слушаем выстрелы башен
    EventBus::subscribe(EventType::TowerFired, [](const Event& e) {
        if (!e.textData.empty()) { // проверка что строка не пустая
            AudioManager::playSound(e.textData.c_str(), 0.1f);
        }
    });

    EventBus::subscribe(EventType::TowerBuilt, [](const Event& e) {
        if (!e.textData.empty()) {
            AudioManager::playSound(e.textData.c_str(), 0.1f);
        }
    });
}

void GameplayState::processInput(GLFWwindow* window, float dt) {
    if (!m_isValid) return;

    if (isKeyJustPressed(window, GLFW_KEY_ESCAPE)) {
        m_stateManager.pushState(std::make_unique<PauseState>(m_stateManager, width, height, m_renderer, m_textRenderer, this));
        return;
    }

    if (isKeyJustPressed(window, GLFW_KEY_ENTER)) startNextWave();
    if (isKeyJustPressed(window, GLFW_KEY_M)) m_world->playerStats.money += 99999;
    if (isKeyJustPressed(window, GLFW_KEY_SPACE)) m_world->spawnEnemy("Basic");

    // читаем мышку каждый кадр
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    m_currentMousePos = glm::vec2(mouseX, mouseY);

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        m_selectedTowerType = ""; // очищаем руку, голограмма сразу исчезнет
    }

    // Получаем состояние левой кнопки мыши
    int mouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

    // Если ЛКМ зажата и на прошлом кадре она не была зажата, то фиксируем момент клика
    if (mouseState == GLFW_PRESS && !m_mousePressedLastFrame) {
        m_mousePressedLastFrame = true; // меняем флаг нажатия на кнопку
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        // если кликнули по менюшке выбора башни то обновляем выбраную башню
        if (m_buildPanel->checkClick(mouseX, mouseY, this->width, this->height, m_selectedTowerType)) {
            m_selectedTowerOnMap = nullptr; // если выбрали новую башню для стройки то снимаем выделение с башни на карте
            return;
        }

        // чекаем клик по меню башни
        if (m_towerMenuUI->processClick(mouseX, mouseY, m_selectedTowerOnMap, m_buildManager.get(), *m_world, m_selectedTowerOnMap)) {
            return; // если кликнули по кнопке, выходим
        }

        // чекаем клик по башне
        glm::ivec2 clickedCell = m_world->grid->pixelToGrid(glm::vec2(mouseX, mouseY));
        Tower* clickedTower = m_world->entityManager->getTowerAt(clickedCell.x, clickedCell.y);

        if (clickedTower != nullptr) {
            m_selectedTowerOnMap = clickedTower;
            m_selectedTowerType = ""; // очищаем руку
            return;
        }
        else { // снять выделение
            m_selectedTowerOnMap = nullptr;
        }

        m_buildManager->tryBuildOrUpgrade(
            m_currentMousePos,
            m_selectedTowerType,
            *m_world
        );
    }
    // Если мышка отпущена — сбрасываем флаг зажатия
    else if (mouseState == GLFW_RELEASE) {
        m_mousePressedLastFrame = false;
    }
}

void GameplayState::update(float dt) {
    if (!m_isValid) {
        m_stateManager.setState(std::make_unique<MainMenuState>(m_stateManager, width, height, m_renderer, m_textRenderer));
        return;
    }

    //обновляем визуализатор пути
    if (m_world->grid) m_pathVisualizer->update(dt, m_world->grid->getCellSize());
    // обновляем башни, пули и другое
    m_world->update(dt);

    // проверка на проигрыш
    if (m_world->playerStats.baseHealth <= 0) {
        m_world->playerStats.baseHealth = 0;
        m_stateManager.setState(std::make_unique<GameOverState>(m_stateManager, width, height, m_renderer, m_textRenderer, m_currentLevelPath));
        return;
    }

    if (m_world->waveManager->isAllWavesCompleted() && m_world->entityManager->getEnemies().empty()) {
        m_stateManager.setState(std::make_unique<VictoryState>(m_stateManager, width, height, m_renderer, m_textRenderer));
        return;
    }
}

void GameplayState::render() {
    if (!m_isValid) return;

    m_renderer->beginBatch(); // открываем пакет

    // Малюем игровую сетку
    m_world->grid->draw(m_renderer.get(), m_mainAtlas, { 1.0f, 1.0f, 1.0f });

    // стрелочки пути   
    Texture2D* arrowTex = ResourceManager::getTexture("arrowTexture");
    std::shared_ptr<Texture2D> arrowTexPtr(arrowTex, [](Texture2D*) {});
    for (const auto& path : m_world->paths) {
        m_pathVisualizer->renderPathArrows(
            m_renderer.get(),
            arrowTexPtr,
            path,
            *m_world->grid
        );
    }

    // рисуем все башни врагов и пули
    m_world->entityManager->render(m_renderer.get(), m_mainAtlas, m_enemyAtlas, m_radiusTexture, arrowTexPtr, m_particleTexture, *m_world->grid, m_selectedTowerOnMap);

    m_renderer->flush();

    // ОТРИСОВКА ИНТЕРФЕЙСА
    Texture2D* uiTex = ResourceManager::getTexture("uiBaseTexture");
    std::shared_ptr<Texture2D> uiTexPtr(uiTex, [](Texture2D*) {});
    
    m_towerMenuUI->render(m_selectedTowerOnMap, m_renderer.get(), m_textRenderer, uiTexPtr, *m_world->grid);

    bool hasPath = !m_world->paths.empty();
    glm::vec2 currentPanelPos = m_buildPanel->getUIPanelPos(this->width, this->height);

    m_placementUI->renderHologram(
        m_renderer.get(),
        m_mainAtlas,
        m_radiusTexture,
        *m_world->grid,
        m_currentMousePos,
        m_selectedTowerType,
        m_world->playerStats,
        currentPanelPos,
        hasPath
    );

    m_statsPanel->drawStatsPanel(m_world->playerStats, m_world->waveManager.get(), m_textRenderer, this->width, this->height);

    m_buildPanel->BuildRenderUI(
        m_world->playerStats,
        m_renderer.get(),
        m_textRenderer,
        m_mainAtlas,
        uiTexPtr,
        this->width,
        this->height,
        m_selectedTowerType
    );

    m_renderer->endBatch(); // закрываем пакет
}

void GameplayState::startNextWave() {
    if (m_world->waveManager) m_world->waveManager->startNextWave();
}

void GameplayState::restartGame() {
    m_world = std::make_unique<GameWorld>();
    m_world->loadLevel(m_currentLevelPath, this->width, this->height);
    m_selectedTowerType = "";
    m_selectedTowerOnMap = nullptr;
    std::cout << "Game Restarted!" << std::endl;
}

bool GameplayState::isKeyJustPressed(GLFWwindow* window, int key) {
    if (key < 0 || key >= 1024) return false;

    if (glfwGetKey(window, key) == GLFW_PRESS) {
        if (!m_keysProcessed[key]) {
            m_keysProcessed[key] = true; // блочим кнопку
            return true; // срабатывает только один раз
        }
    }
    else {
        m_keysProcessed[key] = false; // если кнопку отпустили то убираем блок
    }

    return false;
}

void GameplayState::resize(int windowWidth, int windowHeight) {
    this->width = windowWidth;
    this->height = windowHeight;

    if (!m_isValid) return;

    if (m_world) {
        m_world->resize(windowWidth, windowHeight);
    }
}

void GameplayState::saveGame(const std::string& saveName) {
    SaveManager saveManager;
    saveManager.writeSave(saveName, m_currentLevelPath, m_world->playerStats, *m_world->waveManager, *m_world->entityManager);
    std::cout << "[GameplayState] Игра сохранена в слот: " << saveName << std::endl;
}

bool GameplayState::loadSavedGame(const std::string& saveName) {
    SaveManager saveManager;
    std::string loadedLevelPath;
    PlayerStats loadedStats;
    int loadedWaveIndex = 0;
    nlohmann::json restoredTowers;

    if (!saveManager.readSave(saveName, loadedLevelPath, loadedStats, loadedWaveIndex, restoredTowers)) {
        std::cout << "[LoadGame] Не удалось загрузить файл: " << saveName << std::endl;
        return false;
    }

    std::cout << "[LoadGame] Файл прочитан. Начинаем восстановление стейта..." << std::endl;

    cleanup();

    m_currentLevelPath = loadedLevelPath;
    m_selectedTowerType = "";
    m_selectedTowerOnMap = nullptr;

    m_world = std::make_unique<GameWorld>();
    m_world->loadLevel(m_currentLevelPath, this->width, this->height);

    m_world->playerStats = loadedStats;
    m_world->waveManager->setCurrentWaveIndex(loadedWaveIndex);

    AudioManager::playMusic("res/sounds/background.mp3");
    m_buildPanel = std::make_unique<Buildpanel>();
    m_buildPanel->initPanelData();
    m_placementUI = std::make_unique<PlacementUI>();
    m_pathVisualizer = std::make_unique<PathVisualizer>();
    m_statsPanel = std::make_unique<StatsPanel>();
    m_towerMenuUI = std::make_unique<TowerMenuUI>();

    m_buildManager = std::make_unique<BuildManager>();

    EventBus::clear();

    EventBus::subscribe(EventType::EnemyDied, [this](const Event& e) {
        this->m_world->playerStats.money += e.value1;
        this->m_world->playerStats.score += e.value2;
        if (!e.textData.empty()) AudioManager::playSound(e.textData.c_str(), 0.1f);
    });

    EventBus::subscribe(EventType::EnemyReachedBase, [this](const Event& e) {
        this->m_world->playerStats.baseHealth -= e.value1;
        if (this->m_world->playerStats.baseHealth <= 0) {
            this->m_world->playerStats.baseHealth = 0;
            std::cout << "GAME OVER!" << std::endl;
        }
    });

    EventBus::subscribe(EventType::TowerFired, [](const Event& e) {
        if (!e.textData.empty()) AudioManager::playSound(e.textData.c_str(), 0.1f);
    });

    EventBus::subscribe(EventType::TowerBuilt, [](const Event& e) {
        if (!e.textData.empty()) AudioManager::playSound(e.textData.c_str(), 0.1f);
    });

    for (const auto& tData : restoredTowers) {
        std::string type = tData.at("type").get<std::string>();
        int gx = tData.at("grid_x").get<int>();
        int gy = tData.at("grid_y").get<int>();
        int lvl = tData.at("level").get<int>();
        int modeInt = tData.at("target_mode").get<int>();

        auto restoredTower = std::make_unique<Tower>(gx, gy, type);
        restoredTower->forceLevel(lvl);
        restoredTower->setTargetMode(static_cast<TargetMode>(modeInt));

        m_world->entityManager->addTower(std::move(restoredTower));
        m_world->grid->setCellType(gx, gy, CellType::Tower);
    }

    m_world->recalculateAllPaths();

    std::cout << "[LoadGame] Мир и маршруты успешно восстановлены с учетом башен!" << std::endl;
    return true;
}