#include "PrimitiveGenerator.h"
#include <cmath>
#include <limits>
#include <algorithm>

constexpr float PI = 3.14159265358979323846f;

std::unique_ptr<Model> PrimitiveGenerator::createCube(float size, const glm::vec4& color) {
    auto model = std::make_unique<Model>();
    model->name = "Cubo (" + std::to_string(size).substr(0, 4) + ")";
    model->filePath = "";
    model->primitiveConfig = "cube " + std::to_string(size);
    model->diffuseColor = color;

    Mesh mesh;
    mesh.name = "Cubo";
    mesh.color = color;

    float h = size * 0.5f;

    struct FaceData {
        glm::vec3 p0, p1, p2, p3;
        glm::vec3 normal;
    };

    FaceData faces[6] = {
        // Frente (+Z)
        { {-h, -h,  h}, { h, -h,  h}, { h,  h,  h}, {-h,  h,  h}, { 0.0f,  0.0f,  1.0f} },
        // Atrás (-Z)
        { { h, -h, -h}, {-h, -h, -h}, {-h,  h, -h}, { h,  h, -h}, { 0.0f,  0.0f, -1.0f} },
        // Arriba (+Y)
        { {-h,  h,  h}, { h,  h,  h}, { h,  h, -h}, {-h,  h, -h}, { 0.0f,  1.0f,  0.0f} },
        // Abajo (-Y)
        { {-h, -h, -h}, { h, -h, -h}, { h, -h,  h}, {-h, -h,  h}, { 0.0f, -1.0f,  0.0f} },
        // Derecha (+X)
        { { h, -h,  h}, { h, -h, -h}, { h,  h, -h}, { h,  h,  h}, { 1.0f,  0.0f,  0.0f} },
        // Izquierda (-X)
        { {-h, -h, -h}, {-h, -h,  h}, {-h,  h,  h}, {-h,  h, -h}, {-1.0f,  0.0f,  0.0f} }
    };

    for (int i = 0; i < 6; i++) {
        unsigned int baseIdx = static_cast<unsigned int>(mesh.vertices.size());

        mesh.vertices.push_back({ faces[i].p0, faces[i].normal });
        mesh.vertices.push_back({ faces[i].p1, faces[i].normal });
        mesh.vertices.push_back({ faces[i].p2, faces[i].normal });
        mesh.vertices.push_back({ faces[i].p3, faces[i].normal });

        // Triángulos en orden CCW
        mesh.indices.push_back(baseIdx + 0);
        mesh.indices.push_back(baseIdx + 1);
        mesh.indices.push_back(baseIdx + 2);

        mesh.indices.push_back(baseIdx + 2);
        mesh.indices.push_back(baseIdx + 3);
        mesh.indices.push_back(baseIdx + 0);
    }

    mesh.setup();
    model->minBounds = mesh.minBounds;
    model->maxBounds = mesh.maxBounds;
    model->meshes.push_back(std::move(mesh));

    return model;
}

std::unique_ptr<Model> PrimitiveGenerator::createPyramid(float baseWidth, float height, const glm::vec4& color) {
    auto model = std::make_unique<Model>();
    model->name = "Piramide (W:" + std::to_string(baseWidth).substr(0, 3) + " H:" + std::to_string(height).substr(0, 3) + ")";
    model->filePath = "";
    model->primitiveConfig = "pyramid " + std::to_string(baseWidth) + " " + std::to_string(height);
    model->diffuseColor = color;

    Mesh mesh;
    mesh.name = "Piramide";
    mesh.color = color;

    float w = baseWidth * 0.5f;
    float h = height * 0.5f;

    glm::vec3 apex(0.0f, h, 0.0f);
    glm::vec3 p0(-w, -h, -w);
    glm::vec3 p1( w, -h, -w);
    glm::vec3 p2( w, -h,  w);
    glm::vec3 p3(-w, -h,  w);

    // Base
    glm::vec3 baseNormal(0.0f, -1.0f, 0.0f);
    unsigned int baseIdx = static_cast<unsigned int>(mesh.vertices.size());
    mesh.vertices.push_back({ p0, baseNormal });
    mesh.vertices.push_back({ p1, baseNormal });
    mesh.vertices.push_back({ p2, baseNormal });
    mesh.vertices.push_back({ p3, baseNormal });

    mesh.indices.push_back(baseIdx + 0);
    mesh.indices.push_back(baseIdx + 2);
    mesh.indices.push_back(baseIdx + 1);

    mesh.indices.push_back(baseIdx + 0);
    mesh.indices.push_back(baseIdx + 3);
    mesh.indices.push_back(baseIdx + 2);

    // Caras laterales
    auto addTriFace = [&](const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2) {
        glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
        unsigned int idx = static_cast<unsigned int>(mesh.vertices.size());
        mesh.vertices.push_back({ v0, normal });
        mesh.vertices.push_back({ v1, normal });
        mesh.vertices.push_back({ v2, normal });

        mesh.indices.push_back(idx + 0);
        mesh.indices.push_back(idx + 1);
        mesh.indices.push_back(idx + 2);
    };

    addTriFace(p3, p2, apex);
    addTriFace(p2, p1, apex);
    addTriFace(p1, p0, apex);
    addTriFace(p0, p3, apex);

    mesh.setup();
    model->minBounds = mesh.minBounds;
    model->maxBounds = mesh.maxBounds;
    model->meshes.push_back(std::move(mesh));

    return model;
}

