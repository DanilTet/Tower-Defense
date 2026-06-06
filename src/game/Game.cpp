#include "Game.h"
#include "resources/ResourceManager.h"
#include "renderer/SpriteRenderer.h"
#include "textures/Texture2D.h"
#include <glm/gtc/matrix_transform.hpp>
#include "Grid.h"
#include "Enemy.h"
#include "Tower.h"
#include "WaveManager.h"
#include <vector>
#include <iostream>
#include "../audio/AudioManager.h"
#include "ConfigManager.h"
#include "Pathfinder.h"

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



    // ПОЛЕ
	// Создаем сетку 10 на 7 клеток, каждая клетка 64 пикселя в размере
    m_gameGrid = std::make_unique<Grid>(10, 7, 64.0f, glm::vec2(20.0f, 20.0f));

	// Обновляем размер клеток сетки, чтобы она всегда занимала все окно, даже при изменении размера окна
    m_gameGrid->updateCellSize(this->width, this->height);

    // ТОЧКИ ПОЯВЛЕНИЯ И БАЗА

    m_spawners.push_back({ 0, 0 }); // спавнер
    m_bases.push_back({ 9, 6 }); // база

    m_gameGrid->setCellType(m_spawners[0].x, m_spawners[0].y, CellType::Spawner);
    m_gameGrid->setCellType(m_bases[0].x, m_bases[0].y, CellType::Base);

    // ИНИЦИАЛИЗАЦИЯ АЛГОРИТМА И ПОИСК ПУТИ
    m_pathfinder = std::make_unique<Pathfinder>(10, 7); // размері сетки

    // запуск алгоритма А*
    std::vector<glm::ivec2> calculatedPath = m_pathfinder->findPath(*m_gameGrid, m_spawners[0], m_bases[0]);

    // добавление точки спавна в начало пути
    calculatedPath.insert(calculatedPath.begin(), m_spawners[0]);

    // сохранение готового маршрута в список маршрутов
    m_paths.push_back(calculatedPath);
    m_levelPath = m_paths[0];
    // блокируем постройку башни 
    for (const auto& p : m_paths[0]) {
        if (m_gameGrid->getCellType(p.x, p.y) == CellType::Empty) {
            m_gameGrid->setCellType(p.x, p.y, CellType::Path);
        }
    }


    
    // МЕНЕДЖЕР ВОЛН
    m_waveManager = std::make_unique<WaveManager>();
    m_waveManager->loadLevel("res/levels/level_1.json"); // загружаем конфигурацию уровня

    // АУДИО
    AudioManager::playMusic("res/sounds/background.mp3"); // врубаем имбовый трек

    /*
    // МАРШРУТ БЛОКИРУЕТ КЛЕТКИ
    for (size_t i = 0; i < m_levelPath.size() - 1; ++i) {
        glm::ivec2 start = m_levelPath[i];
        glm::ivec2 end = m_levelPath[i + 1];

        // вычисляем направление шага -1, 0 или 1
        int stepX = (end.x > start.x) ? 1 : (end.x < start.x) ? -1 : 0;
        int stepY = (end.y > start.y) ? 1 : (end.y < start.y) ? -1 : 0;

        glm::ivec2 current = start;
        while (current != end + glm::ivec2(stepX, stepY)) {
            // помечаем клетку как путь запрещенную для постройки
            m_gameGrid->setCellType(current.x, current.y, CellType::Path);
            current.x += stepX;
            current.y += stepY;
        }
    }
    */
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

        //проверка клика по Ui
        glm::vec2 panelPos = getUIPanelPos();


        // если мышка внутри прямоугольника панели
        if (mouseX >= panelPos.x && mouseY >= panelPos.y) {

            // проверяем, по какой именно башне кликнули
            for (int i = 0; i < 3; ++i) {
                glm::vec2 iconPos = getTowerIconPos(i);

                if (mouseX >= iconPos.x && mouseX <= iconPos.x + UI_ICON_SIZE &&
                    mouseY >= iconPos.y && mouseY <= iconPos.y + UI_ICON_SIZE) {

                    // меняем выбранную башню
                    if (i == 0) m_selectedTowerType = TowerType::Basic;
                    else if (i == 1) m_selectedTowerType = TowerType::Sniper;
                    else if (i == 2) m_selectedTowerType = TowerType::Cannon;

                    //AudioManager::playSound("res/sounds/build.wav", 0.5f); // Звук клика по кнопке
                }
            }
            return;
        }

        // узнаем стоимость выбраной башни
        

        glm::ivec2 clickedCell = m_gameGrid->pixelToGrid(glm::vec2(mouseX, mouseY));
        
        // если клик поза поля
        if (clickedCell.x < 0 || clickedCell.x >= m_gameGrid->getWidth() ||
            clickedCell.y < 0 || clickedCell.y >= m_gameGrid->getHeight()) {
            return;
        }

        // логика апгрейда
        if (m_selectedTowerType == TowerType::None) {
            CellType clickedCellType = m_gameGrid->getCellType(clickedCell.x, clickedCell.y);

            if (clickedCellType == CellType::Tower) {
                // ищем в нашем векторе башню с такими же координатами YX
                for (auto& tower : m_towers) {
                    if (tower && tower->getGridX() == clickedCell.x && tower->getGridY() == clickedCell.y) {
                        int cost = tower->getUpgradeCost();
                        if (tower->upgrade(m_playerMoney)) {
                            std::cout << "Tower upgraded to level " << tower->getLevel() << "!" << std::endl;
                        }
                        else {
                            std::cout << "Upgrade failed! Max level or check money ($" << cost << ")" << std::endl;
                        }
                        break; // выходим
                    }
                }
            }
            return; // ОБЯЗАТЕЛЬНО выходим из метода, чтобы пустая рука не шла строить!
        }


        int currentCost = Tower::getStatsfromTowerType(m_selectedTowerType).cost;

        if (m_playerMoney >= currentCost && m_gameGrid->canBuildAt(clickedCell.x, clickedCell.y)) {

            // запоминаем какая клетка была до клика
            CellType oldCellType = m_gameGrid->getCellType(clickedCell.x, clickedCell.y);

            // виртуально ставим башню
            m_gameGrid->setCellType(clickedCell.x, clickedCell.y, CellType::Tower);
            //проверка маршрута
            std::vector<glm::ivec2> testPath = m_pathfinder->findPath(*m_gameGrid, m_spawners[0], m_bases[0]);

            // если пути нету значит игрок заблокировал маршрут
            if (testPath.empty()) {
                m_gameGrid->setCellType(clickedCell.x, clickedCell.y, oldCellType);
                std::cout << "Path Blocked! Cannot build here." << std::endl;
            }
            else {
                // если путь есть
                // забираем деньги
                m_playerMoney -= currentCost;
                // спавним башню
                auto newTower = std::make_unique<Tower>(clickedCell.x, clickedCell.y, m_selectedTowerType);
                m_towers.push_back(std::move(newTower));
                // звук постройки
                TowerStats stats = Tower::getStatsfromTowerType(m_selectedTowerType);
                AudioManager::playSound(stats.buildSound.c_str());

                // обновляем путь
                testPath.insert(testPath.begin(), m_spawners[0]);
                m_paths[0] = testPath;
                m_levelPath = testPath;

                // убираем старій путь
                for (int x = 0; x < m_gameGrid->getWidth(); ++x) {
                    for (int y = 0; y < m_gameGrid->getHeight(); ++y) {
                        if (m_gameGrid->getCellType(x, y) == CellType::Path) {
                            m_gameGrid->setCellType(x, y, CellType::Empty);
                        }
                    }
                }
                // записуем новій путь
                for (const auto& p : m_paths[0]) {
                    if (m_gameGrid->getCellType(p.x, p.y) == CellType::Empty) {
                        m_gameGrid->setCellType(p.x, p.y, CellType::Path);
                    }
                }

                // даем врагам новій путь
                for (auto& enemy : m_enemies) {
                    if (enemy) {
                        enemy->recalculatePath(m_pathfinder.get(), *m_gameGrid, m_bases[0]);
                    }
                }
            }
        }
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
        m_pathAnimationTimer += dt * (m_gameGrid->getCellSize() * 0.5f); // таймер анимации двигаем оп оп
    }
    // менеджер волн
    if (m_waveManager) {
        m_waveManager->update(dt, *this); // Передаем разницу во времени и ссылку на себя
    }
    
    // Пробегаемся по всему вектору активных башен на карте
    for (const auto& tower : m_towers) {
        if (tower) {
            tower->update(dt, m_enemies, m_projectiles, *m_gameGrid);
        }
    }
    // Пробегаемся по всему вектору активных врагов на карте
    for (const auto& enemy : m_enemies) {
        if (enemy) { // Если указатель на врага живой
            enemy->update(dt, *m_gameGrid); // Приказываем врагу пересчитать свою позицию с учетом deltaTime
        }
    }

    // если враг убит добавляем игроку деняк иначе отминаем от базі хп
    for (const auto& enemy : m_enemies) {
        if (enemy->isDead()) {
            m_playerMoney += enemy->getReward();
        }
        if (enemy->isReachedEnd()) {
            m_baseHealth -= 1;
        }
    }

    // обновляем пули
    for (const auto& proj : m_projectiles) {
        if (proj) {
            proj->update(dt, m_enemies, *m_gameGrid);
        }
    }

    // удаляем уничтоженіе пули
    m_projectiles.erase(
        std::remove_if(m_projectiles.begin(), m_projectiles.end(),
            [](const std::unique_ptr<Projectile>& proj) {
                return proj->isDestroyed();
            }),
        m_projectiles.end()
    );

	// Удаляем врагов, которые достигли конца пути или умерли
    m_enemies.erase(
        // remove_if сдвигает всех "финишировавших" врагов в хвост вектора и возвращает итератор на начало этого хвоста
        std::remove_if(m_enemies.begin(), m_enemies.end(),
            [](const std::unique_ptr<Enemy>& enemy) {
                return enemy->isReachedEnd() || enemy->isDead(); // Критерий удаления: метод вернул true
            }),
        m_enemies.end() // Метод erase физически отрезает этот хвост из памяти
    );
}

