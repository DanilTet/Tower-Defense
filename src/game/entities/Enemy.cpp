#include "Enemy.h"
#include "../renderer/SpriteRenderer.h"
#include "../textures/Texture2D.h"
#include "world/Grid.h"
#include <iostream>
#include "core/ConfigManager.h"
#include "world/Pathfinder.h"
#include "../../resources/ResourceManager.h"

int Enemy::s_nextId = 0; // инициализация общего счетчика

EnemyStats Enemy::getStatsfromEnemyType(const std::string& type) {
    return ConfigManager::getEnemyStats(type);
}

// конструктор который вызывается при спавне крипа
Enemy::Enemy(const std::vector<glm::ivec2>& gridPath, const Grid& grid, const std::string& type, int targetBaseIndex)
    : m_path(gridPath), // сохраняем ссылку на маршрут врага по клеткам сетки
    m_currentWayPoint(0), // начинаем с первой контрольной точки маршрута
    m_reachedEnd(false), // изначально враг не достиг конца маршрута
    m_type(type), // сохраняем тип врага
    m_distanceTraveled(0.0f),// инициализация одометра
    m_targetBaseIndex(targetBaseIndex) // СОХРАНЯЕМ ИНДЕКС БАЗЫ
{
    m_id = s_nextId++; // выдаем уникальный айди

    EnemyStats stats = Enemy::getStatsfromEnemyType(type); // Получаем характеристики врага в зависимости от его типа
    m_speed = stats.speed; // сохраняем скорость врага
	m_health = stats.Maxhealth; // сохраняем здоровье врага
    m_reward = stats.reward; // сохраняем награду за убийство врага
    m_color = stats.color;
    m_deathSound = stats.deathSound;
    m_deathParticle = stats.deathParticle;

    m_radiusMultiplier = stats.sizeScale / 2.0f; // хитбокс
	// Если маршрут не пустой, то устанавливаем начальную позицию врага в пикселях на основе первой контрольной точки маршрута
    if (!m_path.empty()) {
        // Берем индексы x и y первой ячейки [0], переводим в экранные пиксели и сохраняем в m_pixelPos
        m_pixelPos = grid.gridToPixel(m_path[0].x, m_path[0].y);
    }

    for(const auto& pair : stats.animations) {
        m_animator.addAnimation(pair.first, pair.second);
    }
    m_animator.play("Walk");
}

// Обновление логики и расчет движения
void Enemy::update(float dt, const Grid& grid) {
    // Если враг достиг конца или нету пути - выходим
    if (m_reachedEnd || m_path.empty()) return;

    m_animator.update(dt); // прокрутка анимации

    // Определяем контрольную точку маршрута к которой идем
    glm::ivec2 targetCell = m_path[m_currentWayPoint];

	// переводим индексы контрольной точки в пиксели экрана
    glm::vec2 targetPixelPos = grid.gridToPixel(targetCell.x, targetCell.y);

	// Считаем вектор от текущей позиции врага до контрольной точки
    glm::vec2 toTarget = targetPixelPos - m_pixelPos;

	// Считаем расстояние до контрольной точки
    float distance = glm::length(toTarget);
    
    // перевод скоросты в пиксели в секунду
    float currentPixelSpeed = m_speed * grid.getCellSize();

	// Считаем сколько пикселей враг может пройти за этот кадр, умножая скорость на deltaTime
    float moveDistance = currentPixelSpeed * dt;

    // если расстояние до чекпоинта меньше или равно шагу, который мы можем сделать
    if (distance <= moveDistance) {
        m_distanceTraveled += distance;// плюсуем пройденое расстояние за кадр
        m_pixelPos = targetPixelPos; // приравниваем позицию врага к координатам цели
        m_currentWayPoint++; // теперь целью становится следующий чекпоинт

        // если индекс новой цели превысил размер массива маршрута — чекпоинты закончились
        if (m_currentWayPoint >= m_path.size()) {
            m_reachedEnd = true; // Выставляем флаг, что враг успешно закончил свой путь
            std::cout << "enemy came to base!" << std::endl; // Сигнализируем в консоль
        }
    }
    // если до контрольной точк далеко
    else {
		// Нормализуем вектор до контрольной точки, чтобы получить направление движения
        glm::vec2 direction = glm::normalize(toTarget);
		// Двигаем врага в направлении контрольной точки на расстояние, которое он может пройти за этот кадр
        m_pixelPos += direction * moveDistance;
        m_distanceTraveled += moveDistance; // плюсуем пройденое расстояние за кадр
    }
}

