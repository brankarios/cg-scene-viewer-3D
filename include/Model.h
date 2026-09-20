#pragma once

#include "Mesh.h"
#include "Shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>

class Model {
public:
    std::vector<Mesh> meshes;
    std::string name;
    std::string filePath;

    // Transformacion
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f); // Angulos de Euler en grados
    glm::vec3 scale    = glm::vec3(1.0f);

    Model() = default;

    // Cargar modelo desde archivo .obj
    bool loadFromFile(const std::string& path);

    // Dibujar todas las mallas con el shader dado
    void draw(Shader& shader) const;

    // Obtener la matriz de modelo (T * R * S)
    glm::mat4 getModelMatrix() const;

private:
    // Centrar y escalar el modelo para que quepa en [-1, 1]
    void normalizeModel();

    // Calcular normales por promedio de las normales de cada cara
    void computeNormals(std::vector<Vertex>& vertices,
                        const std::vector<unsigned int>& indices);
};
