#include "Mesh.h"
#include <glm/gtc/matrix_transform.hpp>
#include <utility>
#include <limits>
#include <algorithm>

Mesh::~Mesh() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
}

Mesh::Mesh(Mesh&& other) noexcept
    : vertices(std::move(other.vertices))
    , indices(std::move(other.indices))
    , color(other.color)
    , name(std::move(other.name))
    , minBounds(other.minBounds)
    , maxBounds(other.maxBounds)
    , position(other.position)
    , rotation(other.rotation)
    , scale(other.scale)
    , VAO(other.VAO), VBO(other.VBO), EBO(other.EBO)
{
    other.VAO = other.VBO = other.EBO = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        // Liberar recursos propios
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
        if (EBO) glDeleteBuffers(1, &EBO);

        vertices = std::move(other.vertices);
        indices = std::move(other.indices);
        color = other.color;
        name = std::move(other.name);
        minBounds = other.minBounds;
        maxBounds = other.maxBounds;
        position = other.position;
        rotation = other.rotation;
        scale = other.scale;
        VAO = other.VAO; VBO = other.VBO; EBO = other.EBO;
        other.VAO = other.VBO = other.EBO = 0;
    }
    return *this;
}

glm::mat4 Mesh::getLocalModelMatrix() const {
    glm::vec3 center = (minBounds + maxBounds) * 0.5f;
    glm::mat4 model = glm::mat4(1.0f);
    // Trasladar al centro de la submalla para rotar y escalar respecto a su propio eje
    model = glm::translate(model, position + center);
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, scale);
    model = glm::translate(model, -center);
    return model;
}

void Mesh::computeAABB() {
    if (vertices.empty()) {
        minBounds = maxBounds = glm::vec3(0.0f);
        return;
    }
    minBounds = glm::vec3(std::numeric_limits<float>::max());
    maxBounds = glm::vec3(std::numeric_limits<float>::lowest());
    for (const auto& v : vertices) {
        minBounds = glm::min(minBounds, v.position);
        maxBounds = glm::max(maxBounds, v.position);
    }
}

// Setup de buffers OpenGL 

void Mesh::setup() {
    computeAABB();

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // Subir vertices
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(Vertex),
                 vertices.data(),
                 GL_STATIC_DRAW);

    // Subir indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(unsigned int),
                 indices.data(),
                 GL_STATIC_DRAW);

    // layout (location = 0) in vec3 aPos
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex),
                          (void*)offsetof(Vertex, position));

    // layout (location = 1) in vec3 aNormal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex),
                          (void*)offsetof(Vertex, normal));

    glBindVertexArray(0);
}

// Draw 

void Mesh::draw() const {
    if (!VAO) return;
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES,
                   static_cast<GLsizei>(indices.size()),
                   GL_UNSIGNED_INT,
                   0);
    glBindVertexArray(0);
}
