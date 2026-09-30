#include "Enemy.h"
#include "../renderer/SpriteRenderer.h"
#include "../textures/Texture2D.h"
#include "world/Grid.h"
#include <iostream>
#include "core/ConfigManager.h"
#include "world/Pathfinder.h"
#include "../../resources/ResourceManager.h"
#include "../ui/HealthBarRenderer.h"

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

    // --- ОБРАБОТКА СТАТУС-ЭФФЕКТОВ (Замедление и Яд) ---
    float maxSlow = 0.0f;
    for (auto& effect : m_statusEffects) {
        effect.duration -= dt;
        if (effect.type == StatusType::Slow) {
            if (effect.intensity > maxSlow) {
                maxSlow = effect.intensity;
            }
        }
        else if (effect.type == StatusType::Poison) {
            effect.tickTimer += dt;
            if (effect.tickTimer >= effect.tickInterval) {
                effect.tickTimer -= effect.tickInterval;
                m_health -= effect.damagePerTick;
                if (m_health <= 0) {
                    m_health = 0;
                    return; // Враг погиб от яда, движение прекращается
                }
            }
        }
    }

    // Удаляем истекшие эффекты
    m_statusEffects.erase(
        std::remove_if(m_statusEffects.begin(), m_statusEffects.end(),
            [](const StatusEffect& e) { return e.duration <= 0.0f; }),
        m_statusEffects.end()
    );

    // Кап замедления (макс 80%, чтобы мобы не застревали намертво)
    if (maxSlow > 0.8f) maxSlow = 0.8f;
    m_speedModifier = 1.0f - maxSlow;

    // Таймер оглушения (при ударе о препятствие)
    if (m_stunTimer > 0.0f) {
        m_stunTimer -= dt;
        if (m_stunTimer < 0.0f) m_stunTimer = 0.0f;
        // Во время оглушения враг стоит на месте
        return;
    }

    // Обработка контролируемого отталкивания ровно на 1 клетку (поршень)
    if (m_isKnockedBack) {
        m_knockbackTimer += dt;
        float t = m_knockbackTimer / m_knockbackDuration;
        if (t >= 1.0f) {
            m_pixelPos = m_knockbackTargetPos;
            m_isKnockedBack = false;

            // Перенастраиваем маршрут, чтобы враг дальше шёл от новой клетки
            applyPostKnockbackPath(m_knockbackDestCell, m_knockbackFromCell, grid);
        }
        else {
            // Эффект удара (ease-out quad: сначала резкий импульс, затем плавное торможение)
            float easeOut = 1.0f - (1.0f - t) * (1.0f - t);
            m_pixelPos = glm::mix(m_knockbackStartPos, m_knockbackTargetPos, easeOut);
        }

        // Во время отталкивания стандартное движение по маршруту заблокировано
        return;
    }

    // Проверка безопасности: если целевая клетка заблокирована башней, останавливаемся
    if (m_currentWayPoint < m_path.size()) {
        glm::ivec2 checkCell = m_path[m_currentWayPoint];
        if (!Pathfinder::isCellWalkable(grid, checkCell.x, checkCell.y)) {
            m_path.clear();
            m_currentWayPoint = 0;
            return;
        }
    }

    m_animator.update(dt); // прокрутка анимации

    // Определяем контрольную точку маршрута к которой идем
    glm::ivec2 targetCell = m_path[m_currentWayPoint];

	// переводим индексы контрольной точки в пиксели экрана
    glm::vec2 targetPixelPos = grid.gridToPixel(targetCell.x, targetCell.y);

	// Считаем вектор от текущей позиции врага до контрольной точки
    glm::vec2 toTarget = targetPixelPos - m_pixelPos;

    if (glm::length(toTarget) > 0.001f) {
        m_angle = glm::degrees(atan2(toTarget.y, toTarget.x));
    }

	// Считаем расстояние до контрольной точки
    float distance = glm::length(toTarget);
    
    // перевод скорости в пиксели в секунду с учетом замедления
    float currentPixelSpeed = m_speed * m_speedModifier * grid.getCellSize();

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

    glm::vec3 renderColor = m_color;
    if (m_stunTimer > 0.0f) {
        renderColor = glm::vec3(1.0f, 0.85f, 0.25f); // Золотисто-желтый оттенок оглушения
    }
    else if (isPoisoned() && isSlowed()) {
        renderColor = glm::vec3(0.3f, 0.95f, 0.75f); // Ядовито-ртутный бирюзовый оттенок
    }
    else if (isPoisoned()) {
        renderColor = glm::vec3(0.35f, 1.0f, 0.35f); // Токсично-зеленый оттенок
    }
    else if (isSlowed()) {
        renderColor = glm::vec3(0.45f, 0.75f, 1.0f); // Замедленно-синий оттенок
    }

    renderer->drawSprite(enemyTexPtr, centeredPos, size, m_angle, renderColor, currentFrameUV);

    // --- ОТРИСОВКА ПОЛОСКИ ЗДОРОВЬЯ (HEALTH BAR) ЧЕРЕЗ HealthBarRenderer ---
    HealthBarRenderer::draw(renderer, texture, centeredPos, enemySizeDim, m_health, stats.Maxhealth);

    // --- ОТРИСОВКА ХИТБОКСА (ДЕБАГ) ---

    CircleCollider collider = getCollider(grid);

    glm::vec2 hitboxSize(collider.radius * 2.0f, collider.radius * 2.0f);
    glm::vec2 hitboxPos = collider.center - glm::vec2(collider.radius, collider.radius);

    // ИСПРАВЛЕНИЕ 2: drawSprite с маленькой буквы и передаем radiusTex без звездочки (это shared_ptr)
    renderer->drawSprite(radiusTex, hitboxPos, hitboxSize, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
}

