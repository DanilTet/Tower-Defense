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

// Конструктор и деструктор
Game::Game(int width, int height)
    : width(width), // запоминаем стартовую ширину окна
	height(height), // запоминаем стартовую высоту окна
	m_mousePressedLastFrame(false), // изначально мышь не нажата флаг сброшен
    m_playerMoney(100000), // Выдаем в самом начале 100 деняк
    m_baseHealth(100),
    m_selectedTowerType(TowerType::None) {
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
        m_gameGrid->setCellType(spawner.x, spawner.y, CellType::Spawner);
    }

    // устанавливаем базы на карту
    for (const auto& base : m_bases) {
        m_gameGrid->setCellType(base.x, base.y, CellType::Base);
    }

    // ИНИЦИАЛИЗАЦИЯ АЛГОРИТМА И ПОИСК ПУТИ
    m_pathfinder = std::make_unique<Pathfinder>(levelData.gridWidth, levelData.gridHeight); // размері сетки

    // запуск алгоритма А*
    std::vector<glm::ivec2> calculatedPath = m_pathfinder->findPath(*m_gameGrid, m_spawners[0], m_bases[0]);

    // добавление точки спавна в начало пути
    calculatedPath.insert(calculatedPath.begin(), m_spawners[0]);

    // сохранение готового маршрута в список маршрутов
    m_paths.push_back(calculatedPath);
    m_levelPath = m_paths[0];
    // блокируем постройку башни УЖЕ НЕ АКТУАЛЬНЕНЬКО БАМ БАМ БАМ 
    for (const auto& p : m_paths[0]) {
        if (m_gameGrid->getCellType(p.x, p.y) == CellType::Empty) {
            m_gameGrid->setCellType(p.x, p.y, CellType::Path);
        }
    }


    
    // МЕНЕДЖЕР ВОЛН
    m_waveManager = std::make_unique<WaveManager>();
    m_waveManager->loadLevel(currentLevelPath); // загружаем конфигурацию уровня

    // АУДИО
    AudioManager::playMusic("res/sounds/background.mp3"); // врубаем имбовый трек

    // ИНТЕРФЕЙС
    m_buildPanel = std::make_unique<Buildpanel>(); // инициализация панельки для строительства
    m_placementUI = std::make_unique<PlacementUI>(); // инициализация голограммы для строительства
    m_pathVisualizer = std::make_unique<PathVisualizer>(); // инициализация стрелочек пути
    m_statsPanel = std::make_unique<StatsPanel>(); // инициализация панельки здровья деняг и всякого такого посмотрим че будет

    // Гейплей
    m_buildManager = std::make_unique<BuildManager>(); // инициализация абгрейдера и строителя башен
    m_entityManager = std::make_unique<EntityManager>(); // инициализация менеджера для отрисовки и обновление башен,врагов и пуль
}

// Обработка ввода вызывается каждый кадр
void Game::processInput(GLFWwindow* window, float dt) {
    
    // читаем мышку каждый кадр
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    m_currentMousePos = glm::vec2(mouseX, mouseY);

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        m_selectedTowerType = TowerType::None; // очищаем руку, голограмма сразу исчезнет
    }

	// Получаем состояние левой кнопки мыши (зажата она или отпущена)
    int mouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

    // Если ЛКМ зажата и на прошлом кадре она не была зажата, то фиксируем момент клика
    if (mouseState == GLFW_PRESS && !m_mousePressedLastFrame) {
        m_mousePressedLastFrame = true; // меняем флаг нажатия на кнопку
        // фиксируем координаты мышки
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        // если кликнули по менюшке то обновляем выбраную башню
        if (m_buildPanel->checkClick(mouseX, mouseY, this->width, this->height, m_selectedTowerType)) {
            return;
        }
        
        m_buildManager->tryBuildOrUpgrade(
            m_currentMousePos, // позиция мыши
            m_selectedTowerType,// выбраный тип башни
            m_playerMoney, // деньки игрока
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
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        spawnEnemy(EnemyType::Basic); // Моментально спавним нового врага
    }
    // Если на клавиатуре обнаружено нажатие на клавишу Ентер
    if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
        startNextWave(); // Моментально начинаем новую волну
    }
}

// Обновление игровой логики вызывается каждый кадр, после обработки ввода
void Game::update(float dt) {
    if (m_gameGrid) {
        m_pathVisualizer->update(dt, m_gameGrid->getCellSize()); // таймер анимации двигаем оп оп
    }
    // менеджер волн
    if (m_waveManager) {
        m_waveManager->update(dt, *this); // Передаем разницу во времени и ссылку на себя
    }
    
    // обновляем все башни, врагов и пули
    m_entityManager->update(dt, *m_gameGrid, m_playerMoney, m_baseHealth);
}

// Отрисовка кадра вызывается каждый кадр, после обновления логики
void Game::render() {
    // Малюем игровую сетку передавая туда рендерер, текстуру плитки, сдвиг и белый цвет тонирования
    m_gameGrid->draw(m_renderer.get(), m_grassTexture, m_cellTexture, { 1.0f, 1.0f, 1.0f });


    // стрелочки пути   
    Texture2D* arrowTex = ResourceManager::getTexture("arrowTexture"); // достаем там єти стрелочки из ресурсменеджера
    std::shared_ptr<Texture2D> arrowTexPtr(arrowTex, [](Texture2D*) {}); // и бахаем его в указатель
    m_pathVisualizer->renderPathArrows(
        m_renderer.get(),
        arrowTexPtr,
        m_levelPath,
        *m_gameGrid
    );; // рисуем стрелочки пути

    // рисуем все башни врагов и пули
    m_entityManager->render(m_renderer.get(), m_cellTexture, m_radiusTexture, *m_gameGrid);

    // ОТРИСОВКА ИНТЕРФЕЙСА
    
    // рендерим голограмму строительства
    bool hasPath = m_levelPath.size() >= 2;
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
        m_playerMoney,
        currentPanelPos,
        hasPath
    );

    m_statsPanel->drawstatsPanel(m_playerMoney, m_baseHealth, m_waveManager.get(), m_textRenderer.get());

    // отрисовка панельки для постройки
    m_buildPanel->BuildRenderUI(
        m_renderer.get(),
        m_textRenderer.get(),
        m_cellTexture,
        this->width,
        this->height,
        m_playerMoney,
        m_selectedTowerType
    );
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
void Game::spawnEnemy(EnemyType type, int spawnerIndex) {
    // если спавнера не существуеты
    if (spawnerIndex >= m_paths.size() || m_paths[spawnerIndex].empty()) return;

    // выдаем врагу маршрут из списка путей
    auto newEnemy = std::make_unique<Enemy>(m_paths[spawnerIndex], *m_gameGrid, type);
    // Переносим владение (std::move) над этим указателем и пушим его в конец вектора m_enemies
    m_entityManager->addEnemy(std::move(newEnemy));
}

void Game::startNextWave() {
    if (m_waveManager) {
        m_waveManager->startNextWave();
    }
}