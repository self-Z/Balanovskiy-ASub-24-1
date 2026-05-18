#define GLEW_DLL
#define GLFW_DLL
#include <iostream>
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "Shader.h"
#include "Model.h"

// ---------- Камера ----------
glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Ввод с мыши
float lastX = 1024.0f / 2.0f, lastY = 768.0f / 2.0f;
bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;
float fov = 45.0f;

// Прототипы callback-ов
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

int main() {
    // GLFW init
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1024, 768, "ASSIMP Model", NULL, NULL);
    if (!window) { glfwTerminate(); return -2; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW init
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return -3;

    // Включить тест глубины
    glEnable(GL_DEPTH_TEST);

    // Загрузить шейдеры
    Shader shader("vertex.glsl", "fragment.glsl");

    // ========== НАСТРОЙКА МАТЕРИАЛА И СВЕТА ==========
    shader.Use();
    
    shader.setVec3("material.ambient", 0.15f, 0.15f, 0.20f);
    shader.setVec3("material.diffuse", 0.75f, 0.75f, 0.85f);
    shader.setVec3("material.specular", 1.00f, 1.00f, 1.00f);
    shader.setFloat("material.shininess", 128.0f);
    // Источник света (белый, сверху справа)
    shader.setVec3("light.position", 3.0f, 5.0f, 4.0f);
    shader.setVec3("light.ambient", 0.25f, 0.25f, 0.25f); 
    shader.setVec3("light.diffuse", 0.9f, 0.9f, 0.9f);
    shader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);
    // ========================================================

    Model ourModel("model.obj");
    std::cout << "Loaded meshes: " << ourModel.meshes.size() << std::endl;
    if (ourModel.meshes.empty()) {
        std::cerr << "ERROR: Model not loaded!" << std::endl;
    }
    else {
        std::cout << "Model OK, meshes: " << ourModel.meshes.size() << std::endl;
    }

    // Главный цикл
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        processInput(window);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Матрицы
        glm::mat4 projection = glm::perspective(glm::radians(fov), 1024.0f / 768.0f, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(cameraPosition, cameraPosition + cameraFront, cameraUp);
        glm::mat4 model = glm::mat4(1.0f);

        shader.Use();
        shader.setMat4("projection", glm::value_ptr(projection));
        shader.setMat4("view", glm::value_ptr(view));
        shader.setMat4("model", glm::value_ptr(model));

        // Передаём позицию камеры для зеркальных бликов
        shader.setVec3("viewPos", cameraPosition.x, cameraPosition.y, cameraPosition.z);

        // Отрисовка модели
        ourModel.Draw(shader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

// ---------- Реализация callback'ов ----------
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos; lastY = ypos; firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos; lastY = ypos;
    const float sensitivity = 0.1f;
    xoffset *= sensitivity; yoffset *= sensitivity;
    yaw += xoffset; pitch += yoffset;
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    fov -= (float)yoffset;
    if (fov < 1.0f) fov = 1.0f;
    if (fov > 45.0f) fov = 45.0f;
}
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    float speed = 2.5f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPosition += speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPosition -= speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPosition -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPosition += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
}