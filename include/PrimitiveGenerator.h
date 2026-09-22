#pragma once

#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "Model.h"

class PrimitiveGenerator {
public:
    // 1. Cubo parametrizado (tamaño de arista)
    static std::unique_ptr<Model> createCube(float size = 1.0f,
                                             const glm::vec4& color = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));

    // 2. Pirámide parametrizada (ancho de base y altura)
    static std::unique_ptr<Model> createPyramid(float baseWidth = 1.0f, float height = 1.2f,
                                                const glm::vec4& color = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));

    // 3. Esfera UV parametrizada (radio, sectores/meridianos y pilas/anillos)
    static std::unique_ptr<Model> createSphere(float radius = 0.8f, int sectors = 32, int stacks = 16,
                                               const glm::vec4& color = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));

    // 4. Cilindro parametrizado (radio, altura y segmentos radiales) 
    static std::unique_ptr<Model> createCylinder(float radius = 0.6f, float height = 1.2f, int segments = 32,
                                                 const glm::vec4& color = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));
};