// Отрисовка кадра вызывается каждый кадр, после обновления логики
void Game::render() {
    // Малюем игровую сетку передавая туда рендерер, текстуру плитки, сдвиг и белый цвет тонирования
    m_gameGrid->draw(m_renderer.get(), m_grassTexture, m_cellTexture, { 1.0f, 1.0f, 1.0f });

    renderPathArrows(); // рисуем стрелочки пути

    // Пробегаемся по вектору активных врагов и рисуем каждого поверх сетки
    for (const auto& enemy : m_enemies) {
        if (enemy) { // Если враг существует
            enemy->render(m_renderer.get(), m_cellTexture, m_radiusTexture, m_gameGrid->getOffset(), *m_gameGrid); // Вызываем его метод отрисовки
        }
    }
    // Пробегаемся по вектору активных башен и рисуем каждого поверх сетки
    for (const auto& tower : m_towers) {
        if (tower) {
            tower->render(m_renderer.get(), m_cellTexture, m_radiusTexture, *m_gameGrid);
        }
    }

    // рисуем пули
    for (const auto& proj : m_projectiles) {
        if (proj) {
            proj->render(m_renderer.get(), m_cellTexture);
        }
    }
    // рисуем голограму
    renderHologram();

    // ОТРИСОВКА ИНТЕРФЕЙСА
    // Желтый цвет для денег
    m_textRenderer->RenderText("Деньги: " + std::to_string(m_playerMoney), 25.0f, 25.0f, 1.0f, glm::vec3(1.0f, 0.9f, 0.2f));

    // Красный цвет для здоровья базы (рисуем чуть ниже)
    m_textRenderer->RenderText("База: " + std::to_string(m_baseHealth) + " HP", 25.0f, 60.0f, 1.0f, glm::vec3(1.0f, 0.3f, 0.3f));

    m_textRenderer->RenderText("Волна: " + std::to_string(m_waveManager->getCurrentWaveNumber()), 25.0f, 95.0f, 1.0f, glm::vec3(0.5f, 1.0f, 0.5f));

    renderUI();
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
        for (const auto& enemy : m_enemies) {
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
void Game::spawnEnemy(EnemyType type, int spawnerIndex) {\
    // если спавнера не существуеты
    if (spawnerIndex >= m_paths.size() || m_paths[spawnerIndex].empty()) return;

    // выдаем врагу маршрут из списка путей
    auto newEnemy = std::make_unique<Enemy>(m_paths[spawnerIndex], *m_gameGrid, type);
    // Переносим владение (std::move) над этим указателем и пушим его в конец вектора m_enemies
    m_enemies.push_back(std::move(newEnemy));
}

void Game::startNextWave() {
    if (m_waveManager) {
        m_waveManager->startNextWave();
    }
}

void Game::renderPathArrows() {
    auto arrowTex = ResourceManager::getTexture("arrowTexture");
    if (!arrowTex) return;

    float cellSize = m_gameGrid->getCellSize(); // размер клетки
    float halfCell = cellSize / 2.0f; // половина клетки

    glm::vec2 arrowSize(cellSize * 0.20f, cellSize * 0.20f); // Размер стрелки — 25% от ширины клетки
    float arrowSpacing = cellSize * 0.50f; // Расстояние между стрелками — 60% от ширины клетки

    // СУПЕР КРУТОЙ ЄФЕКТ СВЕЧЕНИЯ
    // переключаем бленд-функцию в режим сложения цветов
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // белый цвет с альфа-прозрачностью чат гпт говорит 0.6f для мягкого свечения
    glm::vec3 arrowColor(1.0f, 1.0f, 1.0f);

    // пробегаемся по всем отрезкам пути
    for (size_t i = 0; i < m_levelPath.size() - 1; ++i) {
        // переводим узлы пути в пиксельные центры клеток
        glm::vec2 startPx = m_gameGrid->gridToPixel(m_levelPath[i].x, m_levelPath[i].y) + glm::vec2(halfCell);
        glm::vec2 endPx = m_gameGrid->gridToPixel(m_levelPath[i + 1].x, m_levelPath[i + 1].y) + glm::vec2(halfCell);

        glm::vec2 toEnd = endPx - startPx;
        float segmentLength = glm::length(toEnd);
        glm::vec2 direction = glm::normalize(toEnd);

        // вычисляем угол поворота стрелочки в зависимости от направления отрезка
        float angle = glm::degrees(atan2(direction.y, direction.x));
        // ефект ползанья
        // стартовое смещение для этого кадра
        float startOffset = fmod(m_pathAnimationTimer, arrowSpacing);

        // рисуем стрелочки вдоль всего текущего сегмента
        for (float d = startOffset; d < segmentLength; d += arrowSpacing) {
            glm::vec2 arrowPos = startPx + direction * d;

            // сдвигаем позицию, чтобы arrowPos была ровно по центру стрелочки
            glm::vec2 drawPos = arrowPos - (arrowSize / 2.0f);

            // отрисовка через спрайт-рендерер
            m_renderer->drawSprite(std::shared_ptr<Texture2D>(arrowTex, [](Texture2D*) {}), drawPos, arrowSize, angle, arrowColor);
        }
    }

    // возражение режима прозрачности
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Game::renderUI()
{
    glm::vec2 panelPos = getUIPanelPos(); // позиция менюшки

    // рисуем фон панели
    m_renderer->drawSprite(m_cellTexture, panelPos, glm::vec2(UI_PANEL_WIDTH, UI_PANEL_HEIGHT), 0.0f, glm::vec3(0.1f, 0.1f, 0.1f));
    // список башен для отрисовки
    std::vector<TowerType> availableTowers = { TowerType::Basic, TowerType::Sniper, TowerType::Cannon };

    // рисуем каждую башню
    for (size_t i = 0; i < availableTowers.size(); ++i) {
        TowerType currentType = availableTowers[i];
        TowerStats stats = Tower::getStatsfromTowerType(currentType);

        // позиция иконок
        glm::vec2 iconPos = getTowerIconPos(i);

        bool canAfford = (m_playerMoney >= stats.cost); // определяем хватает ли денег

        // цвет башни
        glm::vec3 towerColor(1.0f);
        if (currentType == TowerType::Basic) towerColor = glm::vec3(0.5f, 0.5f, 0.5f);
        else if (currentType == TowerType::Sniper) towerColor = glm::vec3(0.8f, 0.2f, 0.2f);
        else if (currentType == TowerType::Cannon) towerColor = glm::vec3(0.2f, 0.2f, 0.8f);

        // если денег не достаточно то делаем башню серой
        glm::vec3 drawColor;
        
        if (canAfford) {
            drawColor = towerColor;
        }
        else {
            drawColor = glm::vec3(0.2f, 0.2f, 0.2f);
        }

        // выделяем желтой рамкой выбраную башню
        if (m_selectedTowerType == currentType) {
            m_renderer->drawSprite(m_cellTexture, iconPos - glm::vec2(4.0f), glm::vec2(UI_ICON_SIZE + 8.0f), 0.0f, glm::vec3(1.0f, 1.0f, 0.0f));
        }

        // рисуем иконку башни
        m_renderer->drawSprite(m_cellTexture, iconPos, glm::vec2(UI_ICON_SIZE), 0.0f, drawColor);

        // текст цены
        std::string towerName;

        if (currentType == TowerType::Basic) {
            towerName = "Basic";
        }
        else if (currentType == TowerType::Sniper) {
            towerName = "Sniper";
        }
        else {
            towerName = "Cannon";
        }

        // если нету денег текст красный и зеленый если есть
        glm::vec3 textColor;

        if (canAfford) {
            textColor = glm::vec3(0.4f, 1.0f, 0.4f);
        }
        else {
            textColor = glm::vec3(1.0f, 0.3f, 0.3f);
        }

        m_textRenderer->RenderText(towerName + ": $" + std::to_string(stats.cost),
            iconPos.x - 5.0f, iconPos.y + UI_ICON_SIZE + 10.0f, 0.5f, textColor);

    }
}

// возвражает координаті верхнего левого угла всей панели (НАДА БУДЕТ ВОЗВОМЖНО ДЛЯ РЕСАЙЗА)
glm::vec2 Game::getUIPanelPos() const {
    return glm::vec2(this->width - UI_PANEL_WIDTH, this->height - UI_PANEL_HEIGHT);
}

// возвращает координаты конкретной иконки башни 0, 1 или 2
glm::vec2 Game::getTowerIconPos(int index) const {
    glm::vec2 panelPos = getUIPanelPos();
    return glm::vec2(
        panelPos.x + UI_OFFSET_X + (index * UI_ICON_PADDING),
        panelPos.y + UI_OFFSET_Y
    );
}

// рендерим голограму перед покупкой
void Game::renderHologram() {

    if (m_selectedTowerType == TowerType::None) {
        return;
    }

    if (m_levelPath.size() < 2) return;

    auto arrowTex = ResourceManager::getTexture("arrowTexture");
    if (!arrowTex) return;

    if (!m_gameGrid) return;

    // не рисуем башню поверх меню
    glm::vec2 panelPos = getUIPanelPos();
    if (m_currentMousePos.x >= panelPos.x && m_currentMousePos.y >= panelPos.y) {
        return;
    }

    // переводим пиксели мыши в координаты сетки
    glm::ivec2 gridPos = m_gameGrid->pixelToGrid(m_currentMousePos);

    // получаем данные выбранной башни
    TowerStats stats = Tower::getStatsfromTowerType(m_selectedTowerType);

    // проверяем, можно ли тут строить
    bool hasMoney = (m_playerMoney >= stats.cost);
    bool canBuildHere = m_gameGrid->canBuildAt(gridPos.x, gridPos.y);

    // задаем цвет голограмы
    glm::vec3 holoColor;

    if (hasMoney && canBuildHere) {
        holoColor = glm::vec3(0.2f, 1.0f, 0.2f); // Зелёная голограмма все ок
    }
    else {
        holoColor = glm::vec3(1.0f, 0.2f, 0.2f); // Красная голограмма проблема
    }

    // вычисляем пиксельные координаты для центрирования
    float cellSize = m_gameGrid->getCellSize();
    glm::vec2 cellPixelPos = m_gameGrid->gridToPixel(gridPos.x, gridPos.y);
    glm::vec2 cellCenter = cellPixelPos + glm::vec2(cellSize / 2.0f);

    // включаем режим свечения
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // отрисовка радиуса атаки
    float currentPixelRange = stats.range * cellSize;
    glm::vec2 radiusSize(currentPixelRange * 2.0f, currentPixelRange * 2.0f);
    glm::vec2 radiusPos = cellCenter - glm::vec2(currentPixelRange);

    // рисуем радиус цвета голограммы
    m_renderer->drawSprite(m_radiusTexture, radiusPos, radiusSize, 0.0f, holoColor);

    // отрисовка самой башни
    m_renderer->drawSprite(m_cellTexture, cellPixelPos, glm::vec2(cellSize), 0.0f, holoColor);

    // выключаем свечение
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}