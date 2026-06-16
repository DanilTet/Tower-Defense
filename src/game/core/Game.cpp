#include "Game.h"
#include "resources/ResourceManager.h"
#include "renderer/SpriteRenderer.h"
#include "textures/Texture2D.h"
#include <glm/gtc/matrix_transform.hpp>
#include "world/Grid.h"
#include "entities/Enemy.h"
#include "entities/Tower.h"
#include "WaveManager.h"
#include <vector>
#include <iostream>
#include "../audio/AudioManager.h"
#include "ConfigManager.h"
#include "world/Pathfinder.h"
#include "ui/BuildPanel.h"
#include "ui/PlacementUI.h"
#include "ui/StatsPanel.h"
#include "gameplay/BuildManager.h"
#include "core/LevelManager.h"
#include "gameplay/EntityManager.h"
#include "ui/TowerMenuUI.h"
#include "EventBus.h"

// Конструктор и деструктор
Game::Game(int width, int height)
    : width(width), // запоминаем стартовую ширину окна
	height(height),// запоминаем стартовую высоту окна
	m_mousePressedLastFrame(false), // изначально мышь не нажата флаг сброшен
    m_selectedTowerType(""),
    m_state(GameState::MainMenu) {
}

Game::~Game() {
	m_renderer.reset(); // удаляем рендерер и освобождаем память
	m_gameGrid.reset(); // удаляем сетку и освобождаем память
}

// Инициализация игры, загрузка ресурсов, настройка рендерера и создание сетки
void Game::init() {
    // загрузка конфигураций
    ConfigManager::loadConfigs("res/configs/towers.json", "res/configs/enemies.json");
	// загрузака файлов вершинного и фрагментного шейдера в видеокарту под именем "spriteShader"
    if (!ResourceManager::loadShader("spriteShader", "res/shaders/vertex_shader.vert", "res/shaders/fragment_shader.frag")) {
		std::cerr << "Failed to load shaders" << std::endl; // если загрузка не удалась, выводим ошибку в консоль
    }


    // ЗАГРУЗКА ФАЙЛОВ ТЕКСТУРОК в VRAM
    ResourceManager::loadTexture("towerTexture", "res/textures/test_sprite.png"); // текстурка башни
    ResourceManager::loadTexture("grassTexture", "res/textures/spr_grass_02.png"); // текстурка тайла траві
    ResourceManager::loadTexture("radiusTexture", "res/textures/radius2.png"); // радиус атаки башни
    ResourceManager::loadTexture("arrowTexture", "res/textures/pathArrow.png"); // стрелочка пути


    // Получаем указатель на текстура из ResourceManager
    Texture2D* cellTex = ResourceManager::getTexture("towerTexture"); // башня
    Texture2D* grassTex = ResourceManager::getTexture("grassTexture"); // тайл земли
    Texture2D* radTex = ResourceManager::getTexture("radiusTexture"); // радиус атаки башни

    // указатель на тексту оборачиваем его в shared_ptr, чтобы управлять временем жизни текстуры
    m_cellTexture = std::shared_ptr<Texture2D>(cellTex, [](Texture2D*) {}); // башня
    m_grassTexture = std::shared_ptr<Texture2D>(grassTex, [](Texture2D*) {}); // тайл земли
    m_radiusTexture = std::shared_ptr<Texture2D>(radTex, [](Texture2D*) {}); // радиус атаки башни
    

    // ШЕЙДЕР
	// Получаем указатель на шейдер из ResourceManager и создаем рендерер для спрайтов
    ShaderProgram* shader = ResourceManager::getShader("spriteShader");

	// Обертываем его в пустой shared_ptr, чтобы SpriteRenderer не удалял его при уничтожении, так как ResourceManager управляет временем жизни шейдера
    std::shared_ptr<ShaderProgram> shaderPtr(shader, [](ShaderProgram*) {});
    
    // Создаем обьект рендера и передаем туда шейдер
    m_renderer = std::make_unique<SpriteRenderer>(shaderPtr);

	// Создаем  ортографическую матрицу лево = 0, право = ширина окна, низ = высота окна, верх = 0, ближняя плоскость = -1, дальняя плоскость = 1
    // после єтого все координаты в игре будут в пикселях, где (0, 0) - это верхний левый угол окна, а (width, height) - это нижний правый угол окна
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(this->width), static_cast<float>(this->height), 0.0f, -1.0f, 1.0f);
    
	// Загружаем єту матрицу в рендерер, чтобы он использовал ее при отрисовке спрайтов
    m_renderer->setProjection(projection);

    // Включаем шейдер в работу на GPU
    shader->use();

    // Привязываем uniform-переменную текстуры в фрагментном шейдере к текстурному слоту №0
    glUniform1i(glGetUniformLocation(shader->getId(), "u_texture"), 0);



    // ИНИЦИАЛИЗАЦИЯ ТЕКСТА
    if (!ResourceManager::loadShader("textShader", "res/shaders/text_shader.vert", "res/shaders/text_shader.frag")) {
        std::cerr << "Failed to load text shaders" << std::endl;
    }
    ShaderProgram* textShader = ResourceManager::getShader("textShader");
    std::shared_ptr<ShaderProgram> textShaderPtr(textShader, [](ShaderProgram*) {});
    
    // Создаем рендерер текста
    m_textRenderer = std::make_unique<TextRenderer>(textShaderPtr, this->width, this->height);

    // Загружаем шрифт с размером 24 пикселя
    if (!m_textRenderer->Load("res/fonts/Roboto-Regular.ttf", 24)) {
        std::cerr << "Failed to load font!" << std::endl;
    }

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
            this->m_state = GameState::GameOver;
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

