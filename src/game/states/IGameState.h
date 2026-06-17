#pragma once

struct GLFWwindow;

class IGameState {
public:
    virtual ~IGameState() = default;

    // инициализация
    virtual void init() = 0;

    // типа деструктор
    virtual void cleanup() = 0;

    //обрабатывает ввод, логику и отрисовку
    virtual void processInput(GLFWwindow* window, float dt) = 0;
    virtual void update(float dt) = 0;
    virtual void render() = 0;
    virtual void resize(int width, int height) = 0;
};