#include "Enemy.h"
#include "../renderer/SpriteRenderer.h"
#include "../textures/Texture2D.h"
#include "Grid.h"
#include <iostream>

// конструктор который вызывается при спавне крипа
Enemy::Enemy(const std::vector<glm::ivec2>& gridPath, const Grid& grid, EnemyType type)
    : m_path(gridPath), // сохраняем ссылку на маршрут врага по клеткам сетки
    m_currentWayPoint(0), // начинаем с первой контрольной точки маршрута
    m_reachedEnd(false), // изначально враг не достиг конца маршрута
    m_type(type) // сохраняем тип врага
{
    EnemyStats stats = Enemy::getStatsfromEnemyType(type); // Получаем характеристики врага в зависимости от его типа
    m_speed = stats.speed; // сохраняем скорость врага
	m_health = stats.Maxhealth; // сохраняем здоровье врага

	// Если маршрут не пустой, то устанавливаем начальную позицию врага в пикселях на основе первой контрольной точки маршрута
    if (!m_path.empty()) {
        // Берем индексы x и y первой ячейки [0], переводим в экранные пиксели и сохраняем в m_pixelPos
        m_pixelPos = grid.gridToPixel(m_path[0].x, m_path[0].y);
    }
}

// Обновление логики и расчет движения
void Enemy::update(float dt, const Grid& grid) {
    // Если враг достиг конца или нету пути - выходим
    if (m_reachedEnd || m_path.empty()) return;

    // Определяем контрольную точку маршрута к которой идем
    glm::ivec2 targetCell = m_path[m_currentWayPoint];

	// переводим индексы контрольной точки в пиксели экрана
    glm::vec2 targetPixelPos = grid.gridToPixel(targetCell.x, targetCell.y);

	// Считаем вектор от текущей позиции врага до контрольной точки
    glm::vec2 toTarget = targetPixelPos - m_pixelPos;

	// Считаем расстояние до контрольной точки
    float distance = glm::length(toTarget);

	// Считаем сколько пикселей враг может пройти за этот кадр, умножая скорость на deltaTime
    float moveDistance = m_speed * dt;

    // если расстояние до чекпоинта меньше или равно шагу, который мы можем сделать
    if (distance <= moveDistance) {
        m_pixelPos = targetPixelPos; // приравниваем позицию врага к координатам цели
        m_currentWayPoint++; // теперь целью становится следующий чекпоинт

        // если индекс новой цели превысил размер массива маршрута — чекпоинты закончились
        if (m_currentWayPoint >= m_path.size()) {
            m_reachedEnd = true; // Выставляем флаг, что враг успешно закончил свой путь
            std::cout << "Враг прорвал туза!" << std::endl; // Сигнализируем в консоль
        }
    }
    // если до контрольной точк далеко
    else {
		// Нормализуем вектор до контрольной точки, чтобы получить направление движения
        glm::vec2 direction = glm::normalize(toTarget);
		// Двигаем врага в направлении контрольной точки на расстояние, которое он может пройти за этот кадр
        m_pixelPos += direction * moveDistance;
    }
}

// отрисовка врага на экране
void Enemy::render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> texture, glm::vec2 gridOffset, const Grid& grid) {
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

    // задаем цвет в зависимости от типа врага
    glm::vec3 color(1.0f);
    if (m_type == EnemyType::Basic) {
        color = glm::vec3(0.2f, 1.0f, 0.2f);
    }
    else if (m_type == EnemyType::Fast) {
        color = glm::vec3(1.0f, 1.0f, 0.2f);
    }
    else if (m_type == EnemyType::Tank) {
        color = glm::vec3(0.2f, 0.4f, 1.0f);
    }
    // Отправляем команду в SpriteRenderer
    renderer->drawSprite(texture, centeredPos, size, 0.0f, color);
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