// отрисовка врага на экране
void Enemy::render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> texture, std::shared_ptr<Texture2D> radiusTex, glm::vec2 gridOffset, const Grid& grid) {
	if (m_reachedEnd) return; // если враг достиг конца пути, то не рисуем его

	// Получаем характеристики врага в зависимости от его типа, чтобы использовать их для отрисовки (например, размер спрайта)
    EnemyStats stats = Enemy::getStatsfromEnemyType(m_type);
    float cellSize = grid.getCellSize();

	// Вычисляем размер спрайта врага в пикселях, умножая базовый размер клетки на масштаб размера из характеристик врага
    float enemySizeDim = cellSize * stats.sizeScale;
    glm::vec2 size(enemySizeDim, enemySizeDim);

    // Ставим врага по центру
    float padding = (cellSize - enemySizeDim) / 2.0f;
    glm::vec2 centeredPos = m_pixelPos + glm::vec2(padding, padding);

    // Отправляем команду в SpriteRenderer
    Texture2D* enemyTex = ResourceManager::getTexture(stats.textureId);
    if (!enemyTex) {
        enemyTex = ResourceManager::getTexture("towerTexture");
    }
    std::shared_ptr<Texture2D> enemyTexPtr(enemyTex, [](Texture2D*) {});

    SpriteUV currentFrameUV = m_animator.getCurrentUV();

    renderer->drawSprite(enemyTexPtr, centeredPos, size, 0.0f, m_color, currentFrameUV);

    //ЧАТО ГПТИШНОЕ ГОВНО КОТОРОЕ ПОТОМ УБРАТЬ
    // //ЧАТО ГПТИШНОЕ ГОВНО КОТОРОЕ ПОТОМ УБРАТЬ
    // //ЧАТО ГПТИШНОЕ ГОВНО КОТОРОЕ ПОТОМ УБРАТЬ
    // --- ОТРИСОВКА ПОЛОСКИ ЗДОРОВЬЯ (HEALTH BAR) ---

    float hpPercent = static_cast<float>(m_health) / static_cast<float>(stats.Maxhealth);
    if (hpPercent < 0.0f) hpPercent = 0.0f;

    float barWidth = enemySizeDim;
    float barHeight = 6.0f;

    glm::vec2 barPos = centeredPos + glm::vec2(0.0f, -10.0f);

    renderer->drawSprite(texture, barPos, glm::vec2(barWidth, barHeight), 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    renderer->drawSprite(texture, barPos, glm::vec2(barWidth * hpPercent, barHeight), 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));

    // --- ОТРИСОВКА ХИТБОКСА (ДЕБАГ) ---

    CircleCollider collider = getCollider(grid);

    glm::vec2 hitboxSize(collider.radius * 2.0f, collider.radius * 2.0f);
    glm::vec2 hitboxPos = collider.center - glm::vec2(collider.radius, collider.radius);

    // ИСПРАВЛЕНИЕ 2: drawSprite с маленькой буквы и передаем radiusTex без звездочки (это shared_ptr)
    renderer->drawSprite(radiusTex, hitboxPos, hitboxSize, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
}

void Enemy::recalculatePosition(const Grid& oldGrid, const Grid& newGrid) {
    //пропорционально пересчитываем позицию врага в пикселях на основе его текущей контрольной точки маршрута, чтобы он всегда был точно на клетке, даже если размер клеток изменится при ресайзе окна
    if (m_currentWayPoint > 0 && m_currentWayPoint < m_path.size()) {
        //берем предыдущую точку и целевую
        glm::ivec2 prevCell = m_path[m_currentWayPoint - 1];
        glm::ivec2 targetCell = m_path[m_currentWayPoint];

		// Получаем их пиксельные координаты в старой сетке
        glm::vec2 oldPrevPixel = oldGrid.gridToPixel(prevCell.x, prevCell.y);
        glm::vec2 oldTargetPixel = oldGrid.gridToPixel(targetCell.x, targetCell.y);

        // Считаем общий путь (в пикселях) отрезка и сколько из него враг уже прошёл 
        float oldTotalDist = glm::distance(oldTargetPixel, oldPrevPixel);
        float oldPassedDist = glm::distance(m_pixelPos, oldPrevPixel);

        // Фактор прогресса (от 0.0 до 1.0). Сколько процентов пути пройдено?
        float progress = (oldTotalDist > 0.0f) ? (oldPassedDist / oldTotalDist) : 0.0f;

        // Новые пиксельные позиции на ПОСЛЕ-ресайзовой сетке
        glm::vec2 newPrevPixel = newGrid.gridToPixel(prevCell.x, prevCell.y);
        glm::vec2 newTargetPixel = newGrid.gridToPixel(targetCell.x, targetCell.y);

        // Находим новую пиксельную позицию врага, сохраняя тот же самый процент прогресса!
        m_pixelPos = glm::mix(newPrevPixel, newTargetPixel, progress);
    }
    else if (m_currentWayPoint == 0 && !m_path.empty()) {
        m_pixelPos = newGrid.gridToPixel(m_path[0].x, m_path[0].y);
    }
}

// функция которая дает хитбокс зависимо от размера окна
CircleCollider Enemy::getCollider(const Grid& grid) const {
    // умножаем коэффициент на текущий размер клетки из сетки
    float currentRadius = m_radiusMultiplier * grid.getCellSize();

    float halfCell = grid.getCellSize() / 2.0f;
    glm::vec2 center = m_pixelPos + glm::vec2(halfCell, halfCell);

    // возвращаем готовую структуру центр врага и его динамический радиус
    return { center, currentRadius };
}
// функция для нахождения новго пути
void Enemy::recalculatePath(Pathfinder* pathfinder, const Grid& grid, const std::vector<glm::ivec2>& bases) {
    glm::ivec2 currentGridPos = grid.pixelToGrid(m_pixelPos);
    std::vector<glm::ivec2> bestPath;

    if (m_targetBaseIndex == -1) {
        // ищем кратчайший путь с учетом стоимости
        int minCost = 999999;

        for (const auto& base : bases) {
            int currentCost = 0; // переменная для сохранения стоимости
            auto path = pathfinder->findPath(grid, currentGridPos, base, currentCost);
            if (!path.empty()) {
                if (currentCost < minCost) {
                    minCost = currentCost;
                    bestPath = path;
                }
            }
        }
    }
    else {
        int idx = m_targetBaseIndex;
        if (idx < 0 || idx >= bases.size()) idx = 0;
        int dummyCost = 0;
        bestPath = pathfinder->findPath(grid, currentGridPos, bases[idx], dummyCost);
    }

    if (!bestPath.empty()) {
        m_path = bestPath;
        m_currentWayPoint = 0;
    }
}