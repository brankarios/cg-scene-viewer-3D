#include "BoundingBox.h"

BoundingBox::BoundingBox() {
}

BoundingBox::~BoundingBox() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
}

void BoundingBox::init() {
    if (initialized) return;

    // Vértices del cubo unitario de (0,0,0) a (1,1,1)
    float vertices[] = {
        0.0f, 0.0f, 0.0f, // 0
        1.0f, 0.0f, 0.0f, // 1
        1.0f, 1.0f, 0.0f, // 2
        0.0f, 1.0f, 0.0f, // 3
        0.0f, 0.0f, 1.0f, // 4
        1.0f, 0.0f, 1.0f, // 5
        1.0f, 1.0f, 1.0f, // 6
        0.0f, 1.0f, 1.0f  // 7
    };

    // 12 aristas (24 índices de líneas)
    unsigned int indices[] = {
        0, 1,  1, 2,  2, 3,  3, 0, // Base inferior (z = 0)
        4, 5,  5, 6,  6, 7,  7, 4, // Base superior (z = 1)
        0, 4,  1, 5,  2, 6,  3, 7  // Conexiones verticales
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Atributo 0: Posición vec3
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindVertexArray(0);
    initialized = true;
}

void BoundingBox::draw(Shader& shader, const glm::mat4& parentMatrix,
                      const glm::vec3& minBounds, const glm::vec3& maxBounds,
                      const glm::vec4& color) const {
    if (!initialized || !VAO) return;

    glm::vec3 size = maxBounds - minBounds;
    // Si el tamaño es cero o inválido, no dibujar
    if (size.x <= 0.0f && size.y <= 0.0f && size.z <= 0.0f) return;

    glm::mat4 boxTransform = glm::translate(glm::mat4(1.0f), minBounds);
    boxTransform = glm::scale(boxTransform, size);

    shader.setMat4("model", parentMatrix * boxTransform);
    shader.setVec4("flatColor", color);

    glBindVertexArray(VAO);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
