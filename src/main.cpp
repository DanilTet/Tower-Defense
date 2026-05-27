#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include "resources/ResourceManager.h"
#include "game/Game.h"

int windowWidth = 640;
int windowHeight = 480;

std::unique_ptr<Game> TowerDefenseGame;

void glfwWindowSizeCallback(GLFWwindow* window, int width, int height)
{
	windowWidth = width;
	windowHeight = height;
	glViewport(0, 0, windowWidth, windowHeight);

	if (TowerDefenseGame) {
        TowerDefenseGame->resize(width, height);
	}
}

void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}


int main(void)
{
    /* Initialize the library */
    if (!glfwInit()) {
		std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    /* Create a windowed mode window and its OpenGL context */
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "IASA tower defence", nullptr, nullptr);
    if (!window)
    {
		std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwSetWindowSizeCallback(window, glfwWindowSizeCallback);
	glfwSetKeyCallback(window, glfwKeyCallback);

    /* Make the window's context current */
    glfwMakeContextCurrent(window);


    if (!gladLoadGL()) {
		std::cout << "Failed to initialize OpenGL context" << std::endl;
		return -1;
    }

	std::cout << "Rendderer: " << glGetString(GL_RENDERER) << std::endl;
	std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
	std::cout << "OpenGL " << GLVersion.major << "." << GLVersion.minor << std::endl;

    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//создание и инициализация игры
    TowerDefenseGame = std::make_unique<Game>(windowWidth, windowHeight);
    TowerDefenseGame->init();

    // делаем переменные которые будем использовать для управления временем
	double lastFrame = glfwGetTime(); // время начала предыдущего кадра
    float deltaTime = 0.0f; // время между кадрами

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        // Считаем время вот эту дельту
        double currentFrame = glfwGetTime();
        deltaTime = static_cast<float>(currentFrame - lastFrame);
        lastFrame = currentFrame;

        if (deltaTime > 0.1f) {
            deltaTime = 0.1f;
        }

        /* Poll for and process events */
        glfwPollEvents();

        TowerDefenseGame->processInput(window, deltaTime);

        TowerDefenseGame->update(deltaTime);
        
        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);
        TowerDefenseGame->render();

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        
    }

    TowerDefenseGame.reset();
    ResourceManager::clear();

    ResourceManager::clear();
    glfwTerminate();
    return 0;
}