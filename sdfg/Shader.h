#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <GL/glew.h>

class Shader {
public:
    GLuint ID;

    Shader(const GLchar* vertexPath, const GLchar* fragmentPath);

    void Use();

    // Uniform функции без GLM
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;

    // Векторы через отдельные компоненты
    void setVec2(const std::string& name, float x, float y) const;
    void setVec3(const std::string& name, float x, float y, float z) const;
    void setVec4(const std::string& name, float x, float y, float z, float w) const;

    // Векторы через массивы
    void setVec2v(const std::string& name, const float* value) const;
    void setVec3v(const std::string& name, const float* value) const;
    void setVec4v(const std::string& name, const float* value) const;

    // Матрица 4x4 через массив 16 float
    void setMat4(const std::string& name, const float* value) const;

private:
    void checkCompileErrors(GLuint shader, std::string type);
};

#endif