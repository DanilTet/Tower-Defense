#include "GameplayState.h"
#include "GameStateManager.h"
#include "../resources/ResourceManager.h"
#include "../audio/AudioManager.h"
#include "../core/ConfigManager.h"
#include "../core/LevelManager.h"
#include "../core/EventBus.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

GameplayState::GameplayState(GameStateManager& stateManager, int windowWidth, int windowHeight, std::shared_ptr<SpriteRenderer> renderer, TextRenderer* textRenderer) : m_stateManager(stateManager),
	width(windowWidth),// запоминаем стартовую ширину окна
	height(windowHeight),// запоминаем стартовую высоту окна
	m_renderer(renderer), 
	m_textRenderer(textRenderer),
	m_mousePressedLastFrame(false), // изначально мышь не нажата флаг сброшен
	m_selectedTowerType("") // пустая рука в начале
{}

void GameplayState::cleanup() {
    EventBus::clear();
}

void GameplayState::init() {
	// загрузка конфигураций
	ConfigManager::loadConfigs("res/configs/towers.json", "res/configs/enemies.json", "res/configs/particles.json");

    // ЗАГРУЗКА ФАЙЛОВ ТЕКСТУРОК в VRAM
    ResourceManager::loadTexture("towerTexture", "res/textures/test_sprite.png"); // текстурка башни
    ResourceManager::loadTexture("grassTexture", "res/textures/spr_grass_02.png"); // текстурка тайла траві
    ResourceManager::loadTexture("radiusTexture", "res/textures/radius2.png"); // радиус атаки башни
    ResourceManager::loadTexture("arrowTexture", "res/textures/pathArrow.png"); // стрелочка пути
    ResourceManager::loadTexture("particleTexture", "res/textures/particle.png"); // партикл
    ResourceManager::loadTexture("enemyAtlas", "res/textures/enemyAtlas.png"); // атлас для врагов


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

    // ГРУЗИМ УРОВЕНЬ
    // грузим данные уровня
    std::string currentLevelPath = "res/levels/level_1.json";
    LevelMapData levelData = LevelManager::loadLevelMap(currentLevelPath);

    // ПОЛЕ
    // Создаем сетку 10 на 7 клеток, каждая клетка 64 пикселя в размере
    m_gameGrid = std::make_unique<Grid>(
        levelData.gridWidth,
        levelData.gridHeight,
        levelData.cellSize,
        glm::vec2(levelData.offsetX, levelData.offsetY)
    ); // передаем из levelData инфу про поле
    m_gameGrid->updateCellSize(this->width, this->height);


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

            // блокируем постройку на этом пути
            for (const auto& p : calculatedPath) {
                if (m_gameGrid->getCellType(p.x, p.y) == CellType::Empty) {
                    m_gameGrid->setCellType(p.x, p.y, CellType::Path);
                }
            }
        }
    }

    // сохраняем первый путь для голограммы постройки
    if (!m_paths.empty()) {
        m_levelPath = m_paths[0];
    }

    // МЕНЕДЖЕР ВОЛН
    m_waveManager = std::make_unique<WaveManager>();
    m_waveManager->loadLevel(currentLevelPath); // загружаем конфигурацию уровня

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
    //обновляем визуализатор пути
    if (m_gameGrid) m_pathVisualizer->update(dt, m_gameGrid->getCellSize());
    // чекаем менеджер волн
    if (m_waveManager) m_waveManager->update(dt, *this);
    // обновляем башни, пули и другое
    m_entityManager->update(dt, *m_gameGrid);

    // проверка на проигрыш
    if (m_playerStats.baseHealth <= 0) {
        m_playerStats.baseHealth = 0;
    }
}

void GameplayState::render() {
    // Малюем игровую сетку передавая туда рендерер, текстуру плитки, сдвиг и белый цвет тонирования
    m_gameGrid->draw(m_renderer.get(), m_grassTexture, m_cellTexture, { 1.0f, 1.0f, 1.0f });


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
    m_entityManager->render(m_renderer.get(), m_cellTexture, m_radiusTexture, arrowTexPtr, m_particleTexture, *m_gameGrid);

    // ОТРИСОВКА ИНТЕРФЕЙСА
    // отрисовка меню выбраной или выделеной как это называют башни
    m_towerMenuUI->render(m_selectedTowerOnMap, m_renderer.get(), m_textRenderer, m_cellTexture, *m_gameGrid);

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