// Обработка ввода вызывается каждый кадр
void Game::processInput(GLFWwindow* window, float dt) {
    
    // управление в главном меню
    if (m_state == GameState::MainMenu) {
        if (isKeyJustPressed(window, GLFW_KEY_ENTER)) {
            m_state = GameState::Playing; // если бахнули ентер то играем
        }
        return; // дальше ничего не проверяем
    }

    // управление при проигрыше
    if (m_state == GameState::GameOver) {
        if (isKeyJustPressed(window, GLFW_KEY_R)) {
            restartGame();
        }
        return; // дальше ничего не проверяем
    }

    // пауща на p покачто потому что лень переписывать на ескейп
    if (isKeyJustPressed(window, GLFW_KEY_P)) {
        if (m_state == GameState::Playing) m_state = GameState::Paused;
        else if (m_state == GameState::Paused) m_state = GameState::Playing;
    }

    // если пауза то дальше ничего не проверяем и не трекаем мышь
    if (m_state == GameState::Paused) {
        return;
    }
    if (m_state == GameState::Playing) {
        // Если на клавиатуре обнаружено нажатие на клавишу Ентер
        if (isKeyJustPressed(window, GLFW_KEY_ENTER)) {
            startNextWave(); // Моментально начинаем новую волну
        }

        // секретній чит код на деньки
        if (isKeyJustPressed(window, GLFW_KEY_M)) {
            m_playerStats.money += 99999;
        }
    }

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

// Обновление игровой логики вызывается каждый кадр, после обработки ввода
void Game::update(float dt) {
    switch (m_state) {
    case GameState::MainMenu:
        // в главном меню ничего пока что не происходит
        break;

    case GameState::Playing:
        //обновляем визуализатор пути
        if (m_gameGrid) m_pathVisualizer->update(dt, m_gameGrid->getCellSize());
        // чекаем менеджер волн
        if (m_waveManager) m_waveManager->update(dt, *this);
        // обновляем башни, пули и другое
        m_entityManager->update(dt, *m_gameGrid);

        // проверка на проигрыш
        if (m_playerStats.baseHealth <= 0) {
            m_playerStats.baseHealth = 0;
            m_state = GameState::GameOver; //переключаем стейт
        }
        break;

    case GameState::Paused:
    case GameState::GameOver:
        // логика заморожена 
        break;
    }
}

// Отрисовка кадра вызывается каждый кадр, после обновления логики
void Game::render() {

    // если в меню то рисуем меню и всьо
    if (m_state == GameState::MainMenu) {
        m_textRenderer->RenderText("DVIYESHNYKY STUDIOS", this->width / 2.0f - 180.0f, this->height / 2.0f - 50.0f, 1.5f, glm::vec3(1.0f, 1.0f, 0.0f));
        m_textRenderer->RenderText("Press ENTER to Start", this->width / 2.0f - 130.0f, this->height / 2.0f + 20.0f, 1.0f, glm::vec3(1.0f));
        return; // дальше ниче не рисуем
    }

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
    m_entityManager->render(m_renderer.get(), m_cellTexture, m_radiusTexture, arrowTexPtr, *m_gameGrid);

    // ОТРИСОВКА ИНТЕРФЕЙСА
    // отрисовка меню выбраной или выделеной как это называют башни
    if (m_state == GameState::Playing) {
        m_towerMenuUI->render(m_selectedTowerOnMap, m_renderer.get(), m_textRenderer.get(), m_cellTexture, *m_gameGrid);
    }

    // рендерим голограмму строительства
    bool hasPath = !m_paths.empty();
    // актуальная позиция менюшки
    glm::vec2 currentPanelPos = m_buildPanel->getUIPanelPos(this->width, this->height);

    

    // рисуем интерфейс в зависимо от состояния
    if (m_state == GameState::Playing) {
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

        m_statsPanel->drawStatsPanel(m_playerStats, m_waveManager.get(), m_textRenderer.get(), this->width, this->height);

        // отрисовка панельки для постройки
        m_buildPanel->BuildRenderUI(
            m_playerStats,
            m_renderer.get(),
            m_textRenderer.get(),
            m_cellTexture,
            this->width,
            this->height,
            m_selectedTowerType
        );
    }
    // тут пауза
    else if (m_state == GameState::Paused) {
        // затычка просто
        m_textRenderer->RenderText("PAUSED", this->width / 2.0f - 80.0f, this->height / 2.0f, 2.0f, glm::vec3(1.0f, 0.8f, 0.0f));
    }

    // экран проигрыша хотя скорее тектс
    else if (m_state == GameState::GameOver) {
        // оверлей проигрыша
        m_textRenderer->RenderText("GAME OVER", this->width / 2.0f - 100.0f, this->height / 2.0f - 50.0f, 2.0f, glm::vec3(1.0f, 0.1f, 0.1f));
        m_textRenderer->RenderText("Press 'R' to Restart", this->width / 2.0f - 130.0f, this->height / 2.0f + 20.0f, 1.0f, glm::vec3(1.0f));
    }
}

// Изменение размеров окна: вызывается системным колбеком из main.cpp
void Game::resize(int width, int height) {
    this->width = width; // Перезаписываем новое значение ширины игры
    this->height = height; // Перезаписываем новое значение высоты игры

    if (m_renderer) { // Если рендерер уже создан
        // Заново создаем ортографическую матрицу под новые физические размеры окна
        glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, -1.0f, 1.0f);
        m_renderer->setProjection(projection); // Загружаем обновленную матрицу в рендерер
    }
    if (m_gameGrid) { // Если сетка существует
		// Сохраняем старую сетку
        Grid oldGrid = *m_gameGrid;

        // Приказываем сетке пересчитать размеры ячеек под новое окно
        m_gameGrid->updateCellSize(width, height); 
        
		// Обновляем позицию каждого врага, чтобы они оставались на своих клетках даже при изменении размера окна и размера клеток
        for (const auto& enemy : m_entityManager->getEnemies()) {
            if (enemy) {
                enemy->recalculatePosition(oldGrid, *m_gameGrid);   
            }
        }
    }

    if (m_textRenderer) {
        glm::mat4 textProjection = glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f);
        m_textRenderer->updateProjection(textProjection);
    }
}

