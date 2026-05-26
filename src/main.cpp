#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "shaders/ShaderProgram.h"
#include "textures/Texture2D.h"

int windowWidth = 640;
int windowHeight = 480;

float vertices[] = {
    0.5f,    0.5f,    1.0f, 1.0f,
    0.5f,    -0.5f,   1.0f, 0.0f,
    -0.5f,   -0.5f,   0.0f, 0.0f,
    -0.5f,   0.5f,    0.0f, 1.0f
};

unsigned int indices[] = {
	0, 1, 3,
	1, 2, 3
};

void glfwWindowSizeCallback(GLFWwindow* window, int width, int height)
{
	windowWidth = width;
	windowHeight = height;
	glViewport(0, 0, windowWidth, windowHeight);
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

    glClearColor(1, 1, 0, 1);

    ShaderProgram shaderProgram;
    if (!shaderProgram.loadShaders("res/shaders/vertex_shader.vert", "res/shaders/fragment_shader.frag")) {
        std::cerr << "Failed to load/compile shaders!" << std::endl;
        glfwTerminate();
        return -1;
    }

    Texture2D testTexture;
    if (!testTexture.load("res/textures/test_sprite.png")) {
        std::cerr << "Failed to load test texture!" << std::endl;
        glfwTerminate();
        return -1;
    }

    shaderProgram.use();
    int textureLocation = glGetUniformLocation(shaderProgram.getId(), "u_texture");
    glUniform1i(textureLocation, 0);

	unsigned int VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);
        
		shaderProgram.use(); // мои четенькие шейдеры
        testTexture.bind(0);
		glBindVertexArray(VAO);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);

        testTexture.unbind();

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    shaderProgram.deleteProgram();
    glfwTerminate();
    return 0;
}