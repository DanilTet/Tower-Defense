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
    Texture2D* cellTex = ResourceManager::getTexture("towerTexture"); // башня
    Texture2D* grassTex = ResourceManager::getTexture("grassTexture"); // тайл земли
    Texture2D* radTex = ResourceManager::getTexture("radiusTexture"); // радиус атаки башни
    Texture2D* partTex = ResourceManager::getTexture("particleTexture"); // партикл

    // указатель на тексту оборачиваем его в shared_ptr, чтобы управлять временем жизни текстуры
    m_cellTexture = std::shared_ptr<Texture2D>(cellTex, [](Texture2D*) {}); // башня
    m_grassTexture = std::shared_ptr<Texture2D>(grassTex, [](Texture2D*) {}); // тайл земли
    m_radiusTexture = std::shared_ptr<Texture2D>(radTex, [](Texture2D*) {}); // радиус атаки башни
    m_particleTexture = std::shared_ptr<Texture2D>(partTex, [](Texture2D*) {}); // партикл

    // атласы
    Texture2D* mainAtlasTex = ResourceManager::getTexture("mainAtlas");
    m_mainAtlas = std::shared_ptr<Texture2D>(mainAtlasTex, [](Texture2D*) {});

    Texture2D* enemyAtlasTex = ResourceManager::getTexture("enemyAtlas");
    m_enemyAtlas = std::shared_ptr<Texture2D>(enemyAtlasTex, [](Texture2D*) {});


    // если есть строка сохранения то загружаем его 
    if (!m_saveToLoad.empty()) {
        bool success = loadSavedGame(m_saveToLoad);
        m_saveToLoad = "";

        if (!success) {
            // файла нет
            std::cerr << "[ERROR] Файл сохранения не найден! Загрузка отменена." << std::endl;
            m_isValid = false;
            return;
        }
        else {
            return;
        }
    }
    // ГРУЗИМ УРОВЕНЬ
    // грузим данные уровня
    LevelMapData levelData = LevelManager::loadLevelMap(m_currentLevelPath);

    // ПОЛЕ
    // Создаем сетку 10 на 7 клеток, каждая клетка 64 пикселя в размере
    m_gameGrid = std::make_unique<Grid>(
        levelData.gridWidth,
        levelData.gridHeight,
        levelData.cellSize,
        glm::vec2(levelData.offsetX, levelData.offsetY)
    ); // передаем из levelData инфу про поле
    m_gameGrid->updateCellSize(this->width, this->height);

    if (!levelData.layout.empty()) {
        for (int y = 0; y < levelData.gridHeight; ++y) {
            for (int x = 0; x < levelData.gridWidth; ++x) {
                if (levelData.layout[y][x] == 1) {
                    m_gameGrid->setCellType(x, y, CellType::Path);
                }
                else if (levelData.layout[y][x] == 2) {
                    m_gameGrid->setCellType(x, y, CellType::Platform);
                }
                else if (levelData.layout[y][x] == 3) {
                    m_gameGrid->setCellType(x, y, CellType::Scenery);
                }
            }
        }
    }

    // ТОЧКИ ПОЯВЛЕНИЯ И БАЗА
    m_spawners = levelData.spawners;
    m_bases = levelData.bases;

    // устанавливаем спавнеры на карту
    for (const auto& spawner : m_spawners) {
        m_gameGrid->setCellType(spawner.pos.x, spawner.pos.y, CellType::Spawner);
    }

    // устанавливаем базы на карту
    for (const auto& base : m_bases) {
        m_gameGrid->setCellType(base.x, base.y, CellType::Base);
    }

    // ИНИЦИАЛИЗАЦИЯ АЛГОРИТМА И ПОИСК ПУТИ
    m_pathfinder = std::make_unique<Pathfinder>(levelData.gridWidth, levelData.gridHeight); // размері сетки

    m_paths.clear(); // очищаем старые пути

    // циклом прокладываем путь от КАЖДОГО спавнера до базы
    for (size_t i = 0; i < m_spawners.size(); ++i) {
        int baseIdx = m_spawners[i].targetBaseIndex;
        std::vector<glm::ivec2> calculatedPath;

        if (baseIdx == -1) {
            int minCost = 999999;
            float minEuclideanDist = 999999.0f;

            for (const auto& base : m_bases) {
                int currentCost = 0; // Создаем переменную для стоимости
                // Передаем currentCost в функцию, и она сама туда запишет ответ!
                auto path = m_pathfinder->findPath(*m_gameGrid, m_spawners[i].pos, base, currentCost);

                if (!path.empty()) {
                    // НАМ БОЛЬШЕ НЕ НУЖЕН ЦИКЛ ПО path ДЛЯ СТРОИТЕЛЬСТВА СТОИМОСТИ!
                    // currentCost уже равен правильной цифре (например, 150 или 194).

                    float euclideanDist = glm::distance(glm::vec2(m_spawners[i].pos), glm::vec2(base));

                    if (currentCost < minCost || (currentCost == minCost && euclideanDist < minEuclideanDist)) {
                        minCost = currentCost;
                        minEuclideanDist = euclideanDist;
                        calculatedPath = path;
                    }
                }
            }
        }

        else {
            // СТРОГО ПО ИНДЕКСУ ИЗ JSON
            if (baseIdx < 0 || baseIdx >= m_bases.size()) baseIdx = 0;
            int dummyCost = 0;
            calculatedPath = m_pathfinder->findPath(*m_gameGrid, m_spawners[i].pos, m_bases[baseIdx], dummyCost);
        }

        if (!calculatedPath.empty()) {
            calculatedPath.insert(calculatedPath.begin(), m_spawners[i].pos);
            m_paths.push_back(calculatedPath);
        }
    }

    // сохраняем первый путь для голограммы постройки
    if (!m_paths.empty()) {
        m_levelPath = m_paths[0];
    }

    // МЕНЕДЖЕР ВОЛН
    m_waveManager = std::make_unique<WaveManager>();
    m_waveManager->loadLevel(m_currentLevelPath); // загружаем конфигурацию уровня

    // АУДИО
    AudioManager::playMusic("res/sounds/background.mp3"); // врубаем имбовый трек

    // ИНТЕРФЕЙС
    m_buildPanel = std::make_unique<Buildpanel>(); // инициализация панельки для строительства
    m_buildPanel->initPanelData();
    m_placementUI = std::make_unique<PlacementUI>(); // инициализация голограммы для строительства
    m_pathVisualizer = std::make_unique<PathVisualizer>(); // инициализация стрелочек пути
    m_statsPanel = std::make_unique<StatsPanel>(); // инициализация панельки здровья деняг и всякого такого посмотрим че будет
    m_towerMenuUI = std::make_unique<TowerMenuUI>(); // инициализация панельки при клике на башню

    // Гейплей
    m_buildManager = std::make_unique<BuildManager>(); // инициализация абгрейдера и строителя башен
    m_entityManager = std::make_unique<EntityManager>(); // инициализация менеджера для отрисовки и обновление башен,врагов и пуль

    // КРУТАЯ ШИНА СОБЫТИЙ
    EventBus::clear(); // очищаем старые подписки

    // тут слушаем смерть врага
    EventBus::subscribe(EventType::EnemyDied, [this](const Event& e) {
        // e.value1 это деньки, e.value2 это очки
        this->m_playerStats.money += e.value1;
        this->m_playerStats.score += e.value2;
        if (!e.textData.empty()) {
            AudioManager::playSound(e.textData.c_str(), 0.1f);
        }
        });

    // тут слушаем прорыв врага на базу
    EventBus::subscribe(EventType::EnemyReachedBase, [this](const Event& e) {
        // e.value1 это урон по базе
        this->m_playerStats.baseHealth -= e.value1;

        // сразу проверяем на проигрыш здесь а не в update как было по лоховскому
        if (this->m_playerStats.baseHealth <= 0) {
            this->m_playerStats.baseHealth = 0;
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
    if (isKeyJustPressed(window, GLFW_KEY_M)) m_playerStats.money += 99999;
    if (isKeyJustPressed(window, GLFW_KEY_SPACE)) spawnEnemy("Basic");

    // читаем мышку каждый кадр
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    m_currentMousePos = glm::vec2(mouseX, mouseY);

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        m_selectedTowerType = ""; // очищаем руку, голограмма сразу исчезнет
    }

    // Получаем состояние левой кнопки мыши (зажата она или отпущена)
    int mouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

    // Если ЛКМ зажата и на прошлом кадре она не была зажата, то фиксируем момент клика
    if (mouseState == GLFW_PRESS && !m_mousePressedLastFrame) {
        m_mousePressedLastFrame = true; // меняем флаг нажатия на кнопку
        // фиксируем координаты мышки
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        // если кликнули по менюшке выбора башни то обновляем выбраную башню
        if (m_buildPanel->checkClick(mouseX, mouseY, this->width, this->height, m_selectedTowerType)) {
            m_selectedTowerOnMap = nullptr; // если выбрали новую башню для стройки то снимаем выделение с башни на карте
            return;
        }

        // чекаем клик по мею башни
        if (m_towerMenuUI->processClick(mouseX, mouseY, m_selectedTowerOnMap, m_buildManager.get(), m_playerStats, *m_entityManager, *m_gameGrid, *m_pathfinder, m_spawners, m_bases, m_paths, m_levelPath, m_selectedTowerOnMap)) {
            return; // если кликнули по кнопке, выходим
        }

        // чекаем клик по башне
        glm::ivec2 clickedCell = m_gameGrid->pixelToGrid(glm::vec2(mouseX, mouseY));
        Tower* clickedTower = m_entityManager->getTowerAt(clickedCell.x, clickedCell.y);

        if (clickedTower != nullptr) {
            m_selectedTowerOnMap = clickedTower;
            m_selectedTowerType = ""; // очищаем руку
            return;
        }
        else { // типа если кликнуть мимо башни то снять выделение
            m_selectedTowerOnMap = nullptr;
        }


        m_buildManager->tryBuildOrUpgrade(
            m_currentMousePos, // позиция мыши
            m_selectedTowerType,// выбраный тип башни
            m_playerStats, // статы игрока
            m_entityManager->getTowers(),// башни
            m_entityManager->getEnemies(),// враги
            *m_gameGrid, // сетка
            *m_pathfinder, // поиск пути
            m_spawners, // спавны
            m_bases,// базы блять
            m_paths,
            m_levelPath);
    }

    // Если мышка отпущена — сбрасываем флаг зажатия
    else if (mouseState == GLFW_RELEASE) {
        m_mousePressedLastFrame = false;
    }

    // Если на клавиатуре обнаружено нажатие на клавишу Пробел
    if (isKeyJustPressed(window, GLFW_KEY_SPACE)) {
        spawnEnemy("Basic");
    }
}

void GameplayState::update(float dt) {
    if (!m_isValid) {
        m_stateManager.setState(std::make_unique<MainMenuState>(m_stateManager, width, height, m_renderer, m_textRenderer));
        return;
    }

    //обновляем визуализатор пути
    if (m_gameGrid) m_pathVisualizer->update(dt, m_gameGrid->getCellSize());
    // чекаем менеджер волн
    if (m_waveManager) m_waveManager->update(dt, *this);
    // обновляем башни, пули и другое
    m_entityManager->update(dt, *m_gameGrid);

    // проверка на проигрыш
    if (m_playerStats.baseHealth <= 0) {
        m_playerStats.baseHealth = 0;
        m_stateManager.setState(std::make_unique<GameOverState>(m_stateManager, width, height, m_renderer, m_textRenderer, m_currentLevelPath));
        return;
    }

    if (m_waveManager->isAllWavesCompleted() && m_entityManager->getEnemies().empty()) {
        m_stateManager.setState(std::make_unique<VictoryState>(m_stateManager, width, height, m_renderer, m_textRenderer));
        return;
    }
}

void GameplayState::render() {
    if (!m_isValid) return;

    m_renderer->beginBatch(); // открываем пакет

    // Малюем игровую сетку передавая туда рендерер, текстуру плитки, сдвиг и белый цвет тонирования
    m_gameGrid->draw(m_renderer.get(), m_mainAtlas, { 1.0f, 1.0f, 1.0f });

    // стрелочки пути   
    Texture2D* arrowTex = ResourceManager::getTexture("arrowTexture"); // достаем там єти стрелочки из ресурсменеджера
    std::shared_ptr<Texture2D> arrowTexPtr(arrowTex, [](Texture2D*) {}); // и бахаем его в указатель
    for (const auto& path : m_paths) {
        m_pathVisualizer->renderPathArrows(
            m_renderer.get(),
            arrowTexPtr,
            path,
            *m_gameGrid
        );
    } // рисуем стрелочки пути

    // рисуем все башни врагов и пули
    m_entityManager->render(m_renderer.get(), m_mainAtlas, m_enemyAtlas, m_radiusTexture, arrowTexPtr, m_particleTexture, *m_gameGrid);

    m_renderer->flush();

    // ОТРИСОВКА ИНТЕРФЕЙСА
    //берем квадрат 1 на 1
    Texture2D* uiTex = ResourceManager::getTexture("uiBaseTexture");
    std::shared_ptr<Texture2D> uiTexPtr(uiTex, [](Texture2D*) {});
    // отрисовка меню выбраной или выделеной как это называют башни
    m_towerMenuUI->render(m_selectedTowerOnMap, m_renderer.get(), m_textRenderer, uiTexPtr, *m_gameGrid);

    // рендерим голограмму строительства
    bool hasPath = !m_paths.empty();
    // актуальная позиция менюшки
    glm::vec2 currentPanelPos = m_buildPanel->getUIPanelPos(this->width, this->height);

    //тут рисуем голограмму для строительсва 
    m_placementUI->renderHologram(
        m_renderer.get(),
        m_cellTexture,
        m_radiusTexture,
        *m_gameGrid,
        m_currentMousePos,
        m_selectedTowerType,
        m_playerStats,
        currentPanelPos,
        hasPath
    );

    m_statsPanel->drawStatsPanel(m_playerStats, m_waveManager.get(), m_textRenderer, this->width, this->height);

    // отрисовка панельки для постройки
    m_buildPanel->BuildRenderUI(
        m_playerStats,
        m_renderer.get(),
        m_textRenderer,
        m_cellTexture,
        this->width,
        this->height,
        m_selectedTowerType
    );

    m_renderer->endBatch(); // закрываем пакет
}

void GameplayState::spawnEnemy(const std::string& type, int spawnerIndex) {
    // если спавнера не существуеты
    if (spawnerIndex >= m_paths.size() || m_paths[spawnerIndex].empty()) return;

    // вытягиваем индекс базы из данных спавнера и передаем во врага
    int targetIdx = m_spawners[spawnerIndex].targetBaseIndex;
    auto newEnemy = std::make_unique<Enemy>(m_paths[spawnerIndex], *m_gameGrid, type, targetIdx);
    // Переносим владение (std::move) над этим указателем и пушим его в конец вектора m_enemies
    m_entityManager->addEnemy(std::move(newEnemy));
}

void GameplayState::startNextWave() {
    if (m_waveManager) m_waveManager->startNextWave();
}

void GameplayState::restartGame() {
    // сброс статы
    m_playerStats.reset();

    // сбарсываем выбраную башню
    m_selectedTowerType = "";

    // убиваем менеджера сущсностей
    m_entityManager = std::make_unique<EntityManager>();

    // рестертаем волны
    m_waveManager->loadLevel("res/levels/level_1.json");

    std::cout << "Game Restarted!" << std::endl;
}

bool GameplayState::isKeyJustPressed(GLFWwindow* window, int key) {
    // защита от выхода за предел массива
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

    if (m_gameGrid) {
        // запоминаем параметры старой сетки до перерасчета
        Grid oldGrid = *m_gameGrid;

        // приказываем сетке пересчитать размеры ячеек под новое окно
        m_gameGrid->updateCellSize(windowWidth, windowHeight);

        // обновляем позицию каждого врага
        for (const auto& enemy : m_entityManager->getEnemies()) {
            if (enemy) {
                enemy->recalculatePosition(oldGrid, *m_gameGrid);
            }
        }
    }
}

void GameplayState::saveGame(const std::string& saveName) {
    SaveManager saveManager;
    saveManager.writeSave(saveName, m_currentLevelPath, m_playerStats, *m_waveManager, *m_entityManager);
    std::cout << "[GameplayState] Игра сохранена в слот: " << saveName << std::endl;
}

bool GameplayState::loadSavedGame(const std::string& saveName) {
    SaveManager saveManager;
    std::string loadedLevelPath;
    PlayerStats loadedStats;
    int loadedWaveIndex = 0;
    nlohmann::json restoredTowers;

    // пытаемся прочитать файл сохранения
    if (!saveManager.readSave(saveName, loadedLevelPath, loadedStats, loadedWaveIndex, restoredTowers)) {
        std::cout << "[LoadGame] Не удалось загрузить файл: " << saveName << std::endl;
        return false;
    }

    std::cout << "[LoadGame] Файл прочитан. Начинаем восстановление стейта..." << std::endl;

    // очистка старых подписок
    cleanup();

    m_currentLevelPath = loadedLevelPath;
    m_selectedTowerType = "";
    m_selectedTowerOnMap = nullptr;

    // ИНИЦИАЛИЗАЦИЯ КАРТЫ И СЕТКИ ПОД ЗАГРУЖЕННЫЙ УРОВЕНЬ
    LevelMapData levelData = LevelManager::loadLevelMap(m_currentLevelPath);

    m_gameGrid = std::make_unique<Grid>(
        levelData.gridWidth,
        levelData.gridHeight,
        levelData.cellSize,
        glm::vec2(levelData.offsetX, levelData.offsetY)
    );
    m_gameGrid->updateCellSize(this->width, this->height);

    if (!levelData.layout.empty()) {
        for (int y = 0; y < levelData.gridHeight; ++y) {
            for (int x = 0; x < levelData.gridWidth; ++x) {
                if (levelData.layout[y][x] == 1) m_gameGrid->setCellType(x, y, CellType::Path);
                else if (levelData.layout[y][x] == 2) m_gameGrid->setCellType(x, y, CellType::Platform);
                else if (levelData.layout[y][x] == 3) m_gameGrid->setCellType(x, y, CellType::Scenery);
            }
        }
    }

    m_spawners = levelData.spawners;
    m_bases = levelData.bases;

    for (const auto& spawner : m_spawners) m_gameGrid->setCellType(spawner.pos.x, spawner.pos.y, CellType::Spawner);
    for (const auto& base : m_bases) m_gameGrid->setCellType(base.x, base.y, CellType::Base);

    // ВОССТАНОВЛЕНИЕ СТАТИСТИКИ И ИГРОВЫХ СИСТЕМ
    m_playerStats = loadedStats;

    m_waveManager = std::make_unique<WaveManager>();
    m_waveManager->loadLevel(m_currentLevelPath);
    m_waveManager->setCurrentWaveIndex(loadedWaveIndex);

    // Ренициализация менеджеров интерфейса и сущностей
    AudioManager::playMusic("res/sounds/background.mp3");
    m_buildPanel = std::make_unique<Buildpanel>();
    m_buildPanel->initPanelData();
    m_placementUI = std::make_unique<PlacementUI>();
    m_pathVisualizer = std::make_unique<PathVisualizer>();
    m_statsPanel = std::make_unique<StatsPanel>();
    m_towerMenuUI = std::make_unique<TowerMenuUI>();

    m_buildManager = std::make_unique<BuildManager>();
    m_entityManager = std::make_unique<EntityManager>();

    // ПОДПИСКИ НА СОБЫТИЯ ШИНЫ EventBus
    EventBus::clear();

    EventBus::subscribe(EventType::EnemyDied, [this](const Event& e) {
        this->m_playerStats.money += e.value1;
        this->m_playerStats.score += e.value2;
        if (!e.textData.empty()) AudioManager::playSound(e.textData.c_str(), 0.1f);
        });

    EventBus::subscribe(EventType::EnemyReachedBase, [this](const Event& e) {
        this->m_playerStats.baseHealth -= e.value1;
        if (this->m_playerStats.baseHealth <= 0) {
            this->m_playerStats.baseHealth = 0;
            std::cout << "GAME OVER!" << std::endl;
        }
        });

    EventBus::subscribe(EventType::TowerFired, [](const Event& e) {
        if (!e.textData.empty()) AudioManager::playSound(e.textData.c_str(), 0.1f);
        });

    EventBus::subscribe(EventType::TowerBuilt, [](const Event& e) {
        if (!e.textData.empty()) AudioManager::playSound(e.textData.c_str(), 0.1f);
        });

    // РЕКОНСТРУКЦИЯ БАШЕН И ИЗМЕНЕНИЕ ТИПА ЯЧЕЕК В СЕТКЕ
    for (const auto& tData : restoredTowers) {
        std::string type = tData.at("type").get<std::string>();
        int gx = tData.at("grid_x").get<int>();
        int gy = tData.at("grid_y").get<int>();
        int lvl = tData.at("level").get<int>();
        int modeInt = tData.at("target_mode").get<int>();

        auto restoredTower = std::make_unique<Tower>(gx, gy, type);
        restoredTower->forceLevel(lvl);
        restoredTower->setTargetMode(static_cast<TargetMode>(modeInt));

        m_entityManager->addTower(std::move(restoredTower));

        // блокируем клетку типом Tower
        m_gameGrid->setCellType(gx, gy, CellType::Tower);
    }

    // ПЕРЕРАСЧЕТ ПУТЕЙ
    m_pathfinder = std::make_unique<Pathfinder>(levelData.gridWidth, levelData.gridHeight);
    m_paths.clear();

    for (size_t i = 0; i < m_spawners.size(); ++i) {
        int baseIdx = m_spawners[i].targetBaseIndex;
        std::vector<glm::ivec2> calculatedPath;

        if (baseIdx == -1) {
            int minCost = 999999;
            float minEuclideanDist = 999999.0f;

            for (const auto& base : m_bases) {
                int currentCost = 0;
                auto path = m_pathfinder->findPath(*m_gameGrid, m_spawners[i].pos, base, currentCost);

                if (!path.empty()) {
                    float euclideanDist = glm::distance(glm::vec2(m_spawners[i].pos), glm::vec2(base));
                    if (currentCost < minCost || (currentCost == minCost && euclideanDist < minEuclideanDist)) {
                        minCost = currentCost;
                        minEuclideanDist = euclideanDist;
                        calculatedPath = path;
                    }
                }
            }
        }
        else {
            if (baseIdx < 0 || baseIdx >= m_bases.size()) baseIdx = 0;
            int dummyCost = 0;
            calculatedPath = m_pathfinder->findPath(*m_gameGrid, m_spawners[i].pos, m_bases[baseIdx], dummyCost);
        }

        if (!calculatedPath.empty()) {
            calculatedPath.insert(calculatedPath.begin(), m_spawners[i].pos);
            m_paths.push_back(calculatedPath);
        }
    }

    if (!m_paths.empty()) m_levelPath = m_paths[0];

    std::cout << "[LoadGame] Мир и маршруты успешно восстановлены с учетом башен!" << std::endl;
    return true;
}