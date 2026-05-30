#include "Enemy.h"
#include "../renderer/SpriteRenderer.h"
#include "../textures/Texture2D.h"
#include "Grid.h"
#include <iostream>

// конструктор который вызывается при спавне крипа
Enemy::Enemy(const std::vector<glm::ivec2>& gridPath, const Grid& grid, float speed)
	: m_path(gridPath), // сохраняем ссылку на маршрут врага по клеткам сетки
	m_currentWayPoint(0), // начинаем с первой контрольной точки маршрута
	m_speed(speed), // запоминаем скорость врага в пикселях в секунду
	m_reachedEnd(false) // изначально враг не достиг конца маршрута
{
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
void Enemy::render(SpriteRenderer* renderer, std::shared_ptr<Texture2D> texture, glm::vec2 gridOffset) {
	if (m_reachedEnd) return; // если враг достиг конца пути, то не рисуем его

    // Задаем физический размер спрайта врага на экране
    glm::vec2 size(40.0f, 40.0f);

    // Вычисляем финальную позицию на экране: 
    // Плавные пиксели врага + общий сдвиг всей сетки в окне (gridOffset) + микро-сдвиг 12 пикселей для центрирования внутри ячейки
    glm::vec2 centeredPos = m_pixelPos + gridOffset + glm::vec2(12.0f, 12.0f);

    // Отправляем команду в SpriteRenderer
    renderer->drawSprite(texture, centeredPos, size, 0.0f, glm::vec3(0.2f, 1.0f, 0.2f));
}