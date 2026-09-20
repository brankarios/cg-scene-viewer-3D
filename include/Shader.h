#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

class Shader {
public:
    unsigned int ID = 0;

    Shader() = default;
    ~Shader();

    // No copiar, si mover
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    // Cargar y compilar shaders desde archivos
    bool load(const std::string& vertexPath, const std::string& fragmentPath);
    void use() const;

    // Setters de uniforms
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;
    void setMat4(const std::string& name, const glm::mat4& value) const;

private:
    std::string readFile(const std::string& path);
    unsigned int compileShader(const std::string& source, GLenum type);
};
