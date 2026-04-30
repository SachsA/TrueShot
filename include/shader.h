// shader.h

#pragma once

#include <glm/glm.hpp>
#include <string>

// GLSL shader program loader. Owns one OpenGL program object.
class Shader {
public:
    unsigned int ID = 0;

    Shader(const char* vertexPath, const char* fragmentPath);
    ~Shader();

    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;

    void use();
    void setMat4 (const std::string& name, const glm::mat4& mat)   const;
    void setVec3 (const std::string& name, const glm::vec3& value) const;
    void setFloat(const std::string& name, float value)            const;
    void setInt  (const std::string& name, int value)              const;

private:
    void checkCompileErrors(unsigned int shader, const std::string& type);
};
