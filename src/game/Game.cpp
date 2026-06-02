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

// Конструктор и деструктор
Game::Game(int width, int height)
    : width(width), // запоминаем стартовую ширину окна
	height(height), // запоминаем стартовую высоту окна
	m_mousePressedLastFrame(false), // изначально мышь не нажата флаг сброшен
    m_playerMoney(100000), // Выдаем в самом начале 100 деняк
    m_baseHealth(100){ 
}

Game::~Game() {
	m_renderer.reset(); // удаляем рендерер и освобождаем память
	m_gameGrid.reset(); // удаляем сетку и освобождаем память
}

// Инициализация игры, загрузка ресурсов, настройка рендерера и создание сетки
void Game::init() {
	// загрузака файлов вершинного и фрагментного шейдера в видеокарту под именем "spriteShader"
    if (!ResourceManager::loadShader("spriteShader", "res/shaders/vertex_shader.vert", "res/shaders/fragment_shader.frag")) {
		std::cerr << "Failed to load shaders" << std::endl; // если загрузка не удалась, выводим ошибку в консоль
    }


    // ЗАГРУЗКА ФАЙЛОВ ТЕКСТУРОК в VRAM
    ResourceManager::loadTexture("towerTexture", "res/textures/test_sprite.png"); // текстурка башни
    ResourceManager::loadTexture("grassTexture", "res/textures/spr_grass_02.png"); // текстурка тайла траві
    ResourceManager::loadTexture("radiusTexture", "res/textures/radius2.png"); // радиус атаки башни
    
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

    // ПУТЬ ВРАГОВ
    // Масив контрольных точек (x, y) по которым идет враг
    m_levelPath = { {0, 0}, {3, 0}, {3, 3}, {6, 3}, {6, 6}, {9, 6} }; // маршрут врага по клеткам сетки
    
    // МЕНЕДЖЕР ВОЛН
    m_waveManager = std::make_unique<WaveManager>();
    m_waveManager->loadLevel("res/levels/level_1.json"); // загружаем конфигурацию уровня

    // АУДИО
    AudioManager::playMusic("res/sounds/background.mp3"); // врубаем имбовый трек
}

// Обработка ввода вызывается каждый кадр
void Game::processInput(GLFWwindow* window, float dt) {
	// Получаем состояние левой кнопки мыши (зажата она или отпущена)
    int mouseState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

    // Если ЛКМ зажата и на прошлом кадре она не была зажата, то фиксируем момент клика
    if (mouseState == GLFW_PRESS && !m_mousePressedLastFrame) {
		m_mousePressedLastFrame = true; // обновляем флаг, что мышь сейчас зажата
        double mouseX, mouseY;
		glfwGetCursorPos(window, &mouseX, &mouseY); // получаем текущие координаты мыши в пикселях относительно верхнего левого угла окна

        // Переводим пиксели экрана в индексы ячейки с учетом сдвига сетки
        glm::ivec2 clickedCell = m_gameGrid->pixelToGrid(glm::vec2(mouseX, mouseY));

        // проверям можно ли в ячейке с этими индексами строить здания и хватает ли денег
        if (m_playerMoney >= 50 && m_gameGrid->canBuildAt(clickedCell.x, clickedCell.y)) {
            m_playerMoney -= 50; // списываем деньги

            // Если можно — принудительно меняем тип этой ячейки на Tower
            m_gameGrid->setCellType(clickedCell.x, clickedCell.y, CellType::Tower);
            // створюємо об'єкт башні
            auto newTower = std::make_unique<Tower>(clickedCell.x, clickedCell.y, TowerType::Basic);
            m_towers.push_back(std::move(newTower));
            // Выводим отладочный лог в консоль
            std::cout << "tower placed! money: " << m_playerMoney << std::endl;
        }
        else if (m_playerMoney < 50) {
            // если денег не хватает
            std::cout << "Not enough money! You have only: " << m_playerMoney << std::endl;
        }
    }
    // Если мышка отпущена — сбрасываем флаг зажатия, открывая возможность для нового клика
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

    // ОТРИСОВКА ИНТЕРФЕЙСА
    // Желтый цвет для денег
    m_textRenderer->RenderText("Деньги: " + std::to_string(m_playerMoney), 25.0f, 25.0f, 1.0f, glm::vec3(1.0f, 0.9f, 0.2f));

    // Красный цвет для здоровья базы (рисуем чуть ниже)
    m_textRenderer->RenderText("База: " + std::to_string(m_baseHealth) + " HP", 25.0f, 60.0f, 1.0f, glm::vec3(1.0f, 0.3f, 0.3f));

    m_textRenderer->RenderText("Волна: " + std::to_string(m_waveManager->getCurrentWaveNumber()), 25.0f, 95.0f, 1.0f, glm::vec3(0.5f, 1.0f, 0.5f));
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

// функция для спавна врага (принимает скорость)
void Game::spawnEnemy(EnemyType type) {
    // Выделяем память под новый объект Enemy, передавая ему ЕДИНЫЙ путь m_levelPath по константной ссылке
    auto newEnemy = std::make_unique<Enemy>(m_levelPath, *m_gameGrid, type); // спавн нового врага
    // Переносим владение (std::move) над этим указателем и пушим его в конец вектора m_enemies
    m_enemies.push_back(std::move(newEnemy));
}

void Game::startNextWave() {
    if (m_waveManager) {
        m_waveManager->startNextWave();
    }
}