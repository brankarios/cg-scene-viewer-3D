#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h"

class BoundingBox {
public:
    BoundingBox();
    ~BoundingBox();

    BoundingBox(const BoundingBox&) = delete;
    BoundingBox& operator=(const BoundingBox&) = delete;

    void init();
    void draw(Shader& shader, const glm::mat4& parentMatrix,
              const glm::vec3& minBounds, const glm::vec3& maxBounds,
              const glm::vec4& color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)) const;

private:
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;
    bool initialized = false;
};
