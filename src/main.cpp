#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include "resources/ResourceManager.h"
#include "game/core/Game.h"
#include "audio/AudioManager.h"

int windowWidth = 640; // начальная ширина окна
int windowHeight = 480; // начальная высота окна

std::unique_ptr<Game> TowerDefenseGame; //указатель на игру, который будет использоваться в коллбеках и главном цикле


/*
Эту функцию вызавает сама система через GLFW, когда изменяется размер окна
Обновляет глобальные переменные размера окна (windowWidth, windowHeight)
И изменяет область рендеринга OpenGL, чтобы она соответствовала новому размеру окна
*/
void glfwWindowSizeCallback(GLFWwindow* window, int width, int height)
{
    // обновляем глобальные переменные размера окна
	windowWidth = width; 
	windowHeight = height;

	// меняем обралсь рисования OpenGL
	glViewport(0, 0, windowWidth, windowHeight);

    // если игра создана
	if (TowerDefenseGame) {
		// сообщаем игре о новом размере окна
        // чтобы она пересчитала матрицу проекции
        TowerDefenseGame->resize(width, height);
	}
}


// Эту функцию вызывет сама система через GLFW, когда пользователь нажимает клавишу
void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	// Если пользователь нажал ESC, то мы говорим GLFW, что окно должно закрыться
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE); // выход из главного цикла и закрытие окна
}


int main(void)
{
	// Запускаем GLFW и настраиваем контекст OpenGL
    if (!glfwInit()) {
		std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
	// Просим GLFW создать контекст OpenGL 4.6 Core Profile
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    /* Создаем окно програмы с названием которое можно задать */
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "IASA tower defence", nullptr, nullptr);

	// Если окно не удалось создать, то выводим ошибку и завершаем работу
    if (!window)
    {
		std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Привязываем функции колбеки до созданового окна
    glfwSetWindowSizeCallback(window, glfwWindowSizeCallback);
	glfwSetKeyCallback(window, glfwKeyCallback);

    // Делаем контекст этого окна главным для текущего потока процессора
    // все команды OpenGL будут рисовать в єтом окне
    glfwMakeContextCurrent(window);

    // Библиотека GLAD ищет на ПК драйвер видеокарты и подвязует к OpenGL чипі GPU
    if (!gladLoadGL()) {
		std::cout << "Failed to initialize OpenGL context" << std::endl;
		return -1;
    }

	std::cout << "Rendderer: " << glGetString(GL_RENDERER) << std::endl;
	std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
	std::cout << "OpenGL " << GLVersion.major << "." << GLVersion.minor << std::endl;

    glClearColor(0.15f, 0.15f, 0.15f, 1.0f); // Задаем цвет фона
    glEnable(GL_BLEND); // Включение прозрачности
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Включение прозрачности

    // инициализация аудио
    AudioManager::init();

    //создание и инициализация игры
    TowerDefenseGame = std::make_unique<Game>(windowWidth, windowHeight); // Выделяем память под игру
	TowerDefenseGame->init(); // включаем инициализацию где подгружается текстуры, шейдеры и т.д.

    // делаем переменные которые будем использовать для управления временем
	double lastFrame = glfwGetTime(); // время начала предыдущего кадра
	float deltaTime = 0.0f; // время, которое потребовалось копьютеру чтобы просчитать и вывести на екран предыдущий кадр

	// Главный цикл игры, который будет работать пока окно не закроется
    while (!glfwWindowShouldClose(window))
    {
        // Рассчитываем дельту времени
        double currentFrame = glfwGetTime();
        deltaTime = static_cast<float>(currentFrame - lastFrame);
        lastFrame = currentFrame;

		// Если дельта будет большая то ее ограничиваем, чтобы не было резких скачков в игре при зависании или отладке
        if (deltaTime > 0.1f) {
            deltaTime = 0.1f;
        }

		glfwPollEvents(); // Спрашивает систему были ли клики мыши, нажатия клавиш и т.д. и вызывает соответствующие функции колбеки

		TowerDefenseGame->processInput(window, deltaTime); // обработка клики мыши, нажатия клавиш
		TowerDefenseGame->update(deltaTime); // обновление логики игры, перемещение врагов, проверка коллизий и т.д.
        
		glClear(GL_COLOR_BUFFER_BIT); // очистка буфера цвета, чтобы нарисовать новый кадр
		TowerDefenseGame->render(); // отрисовка всех объектов игры на экран
		glfwSwapBuffers(window); // меняем передний и задний буфер, чтобы показать новый кадр на экране

        
    }

    AudioManager::cleanup(); // очистка звуков
	TowerDefenseGame.reset(); // удаляем игру и освобождаем память
	ResourceManager::clear(); // очищаем все ресурсы, которые были загружены через ResourceManager
	glfwTerminate(); // завершаем работу с GLFW и закрываем окно
    return 0;
}