std::unique_ptr<Model> PrimitiveGenerator::createSphere(float radius, int sectors, int stacks, const glm::vec4& color) {
    sectors = std::max(sectors, 6);
    stacks = std::max(stacks, 4);

    auto model = std::make_unique<Model>();
    model->name = "Esfera (R:" + std::to_string(radius).substr(0, 3) + " S:" + std::to_string(sectors) + ")";
    model->filePath = "";
    model->primitiveConfig = "sphere " + std::to_string(radius) + " " + std::to_string(sectors) + " " + std::to_string(stacks);
    model->diffuseColor = color;

    Mesh mesh;
    mesh.name = "Esfera";
    mesh.color = color;

    float lengthInv = 1.0f / radius;

    for (int i = 0; i <= stacks; ++i) {
        float stackAngle = PI / 2.0f - static_cast<float>(i) * (PI / static_cast<float>(stacks));
        float xy = radius * std::cos(stackAngle);
        float y = radius * std::sin(stackAngle);

        for (int j = 0; j <= sectors; ++j) {
            float sectorAngle = static_cast<float>(j) * (2.0f * PI / static_cast<float>(sectors));

            float x = xy * std::cos(sectorAngle);
            float z = xy * std::sin(sectorAngle);

            glm::vec3 pos(x, y, z);
            glm::vec3 norm(x * lengthInv, y * lengthInv, z * lengthInv);

            mesh.vertices.push_back({ pos, norm });
        }
    }

    for (int i = 0; i < stacks; ++i) {
        int k1 = i * (sectors + 1);
        int k2 = k1 + sectors + 1;

        for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
            if (i != 0) {
                mesh.indices.push_back(k1);
                mesh.indices.push_back(k2);
                mesh.indices.push_back(k1 + 1);
            }

            if (i != (stacks - 1)) {
                mesh.indices.push_back(k1 + 1);
                mesh.indices.push_back(k2);
                mesh.indices.push_back(k2 + 1);
            }
        }
    }

    mesh.setup();
    model->minBounds = mesh.minBounds;
    model->maxBounds = mesh.maxBounds;
    model->meshes.push_back(std::move(mesh));

    return model;
}

std::unique_ptr<Model> PrimitiveGenerator::createCylinder(float radius, float height, int segments, const glm::vec4& color) {
    segments = std::max(segments, 6);

    auto model = std::make_unique<Model>();
    model->name = "Cilindro (R:" + std::to_string(radius).substr(0, 3) + " H:" + std::to_string(height).substr(0, 3) + ")";
    model->filePath = "";
    model->primitiveConfig = "cylinder " + std::to_string(radius) + " " + std::to_string(height) + " " + std::to_string(segments);
    model->diffuseColor = color;

    Mesh mesh;
    mesh.name = "Cilindro";
    mesh.color = color;

    float h = height * 0.5f;

    // Pared lateral
    for (int j = 0; j <= segments; ++j) {
        float angle = static_cast<float>(j) * (2.0f * PI / static_cast<float>(segments));
        float x = radius * std::cos(angle);
        float z = radius * std::sin(angle);
        glm::vec3 norm(std::cos(angle), 0.0f, std::sin(angle));

        mesh.vertices.push_back({ {x,  h, z}, norm });
        mesh.vertices.push_back({ {x, -h, z}, norm });
    }

    for (int j = 0; j < segments; ++j) {
        unsigned int top0 = j * 2;
        unsigned int bot0 = top0 + 1;
        unsigned int top1 = (j + 1) * 2;
        unsigned int bot1 = top1 + 1;

        mesh.indices.push_back(top0);
        mesh.indices.push_back(bot0);
        mesh.indices.push_back(top1);

        mesh.indices.push_back(top1);
        mesh.indices.push_back(bot0);
        mesh.indices.push_back(bot1);
    }

    // Tapa superior (+Y)
    unsigned int topCenterIdx = static_cast<unsigned int>(mesh.vertices.size());
    mesh.vertices.push_back({ {0.0f, h, 0.0f}, {0.0f, 1.0f, 0.0f} });

    for (int j = 0; j <= segments; ++j) {
        float angle = static_cast<float>(j) * (2.0f * PI / static_cast<float>(segments));
        float x = radius * std::cos(angle);
        float z = radius * std::sin(angle);
        mesh.vertices.push_back({ {x, h, z}, {0.0f, 1.0f, 0.0f} });
    }

    for (int j = 0; j < segments; ++j) {
        mesh.indices.push_back(topCenterIdx);
        mesh.indices.push_back(topCenterIdx + 1 + j);
        mesh.indices.push_back(topCenterIdx + 2 + j);
    }

    // Tapa inferior (-Y)
    unsigned int botCenterIdx = static_cast<unsigned int>(mesh.vertices.size());
    mesh.vertices.push_back({ {0.0f, -h, 0.0f}, {0.0f, -1.0f, 0.0f} });

    for (int j = 0; j <= segments; ++j) {
        float angle = static_cast<float>(j) * (2.0f * PI / static_cast<float>(segments));
        float x = radius * std::cos(angle);
        float z = radius * std::sin(angle);
        mesh.vertices.push_back({ {x, -h, z}, {0.0f, -1.0f, 0.0f} });
    }

    for (int j = 0; j < segments; ++j) {
        mesh.indices.push_back(botCenterIdx);
        mesh.indices.push_back(botCenterIdx + 2 + j);
        mesh.indices.push_back(botCenterIdx + 1 + j);
    }

    mesh.setup();
    model->minBounds = mesh.minBounds;
    model->maxBounds = mesh.maxBounds;
    model->meshes.push_back(std::move(mesh));

    return model;
}
