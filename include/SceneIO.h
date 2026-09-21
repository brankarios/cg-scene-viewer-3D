#pragma once

#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "Camera.h"
#include "Model.h"

struct RenderSettingsData {
    bool depthTest = true;
    bool cullFace = false;
    bool wireframe = false;
    bool showVertices = false;
    float vertexSize = 5.0f;
    bool showNormals = false;
    float normalLength = 0.06f;
    glm::vec4 normalColor = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);
    bool showBoundingBox = false;
};

struct EnvironmentData {
    glm::vec3 clearColor = glm::vec3(0.12f, 0.12f, 0.14f);
    glm::vec3 lightDir = glm::vec3(-0.2f, -1.0f, -0.3f);
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 ambientLight = glm::vec3(0.2f, 0.2f, 0.2f);
};

class SceneIO {
public:
    // Guardar y cargar la escena completa en formato propio .scene
    static bool saveScene(const std::string& filePath,
                          const Camera& camera,
                          const EnvironmentData& env,
                          const RenderSettingsData& renderSettings,
                          const std::vector<std::unique_ptr<Model>>& models);

    static bool loadScene(const std::string& filePath,
                          Camera& camera,
                          EnvironmentData& env,
                          RenderSettingsData& renderSettings,
                          std::vector<std::unique_ptr<Model>>& models);

    // Exportar la geometría modificada como un nuevo archivo .obj con su correspondiente archivo .mtl
    static bool exportModelToOBJ(const std::string& objFilePath,
                                 const Model& model);
};