// функция для спавна врага
void Game::spawnEnemy(const std::string& type, int spawnerIndex) {
    // если спавнера не существуеты
    if (spawnerIndex >= m_paths.size() || m_paths[spawnerIndex].empty()) return;

    // вытягиваем индекс базы из данных спавнера и передаем во врага
    int targetIdx = m_spawners[spawnerIndex].targetBaseIndex;
    auto newEnemy = std::make_unique<Enemy>(m_paths[spawnerIndex], *m_gameGrid, type, targetIdx);
    // Переносим владение (std::move) над этим указателем и пушим его в конец вектора m_enemies
    m_entityManager->addEnemy(std::move(newEnemy));
}

void Game::startNextWave() {
    if (m_waveManager) {
        m_waveManager->startNextWave();
    }
}


void Game::restartGame() {
    // сброс статы
    m_playerStats.reset();
    m_state = GameState::Playing;

    // сбарсываем выбраную башню
    m_selectedTowerType = "";

    // убиваем менеджера сущсностей
    m_entityManager = std::make_unique<EntityManager>();

    // рестертаем волны
    m_waveManager->loadLevel("res/levels/level_1.json");

    std::cout << "Game Restarted!" << std::endl;
}


bool Game::isKeyJustPressed(GLFWwindow* window, int key) {
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