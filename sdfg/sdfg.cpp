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

// ========== ПЕРЕМЕННЫЕ ДЛЯ ДВИЖЕНИЯ ЧАСТЕЙ СТАНКА ==========
// Индексы мешей (определяются порядком загрузки из OBJ):
// mesh[0] = Cube (Станина - базовый элемент, не двигается)
// mesh[1] = Cube.001 (Первая степень подвижности - вдоль станка)
// mesh[2] = Cube.003 (Вторая степень подвижности - поперек под углом)
// mesh[3] = Cylinder (Третья степень подвижности - вращение барабана)

// Позиции для линейного движения
float spindleAlongPosition = 0.0f;      // Первая степень - вдоль станка (ось X)
float spindleAcrossPosition = 0.0f;     // Вторая степень - поперек станка (ось Z под углом)

// Угол для вращения барабана
float barrelRotation = 0.0f;            // Третья степень - вращение (ось Y)

// Скорости движения
const float moveSpeed = 2.0f;           // Скорость линейного движения
const float rotateSpeed = 90.0f;        // Скорость вращения (градусы в секунду)

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
        shader.setVec3("viewPos", cameraPosition.x, cameraPosition.y, cameraPosition.z);

        // ========== ОТРИСОВКА КАЖДОЙ ЧАСТИ СО СВОЕЙ ТРАНСФОРМАЦИЕЙ ==========
        // Меш 0: Станина (базовый элемент) - не двигается
        if (ourModel.meshes.size() > 0) {
            glm::mat4 baseModel = glm::mat4(1.0f);
            ourModel.meshes[0].DrawWithModel(shader, baseModel);
        }

        // Меш 1: Первая степень подвижности - линейное движение вдоль станка (ось X)
        // Cube.001: начальная позиция X[0.948:1.869], двигаем вдоль X
        // Ограничение: не даем уйти влево за пределы станины (X > -0.5)
        if (ourModel.meshes.size() > 1) {
            glm::mat4 mesh1Model = glm::translate(glm::mat4(1.0f), glm::vec3(spindleAlongPosition, 0.0f, 0.0f));
            ourModel.meshes[1].DrawWithModel(shader, mesh1Model);
        }

        // Меш 2: Вторая степень подвижности - линейное движение поперек под углом (ось Z с наклоном 45°)
        // Cube.003: начальная позиция Y[2.754:3.969] (выше станины), двигаем в плоскости XZ под 45°
        // Движение относительно начальной позиции меша
        if (ourModel.meshes.size() > 2) {
            // Движение поперек станка под углом 45 градусов в плоскости XZ
            float angle = glm::radians(45.0f);
            float acrossX = spindleAcrossPosition * sin(angle);
            float acrossZ = spindleAcrossPosition * cos(angle);
            glm::mat4 mesh2Model = glm::translate(glm::mat4(1.0f), glm::vec3(acrossX, 0.0f, acrossZ));
            ourModel.meshes[2].DrawWithModel(shader, mesh2Model);
        }

        // Меш 3: Третья степень подвижности - вращение инструментального барабана
        // Cylinder: начальная позиция X[1.046:1.632] Y[2.140:2.608] Z[0.979:1.242]
        // Центр барабана примерно: (1.339, 2.374, 1.110)
        // Вращаем ВОКРУГ ЕГО СОБСТВЕННОЙ ОСИ Y (проходящей через его центр)
        if (ourModel.meshes.size() > 3) {
            // 1) Сместить барабан так, чтобы его центр оказался в начале координат
            // 2) Повернуть вокруг оси Y
            // 3) Вернуть обратно
            glm::vec3 barrelCenter(1.339f, 2.374f, 1.110f);
            glm::mat4 mesh3Model = 
                glm::translate(glm::mat4(1.0f), barrelCenter) *
                glm::rotate(glm::mat4(1.0f), glm::radians(barrelRotation), glm::vec3(0.0f, 1.0f, 0.0f)) *
                glm::translate(glm::mat4(1.0f), -barrelCenter);
            ourModel.meshes[3].DrawWithModel(shader, mesh3Model);
        }

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
    
    // === УПРАВЛЕНИЕ КАМЕРОЙ (WASD + стрелки + мышь) ===
    float speed = 2.5f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPosition += speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPosition -= speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPosition -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPosition += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
    
    // Движение камеры стрелками (вперед/назад/влево/вправо относительно мира)
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) cameraPosition.z -= speed;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) cameraPosition.z += speed;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) cameraPosition.x -= speed;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) cameraPosition.x += speed;
    
    // === УПРАВЛЕНИЕ ЧАСТЯМИ СТАНКА (НЕЗАВИСИМОЕ ДВИЖЕНИЕ) ===
    
    // Первая степень подвижности: линейное движение шпиндельной головы вдоль станка (ось X)
    // Cube.001: начальная позиция X[0.948:1.869], Z[-2.829:-0.830]
    // Станина (Cube): X[-1.176:1.867], Z[-2.144:1.937]
    // Движение: Q (влево/назад), E (вправо/вперед) вдоль оси X
    // Ограничение: не даем уйти влево за пределы станины (min X = -0.5) и вправо (max X = 1.0)
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        spindleAlongPosition -= moveSpeed * deltaTime;
        // Ограничение движения чтобы не \"ломать\" станок - не уходим влево за пределы
        if (spindleAlongPosition < -0.5f) spindleAlongPosition = -0.5f;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        spindleAlongPosition += moveSpeed * deltaTime;
        // Ограничение движения чтобы не \"ломать\" станок - не уходим вправо за пределы
        if (spindleAlongPosition > 1.0f) spindleAlongPosition = 1.0f;
    }
    
    // Вторая степень подвижности: линейное движение поперек станка под углом (ось Z под 45°)
    // Cube.003: начальная позиция Y[2.754:3.969] (выше станины), двигаем в плоскости XZ под 45°
    // Движение: R (назад/влево), F (вперед/вправо) под углом 45° в плоскости XZ
    // Ограничение: не даем уйти слишком далеко (-1.5 до 1.5)
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        spindleAcrossPosition -= moveSpeed * deltaTime;
        if (spindleAcrossPosition < -1.5f) spindleAcrossPosition = -1.5f;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        spindleAcrossPosition += moveSpeed * deltaTime;
        if (spindleAcrossPosition > 1.5f) spindleAcrossPosition = 1.5f;
    }
    
    // Третья степень подвижности: вращение инструментального барабана (вокруг собственной оси Y)
    // Cylinder: вращается независимо вокруг своей центральной оси Y
    // Барабан двигается НЕЗАВИСИМО от других частей
    // Клавиши: T (по часовой), G (против часовой)
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
        barrelRotation += rotateSpeed * deltaTime;
        if (barrelRotation >= 360.0f) barrelRotation -= 360.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
        barrelRotation -= rotateSpeed * deltaTime;
        if (barrelRotation <= 0.0f) barrelRotation += 360.0f;
    }
}