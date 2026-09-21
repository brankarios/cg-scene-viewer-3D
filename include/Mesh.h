#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    glm::vec4 color = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f); // Kd (RGB) + alfa
    std::string name;

    // AABB (Bounding Box local de la submalla)
    glm::vec3 minBounds = glm::vec3(0.0f);
    glm::vec3 maxBounds = glm::vec3(0.0f);
    void computeAABB();

    // Transformaciones locales de la submalla (permite mover submallas independientemente)
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f); // Grados
    glm::vec3 scale = glm::vec3(1.0f);
    glm::mat4 getLocalModelMatrix() const;

    Mesh() = default;
    ~Mesh();

    // No copiar, si mover
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    // Crear VAO/VBO/EBO y subir datos a la GPU
    void setup();

    // Dibujar la malla
    void draw() const;

private:
    unsigned int VAO = 0, VBO = 0, EBO = 0;
};
