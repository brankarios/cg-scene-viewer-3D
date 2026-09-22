#include "Model.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <limits>
#include <algorithm>

// Carga de .obj con tinyobjloader 

bool Model::loadFromFile(const std::string& path) {
    // Directorio base para buscar archivos de materiales (.mtl / .mlt)
    size_t lastSlash = path.find_last_of("/\\");
    std::string baseDir = (lastSlash != std::string::npos) ? path.substr(0, lastSlash + 1) : "";
    std::string baseName = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
    size_t lastDot = baseName.find_last_of('.');
    std::string nameWithoutExt = (lastDot != std::string::npos) ? baseName.substr(0, lastDot) : baseName;

    tinyobj::ObjReaderConfig config;
    config.triangulate = true; // Triangular quads y poligonos automaticamente
    config.vertex_color = false;
    config.mtl_search_path = baseDir; // Buscar .mtl en la misma carpeta del .obj

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(path, config)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjLoader Error: " << reader.Error() << std::endl;
        }
        return false;
    }

    if (!reader.Warning().empty()) {
        std::cout << "TinyObjLoader Warning: " << reader.Warning() << std::endl;
    }

    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();
    const auto& materials = reader.GetMaterials();

    bool hasNormals = !attrib.normals.empty();

    for (const auto& shape : shapes) {
        Mesh mesh;
        mesh.name = shape.name;

        if (hasNormals) {
            // Con normales
            std::map<std::pair<int,int>, unsigned int> uniqueVertices;

            for (size_t i = 0; i < shape.mesh.indices.size(); i++) {
                const auto& idx = shape.mesh.indices[i];
                auto key = std::make_pair(idx.vertex_index, idx.normal_index);

                if (uniqueVertices.find(key) == uniqueVertices.end()) {
                    Vertex vertex;
                    vertex.position = glm::vec3(
                        attrib.vertices[3 * idx.vertex_index + 0],
                        attrib.vertices[3 * idx.vertex_index + 1],
                        attrib.vertices[3 * idx.vertex_index + 2]
                    );
                    vertex.normal = glm::vec3(
                        attrib.normals[3 * idx.normal_index + 0],
                        attrib.normals[3 * idx.normal_index + 1],
                        attrib.normals[3 * idx.normal_index + 2]
                    );
                    uniqueVertices[key] = static_cast<unsigned int>(mesh.vertices.size());
                    mesh.vertices.push_back(vertex);
                }
                mesh.indices.push_back(uniqueVertices[key]);
            }
        } else {
            // Sin normales: calcular promedio
            std::map<int, unsigned int> uniqueVertices;

            for (size_t i = 0; i < shape.mesh.indices.size(); i++) {
                const auto& idx = shape.mesh.indices[i];

                if (uniqueVertices.find(idx.vertex_index) == uniqueVertices.end()) {
                    Vertex vertex;
                    vertex.position = glm::vec3(
                        attrib.vertices[3 * idx.vertex_index + 0],
                        attrib.vertices[3 * idx.vertex_index + 1],
                        attrib.vertices[3 * idx.vertex_index + 2]
                    );
                    vertex.normal = glm::vec3(0.0f);
                    uniqueVertices[idx.vertex_index] = static_cast<unsigned int>(mesh.vertices.size());
                    mesh.vertices.push_back(vertex);
                }
                mesh.indices.push_back(uniqueVertices[idx.vertex_index]);
            }

            computeNormals(mesh.vertices, mesh.indices);
        }

        // Color difuso y alfa desde .mtl
        if (!shape.mesh.material_ids.empty() && shape.mesh.material_ids[0] >= 0) {
            int matId = shape.mesh.material_ids[0];
            const auto& mat = materials[matId];
            mesh.color = glm::vec4(
                mat.diffuse[0], mat.diffuse[1], mat.diffuse[2],
                mat.dissolve
            );
        }

        meshes.push_back(std::move(mesh));
    }

    // Si no hay shapes definidos pero hay vertices sueltos, tratarlos como una sola malla
    if (shapes.empty() && !attrib.vertices.empty()) {
        std::cerr << "Advertencia: El archivo no tiene shapes definidos." << std::endl;
        return false;
    }

    // Normalizar el modelo (centrar en origen y escalar a [-1, 1])
    normalizeModel();

    // Subir datos a la GPU despues de normalizar
    for (auto& mesh : meshes) {
        mesh.setup();
    }

    // Guardar nombre y ruta
    filePath = path;
    name = baseName;
    primitiveConfig = "";

    // Si tinyobjloader cargó materiales, actualizar diffuseColor
    if (!materials.empty()) {
        diffuseColor = glm::vec4(
            materials[0].diffuse[0],
            materials[0].diffuse[1],
            materials[0].diffuse[2],
            materials[0].dissolve
        );
        std::cout << "Material .mtl detectado automaticamente -> Kd: ("
                  << diffuseColor.r << ", " << diffuseColor.g << ", " << diffuseColor.b << ")" << std::endl;
    } else {
        // Intentar buscar archivo de material con el mismo nombre (.mtl o .mlt)
        std::string autoMtl = baseDir + nameWithoutExt + ".mtl";
        std::string autoMlt = baseDir + nameWithoutExt + ".mlt";
        std::ifstream checkMtl(autoMtl);
        if (checkMtl.is_open()) {
            checkMtl.close();
            loadMaterialFromFile(autoMtl);
        } else {
            std::ifstream checkMlt(autoMlt);
            if (checkMlt.is_open()) {
                checkMlt.close();
                loadMaterialFromFile(autoMlt);
            }
        }
    }

    std::cout << "Modelo cargado: " << name
              << " (" << meshes.size() << " malla(s))" << std::endl;

    return true;
}