void Enemy::recalculatePosition(const Grid& oldGrid, const Grid& newGrid) {
    if (m_isKnockedBack) {
        m_knockbackStartPos = newGrid.gridToPixel(m_knockbackFromCell.x, m_knockbackFromCell.y);
        m_knockbackTargetPos = newGrid.gridToPixel(m_knockbackDestCell.x, m_knockbackDestCell.y);
    }

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
bool Enemy::isPathIntersecting(glm::ivec2 cell) const {
    if (m_path.empty()) return false;
    for (size_t i = m_currentWayPoint; i < m_path.size(); ++i) {
        if (m_path[i] == cell) {
            return true;
        }
        // Проверяем диагональный срез угла между точками i и i+1
        if (i + 1 < m_path.size()) {
            int dx = m_path[i + 1].x - m_path[i].x;
            int dy = m_path[i + 1].y - m_path[i].y;
            if (std::abs(dx) == 1 && std::abs(dy) == 1) {
                if ((cell.x == m_path[i].x + dx && cell.y == m_path[i].y) ||
                    (cell.x == m_path[i].x && cell.y == m_path[i].y + dy)) {
                    return true;
                }
            }
        }
    }
    return false;
}

// функция для нахождения нового пути
void Enemy::recalculatePath(Pathfinder* pathfinder, const Grid& grid, const std::vector<BaseData>& bases) {
    if (!pathfinder || bases.empty()) return;

    glm::ivec2 currentGridPos = grid.pixelToGrid(m_pixelPos);
    std::vector<glm::ivec2> bestPath;
    int dummyCost = 0;

    std::vector<glm::ivec2> targetPositions;
    if (m_targetBaseIndex >= 0) {
        for (const auto& b : bases) {
            if (b.id == m_targetBaseIndex) {
                targetPositions.push_back(glm::ivec2(b.x, b.y));
            }
        }
    }
    // Fallback: если targetBaseIndex == -1 или базы с таким id нет на карте — идем к ближайшей среди ВСЕХ баз
    if (targetPositions.empty()) {
        for (const auto& b : bases) {
            targetPositions.push_back(glm::ivec2(b.x, b.y));
        }
    }

    bestPath = pathfinder->findPathToAny(grid, currentGridPos, targetPositions, dummyCost);

    if (!bestPath.empty()) {
        m_path = bestPath;
        m_currentWayPoint = 0;
    } else {
        // Тупик: пути нет. Очищаем маршрут, чтобы моб не шел сквозь башни!
        m_path.clear();
        m_currentWayPoint = 0;
    }
}

void Enemy::applySlow(float duration, float slowPercent) {
    for (auto& effect : m_statusEffects) {
        if (effect.type == StatusType::Slow) {
            if (slowPercent >= effect.intensity) {
                effect.intensity = slowPercent;
                effect.duration = std::max(effect.duration, duration);
            }
            return;
        }
    }
    m_statusEffects.push_back({ StatusType::Slow, duration, slowPercent, 0.0f, 0.0f, 0 });
}

void Enemy::applyPoison(float duration, float tickInterval, int damagePerTick) {
    for (auto& effect : m_statusEffects) {
        if (effect.type == StatusType::Poison) {
            effect.duration = std::max(effect.duration, duration);
            if (damagePerTick > effect.damagePerTick) {
                effect.damagePerTick = damagePerTick;
                effect.tickInterval = tickInterval;
            }
            return;
        }
    }
    m_statusEffects.push_back({ StatusType::Poison, duration, 0.0f, tickInterval, 0.0f, damagePerTick });
}

bool Enemy::isSlowed() const {
    for (const auto& e : m_statusEffects) {
        if (e.type == StatusType::Slow && e.duration > 0.0f) return true;
    }
    return false;
}

bool Enemy::isPoisoned() const {
    for (const auto& e : m_statusEffects) {
        if (e.type == StatusType::Poison && e.duration > 0.0f) return true;
    }
    return false;
}

void Enemy::pushOneCell(glm::ivec2 fromCell, glm::ivec2 punchDir, const Grid& grid) {
    if (m_reachedEnd) return;

    glm::ivec2 destCell = fromCell + punchDir;

    // Проверяем, свободна ли клетка назначения (не стена, не башня, не декорация и не за краем карты)
    bool canPush = Pathfinder::isCellWalkable(grid, destCell.x, destCell.y);

    if (!canPush) {
        // Удар о препятствие: враг остаётся на месте, оглушается на 0.8с, 0 урона!
        m_stunTimer = 0.8f;
        m_isKnockedBack = false;
        return;
    }

    // Запускаем смещение ровно на 1 клетку
    m_isKnockedBack = true;
    m_knockbackStartPos = m_pixelPos;
    m_knockbackTargetPos = grid.gridToPixel(destCell.x, destCell.y);
    m_knockbackDuration = 0.20f;
    m_knockbackTimer = 0.0f;
    m_knockbackDestCell = destCell;
    m_knockbackFromCell = fromCell;
}

void Enemy::applyPostKnockbackPath(glm::ivec2 destCell, glm::ivec2 fromCell, const Grid& grid) {
    if (m_path.empty()) return;

    // 1. Проверяем, есть ли destCell в существующем маршруте m_path
    int destIndex = -1;
    for (int i = 0; i < static_cast<int>(m_path.size()); ++i) {
        if (m_path[i] == destCell) {
            destIndex = i;
            break;
        }
    }

    if (destIndex != -1) {
        // Клетка назначения уже есть на маршруте (например, враг отброшен назад по тропе).
        // Следующей целью становится следующая контрольная точка
        if (destIndex + 1 < static_cast<int>(m_path.size())) {
            m_currentWayPoint = destIndex + 1;
        }
        else {
            m_currentWayPoint = destIndex;
        }
    }
    else {
        // Клетка была выбита вбок от тропы.
        // Ищем индекс fromCell в маршруте
        int fromIndex = -1;
        for (int i = 0; i < static_cast<int>(m_path.size()); ++i) {
            if (m_path[i] == fromCell) {
                fromIndex = i;
                break;
            }
        }

        std::vector<glm::ivec2> newPath;
        newPath.push_back(destCell); // Текущее положение врага после толчка
        if (fromIndex != -1) {
            for (size_t i = fromIndex; i < m_path.size(); ++i) {
                newPath.push_back(m_path[i]);
            }
        }
        else {
            // Если fromCell не найден, ищем ближайшую точку маршрута
            float bestDistSq = 1e9f;
            size_t bestIdx = 0;
            for (size_t i = 0; i < m_path.size(); ++i) {
                glm::vec2 diff = glm::vec2(destCell - m_path[i]);
                float dSq = glm::dot(diff, diff);
                if (dSq < bestDistSq) {
                    bestDistSq = dSq;
                    bestIdx = i;
                }
            }
            for (size_t i = bestIdx; i < m_path.size(); ++i) {
                newPath.push_back(m_path[i]);
            }
        }

        m_path = newPath;
        m_currentWayPoint = 1; // Идем от destCell к следующей точке
    }

    // Пересчитываем пройденную дистанцию для корректного таргетинга башен
    m_distanceTraveled = 0.0f;
    for (size_t i = 1; i <= m_currentWayPoint && i < m_path.size(); ++i) {
        m_distanceTraveled += glm::distance(
            grid.gridToPixel(m_path[i].x, m_path[i].y),
            grid.gridToPixel(m_path[i - 1].x, m_path[i - 1].y)
        );
    }
}

void Enemy::applyKnockback(glm::vec2 direction, float force) {
    // Сохранено для обратной совместимости
}