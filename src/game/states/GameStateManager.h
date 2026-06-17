#pragma once
#include <memory>
#include <vector>
#include "IGameState.h"

class GameStateManager {
private:
    // типа стек состояний самый верхнее состояние оно активное
    std::vector<std::unique_ptr<IGameState>> m_states;

    // флаги для безопасного переключения в конце кадра
    std::unique_ptr<IGameState> m_nextState;
    bool m_clearAllAndSet = false;
    bool m_popRequested = false;

public:
    GameStateManager() = default;
    ~GameStateManager();

    // полная смена состояния
    void setState(std::unique_ptr<IGameState> newState);

    // добавить состояние поверх текущего
    void pushState(std::unique_ptr<IGameState> newState);

    // удалить верхнее состояние
    void popState();

    // системные методы
    void processInput(GLFWwindow* window, float dt);
    void update(float dt);
    void render();
    void resize(int width, int height);
    // применение отложенных изменений стейтов
    void applyPendingChanges();

    bool isEmpty() const { return m_states.empty(); }
};