// Carga y parseo de archivo .mtl

bool Model::loadMaterialFromFile(const std::string& mtlPath) {
    std::ifstream file(mtlPath);
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo de material: " << mtlPath << std::endl;
        return false;
    }

    glm::vec3 kd(0.8f, 0.8f, 0.8f);
    float alpha = 1.0f;
    bool foundKd = false;

    std::string line;
    while (std::getline(file, line)) {
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos || line[start] == '#') continue;
        std::string trimmed = line.substr(start);

        std::stringstream ss(trimmed);
        std::string token;
        ss >> token;

        // Color difuso (Kd r g b)
        if (token == "Kd" || token == "kd") {
            ss >> kd.r >> kd.g >> kd.b;
            foundKd = true;
        }
        // Disolución / opacidad (d alfa)
        else if (token == "d") {
            ss >> alpha;
        }
        // Transparencia alternativa (Tr alfa -> alfa = 1 - Tr)
        else if (token == "Tr") {
            float tr = 0.0f;
            ss >> tr;
            alpha = 1.0f - tr;
        }
    }

    if (!foundKd) {
        std::cout << "Aviso: No se encontro etiqueta Kd en " << mtlPath << ". Se mantendran los valores actuales." << std::endl;
        return false;
    }

    glm::vec4 newColor(kd.r, kd.g, kd.b, std::clamp(alpha, 0.0f, 1.0f));
    setDiffuseColor(newColor);

    std::cout << "Material aplicado desde " << mtlPath
              << " -> Kd: (" << kd.r << ", " << kd.g << ", " << kd.b << ", alfa: " << alpha << ")" << std::endl;
    return true;
}

void Model::setDiffuseColor(const glm::vec4& color) {
    diffuseColor = color;
    for (auto& mesh : meshes) {
        mesh.color = color;
    }
}

// Calculo de normales por promedio de caras 

void Model::computeNormals(std::vector<Vertex>& vertices,
                           const std::vector<unsigned int>& indices) {
    // Reiniciar normales
    for (auto& v : vertices) {
        v.normal = glm::vec3(0.0f);
    }

    // Para cada triangulo, calcular la normal de la cara y acumularla
    // en cada uno de sus 3 vertices
    for (size_t i = 0; i < indices.size(); i += 3) {
        Vertex& v0 = vertices[indices[i + 0]];
        Vertex& v1 = vertices[indices[i + 1]];
        Vertex& v2 = vertices[indices[i + 2]];

        glm::vec3 edge1 = v1.position - v0.position;
        glm::vec3 edge2 = v2.position - v0.position;
        glm::vec3 faceNormal = glm::cross(edge1, edge2);

        v0.normal += faceNormal;
        v1.normal += faceNormal;
        v2.normal += faceNormal;
    }

    // Normalizar cada normal acumulada
    for (auto& v : vertices) {
        if (glm::length(v.normal) > 0.0001f) {
            v.normal = glm::normalize(v.normal);
        }
    }
}

// Normalizacion del modelo (centrar + escalar) 

void Model::normalizeModel() {
    // Encontrar el bounding box global de todas las mallas
    glm::vec3 minBounds(std::numeric_limits<float>::max());
    glm::vec3 maxBounds(std::numeric_limits<float>::lowest());

    for (const auto& mesh : meshes) {
        for (const auto& vertex : mesh.vertices) {
            minBounds = glm::min(minBounds, vertex.position);
            maxBounds = glm::max(maxBounds, vertex.position);
        }
    }

    // Centro del bounding box
    glm::vec3 center = (minBounds + maxBounds) * 0.5f;

    // Extension maxima para escalar uniformemente
    glm::vec3 size = maxBounds - minBounds;
    float maxExtent = std::max({size.x, size.y, size.z});
    float scaleFactor = (maxExtent > 0.0001f) ? 2.0f / maxExtent : 1.0f;

    // Aplicar centrado y escalado a todos los vertices
    for (auto& mesh : meshes) {
        for (auto& vertex : mesh.vertices) {
            vertex.position = (vertex.position - center) * scaleFactor;
        }
    }

    // Calcular y guardar los minBounds y maxBounds finales normalizados
    this->minBounds = glm::vec3(std::numeric_limits<float>::max());
    this->maxBounds = glm::vec3(std::numeric_limits<float>::lowest());
    for (auto& mesh : meshes) {
        mesh.computeAABB();
        this->minBounds = glm::min(this->minBounds, mesh.minBounds);
        this->maxBounds = glm::max(this->maxBounds, mesh.maxBounds);
    }
}

//  Dibujo 

void Model::draw(Shader& shader) const {
    glm::mat4 modelMatrix = getModelMatrix();

    for (const auto& mesh : meshes) {
        glm::mat4 finalMatrix = modelMatrix * mesh.getLocalModelMatrix();
        shader.setMat4("model", finalMatrix);
        shader.setVec4("objectColor", mesh.color);
        mesh.draw();
    }
}

glm::mat4 Model::getModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1, 0, 0));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0, 1, 0));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0, 0, 1));
    model = glm::scale(model, scale);
    return model;
}
