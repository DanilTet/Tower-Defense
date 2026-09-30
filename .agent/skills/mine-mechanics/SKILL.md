---
name: mine-mechanics
description: "Разработка и внедрение шахтных игровых механик Tower Defense (Горловка / ртутные рудники): система статус-эффектов (Slow, DoT), ртутная башня, поршень-толкатель, сплит врагов-капель, рельсы и вагонетки, шурфы-обрывы, ускорение времени x2."
triggers:
  - "сделай ртутную башню"
  - "сделай вагонетку"
  - "сделай поршень"
  - "сделай механику"
  - "шахтные механики"
  - "реализуй статус"
  - "реализуй сплит"
  - "реализуй шурф"
  - "добавь ускорение времени"
---

# Роль и Контекст
Ты — Senior C++ Gameplay & Engine разработчик. Твоя задача — проектировать и безошибочно внедрять игровые механики в проект шахтного Tower Defense (`C:\TD\Tower-Defense`).

Все механики должны гармонично вписываться в существующую модульную архитектуру:
* Сущности: [`Enemy`](file:///C:/TD/Tower-Defense/src/game/entities/Enemy.h), [`Tower`](file:///C:/TD/Tower-Defense/src/game/entities/Tower.h), [`Projectile`](file:///C:/TD/Tower-Defense/src/game/entities/Projectile.h).
* Менеджеры: [`EntityManager`](file:///C:/TD/Tower-Defense/src/game/gameplay/EntityManager.h), [`BuildManager`](file:///C:/TD/Tower-Defense/src/game/gameplay/BuildManager.cpp), [`WaveManager`](file:///C:/TD/Tower-Defense/src/game/core/WaveManager.h).
* Мир и навигация: [`Grid`](file:///C:/TD/Tower-Defense/src/game/world/Grid.h), [`Pathfinder`](file:///C:/TD/Tower-Defense/src/game/world/Pathfinder.h), [`PathService`](file:///C:/TD/Tower-Defense/src/game/world/PathService.h).
* Рендеринг и UI: [`GameplayRenderer`](file:///C:/TD/Tower-Defense/src/renderer/GameplayRenderer.h), [`BuildPanel`](file:///C:/TD/Tower-Defense/src/game/ui/BuildPanel.h).
* Конфиги: [`res/configs/towers.json`](file:///C:/TD/Tower-Defense/res/configs/towers.json), [`res/configs/enemies.json`](file:///C:/TD/Tower-Defense/res/configs/enemies.json).

---

## 1. Механика: Система Статус-Эффектов (Slow & Poison)

### В `Enemy.h` / `Enemy.cpp`:
1. Структура `StatusEffect`:
   ```cpp
   enum class StatusType { Slow, Poison, Stun };
   struct StatusEffect {
       StatusType type;
       float duration;
       float intensity;       // напр. 0.4f = замедление на 40%
       float tickInterval;    // напр. 0.25f сек
       float tickTimer = 0.0f;
       int damagePerTick = 0;
   };
   ```
2. Хранение: `std::vector<StatusEffect> m_statusEffects;`
3. В `Enemy::update(float dt)`:
   * Вычислять результирующий `speedModifier`: брать максимальное замедление среди активных (не складывать линейно во избежание полной остановки).
   * Итоговая скорость кадра: `float effectiveSpeed = m_speed * (1.0f - maxSlow)`.
   * Обрабатывать таймеры тиков яда и наносить урон: `m_health -= damage`.
   * Очищать истекшие эффекты (`duration <= 0.0f`).
4. Визуальный отклик: подкрашивать `m_color` врага в зеленый при яде и в синевато-серый/ртутный при замедлении.

---

## 2. Механика: Ртутная Башня («Никитовка»)

1. Конфиг в `res/configs/towers.json`:
   * Секция `"Mercury"`: умеренный прямой урон, радиус 2.5–3.5, свойства `"slowPercent": 0.4`, `"slowDuration": 2.5`, `"poisonDps": 12`, `"poisonDuration": 3.0`.
2. В `Projectile.cpp`:
   * При попадании в цель (или в радиусе сплеша) вызывать `target->applySlow(...)` и `target->applyPoison(...)`.
3. Добавить кнопку в [`BuildPanel`](file:///C:/TD/Tower-Defense/src/game/ui/BuildPanel.cpp) для покупки и строительства ртутной башни.

---

## 3. Механика: Враг «Ртутная Капля» (Сплит при смерти)

1. Конфиг в `res/configs/enemies.json`:
   * Враги: `"MercurySlime_Big"`, `"MercurySlime_Medium"`, `"MercurySlime_Small"`.
   * Поля в конфиге:
     ```json
     "splitOnDeath": {
       "childType": "MercurySlime_Medium",
       "count": 2,
       "scatterOffset": 12.0
     }
     ```
2. В `EntityManager.cpp`:
   * При наступлении `m_health <= 0`:
   * Проверить `stats.hasSplit()`.
   * Если да, создать `count` новых врагов типа `childType` на позиции `parent->getPixelPos() + offset`.
   * **Критически важно:** передать дочерним врагам копию маршрута родителя `parent->getPath()` и текущий индекс вейпоинта `parent->getCurrentWaypoint()`, чтобы они продолжили путь к базе, а не бежали с начала!

---

## 4. Механика: Пневматический Поршень и Шурфы (Обрывы)

1. **Башня «Поршень» (Piston):**
   * Смотрит строго в одну из 4 сторон света: 270° (Вверх/Север), 0° (Вправо/Восток), 90° (Вниз/Юг), 180° (Влево/Запад).
   * Не вращается за врагами, а контролирует 1 клетку прямо перед бойком: `targetCell = pos + getDirectionOffset(angle)`.
   * При выборе в панели постройки нажимается `R` для поворота голограммы с подсказкой на экране.
   * Наносит минимальный урон (5 / 10 / 15), выполняя исключительно роль контроля толпы (Crowd Control).
   * Выбрасывает боёк (анимация выдвижения) и сообщает врагу резкий импульс `applyKnockback(punchDir, force)`.
2. **Физика отталкивания и поведение врагов (`Enemy`):**
   * При отталкивании обычное движение врага блокируется (`m_isKnockedBack = true`).
   * Если враг отброшен назад по тропе: его прогресс по маршруту (`m_currentWayPoint`) откатывается назад к соответствующей клетке пути.
   * Если враг впечатан в стену или башню: столкновение останавливает импульс, наносит дополнительный урон и накладывает оглушение `m_stunTimer = 0.8s`.
3. **Шурфы (Обрывы) и падение:**
   * Добавить `CellType::Chasm` (провал/шурф).
   * Если центр `m_pixelPos` врага после толчка оказывается на клетке `CellType::Chasm`:
     * Враг переходит в состояние `Falling` (уменьшение масштаба до 0) и погибает без нанесения вреда базе (Instakill).

---

## 5. Механика: Рельсы и Вагонетка (Minecart Hazard)

1. Клетки `CellType::Rail` в `Grid`:
   * Дорожка для вагонетки. Враги могут по ней ходить, башни строить нельзя.
2. Контроллер вагонеток в `GameWorld`:
   * Таймер `m_minecartTimer` (например, 25 секунд).
   * За 3 секунды до отправления: состояние `Warning` (мигает красный семафор, звук сирены/колокола).
   * Фаза `Active`: объект вагонетки перемещается по рельсам от первой до последней клетки с высокой скоростью.
   * Коллизия: проверяет пересечение `CircleCollider` или AABB со всеми врагами в `EntityManager`.
   * При пересечении: `enemy->takeDamage(99999)` + эффект взрыва ртутных/угольных частиц.

---

## 6. Механика: Ускорение Времени (x1 / x2) и QoL

1. В `GameplayState`:
   * Поле `float m_timeScale = 1.0f;`.
   * Переключение по клавише `Space` или цифровым кнопкам:
     ```cpp
     if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
         m_timeScale = (m_timeScale == 1.0f) ? 2.0f : 1.0f;
     }
     ```
   * В вызове `m_world->update(dt * m_timeScale);`.
2. Индикация в UI: иконка или текст `[x1]` / `[x2]` в углу экрана.

---

## Правила Реализации и Проверки
1. **Чистота сборки:** После любых изменений кода запускать проверку компиляции:
   ```powershell
   & "C:\Program Files\Microsoft Visual Studio\18\Enterprise\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64; cmake --build out/build/x64-Debug
   ```
2. **Безопасность памяти:** Все дочерние сущности при сплите спавнить через `std::unique_ptr` в существующий `m_enemies` вектор менеджера.
3. **Сохранение целостности конфигов:** Не ломать существующие ключи в `towers.json` и `enemies.json`, только расширять новыми необязательными полями со значениями по умолчанию.

