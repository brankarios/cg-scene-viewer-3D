#include "SceneIO.h"
#include "PrimitiveGenerator.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <filesystem>

namespace fs = std::filesystem;


bool SceneIO::saveScene(const std::string& filePath,
                        const Camera& camera,
                        const EnvironmentData& env,
                        const RenderSettingsData& renderSettings,
                        const std::vector<std::unique_ptr<Model>>& models) {
    std::ofstream out(filePath);
    if (!out.is_open()) {
        std::cerr << "Error: No se pudo crear el archivo de escena: " << filePath << std::endl;
        return false;
    }

    out << std::fixed << std::setprecision(6);
    out << "# CG Scene Viewer 3D - Scene Format v1.0\n\n";

    // ── Cámara ──
    out << "[CAMERA]\n";
    out << "distance " << camera.distance << "\n";
    out << "yaw " << camera.yaw << "\n";
    out << "pitch " << camera.pitch << "\n";
    out << "target " << camera.target.x << " " << camera.target.y << " " << camera.target.z << "\n\n";

    // ── Entorno y Luces ──
    out << "[ENVIRONMENT]\n";
    out << "clearColor " << env.clearColor.r << " " << env.clearColor.g << " " << env.clearColor.b << "\n";
    out << "lightDir " << env.lightDir.x << " " << env.lightDir.y << " " << env.lightDir.z << "\n";
    out << "lightColor " << env.lightColor.r << " " << env.lightColor.g << " " << env.lightColor.b << "\n";
    out << "ambientLight " << env.ambientLight.r << " " << env.ambientLight.g << " " << env.ambientLight.b << "\n\n";

    // ── Opciones de Render ──
    out << "[RENDER_SETTINGS]\n";
    out << "depthTest " << (renderSettings.depthTest ? 1 : 0) << "\n";
    out << "cullFace " << (renderSettings.cullFace ? 1 : 0) << "\n";
    out << "wireframe " << (renderSettings.wireframe ? 1 : 0) << "\n";
    out << "showVertices " << (renderSettings.showVertices ? 1 : 0) << "\n";
    out << "vertexSize " << renderSettings.vertexSize << "\n";
    out << "showNormals " << (renderSettings.showNormals ? 1 : 0) << "\n";
    out << "normalLength " << renderSettings.normalLength << "\n";
    out << "normalColor " << renderSettings.normalColor.r << " " << renderSettings.normalColor.g << " "
                          << renderSettings.normalColor.b << " " << renderSettings.normalColor.a << "\n";
    out << "showBoundingBox " << (renderSettings.showBoundingBox ? 1 : 0) << "\n\n";

    // ── Modelos y Submallas ──
    out << "[MODELS_COUNT] " << models.size() << "\n\n";

    for (size_t i = 0; i < models.size(); i++) {
        const auto& model = models[i];
        out << "[MODEL " << i << "]\n";
        out << "filePath " << model->filePath << "\n";
        out << "primitiveConfig " << model->primitiveConfig << "\n";
        out << "name " << model->name << "\n";
        out << "position " << model->position.x << " " << model->position.y << " " << model->position.z << "\n";
        out << "rotation " << model->rotation.x << " " << model->rotation.y << " " << model->rotation.z << "\n";
        out << "scale " << model->scale.x << " " << model->scale.y << " " << model->scale.z << "\n";
        out << "diffuseColor " << model->diffuseColor.r << " " << model->diffuseColor.g << " "
                               << model->diffuseColor.b << " " << model->diffuseColor.a << "\n";
        out << "submeshesCount " << model->meshes.size() << "\n\n";

        for (size_t j = 0; j < model->meshes.size(); j++) {
            const auto& mesh = model->meshes[j];
            out << "[SUBMESH " << j << "]\n";
            out << "name " << (mesh.name.empty() ? ("Submalla_" + std::to_string(j)) : mesh.name) << "\n";
            out << "position " << mesh.position.x << " " << mesh.position.y << " " << mesh.position.z << "\n";
            out << "rotation " << mesh.rotation.x << " " << mesh.rotation.y << " " << mesh.rotation.z << "\n";
            out << "scale " << mesh.scale.x << " " << mesh.scale.y << " " << mesh.scale.z << "\n";
            out << "color " << mesh.color.r << " " << mesh.color.g << " "
                            << mesh.color.b << " " << mesh.color.a << "\n\n";
        }
    }

    std::cout << "Escena guardada exitosamente en: " << filePath << std::endl;
    return true;
}

// ────────────────────────────────────────────
// 2. Cargar Escena desde formato propio .scene
// ────────────────────────────────────────────

bool SceneIO::loadScene(const std::string& filePath,
                        Camera& camera,
                        EnvironmentData& env,
                        RenderSettingsData& renderSettings,
                        std::vector<std::unique_ptr<Model>>& models) {
    std::ifstream in(filePath);
    if (!in.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo de escena: " << filePath << std::endl;
        return false;
    }

    fs::path sceneDir = fs::path(filePath).parent_path();

    std::string line;
    std::string currentSection = "";
    int currentModelIdx = -1;
    int currentSubmeshIdx = -1;

    std::vector<std::unique_ptr<Model>> loadedModels;

    while (std::getline(in, line)) {
        // Limpiar espacios y retornos de carro
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ' || line.back() == '\t')) {
            line.pop_back();
        }
        size_t firstChar = line.find_first_not_of(" \t");
        if (firstChar == std::string::npos) continue;
        line = line.substr(firstChar);
        if (line.empty() || line[0] == '#') continue;

        // Detectar cambio de sección entre corchetes: [SECTION ...]
        if (line.front() == '[' && line.back() == ']') {
            std::string sectionContent = line.substr(1, line.size() - 2);
            std::stringstream secStream(sectionContent);
            std::string secType;
            secStream >> secType;

            if (secType == "CAMERA" || secType == "ENVIRONMENT" || secType == "RENDER_SETTINGS") {
                currentSection = secType;
            } else if (secType == "MODEL") {
                currentSection = "MODEL";
                secStream >> currentModelIdx;
                loadedModels.push_back(std::make_unique<Model>());
            } else if (secType == "SUBMESH") {
                currentSection = "SUBMESH";
                secStream >> currentSubmeshIdx;
            }
            continue;
        }

        std::stringstream ss(line);
        std::string tag;
        ss >> tag;

        // Parsear contenido según la sección activa
        if (currentSection == "CAMERA") {
            if (tag == "distance") ss >> camera.distance;
            else if (tag == "yaw") ss >> camera.yaw;
            else if (tag == "pitch") ss >> camera.pitch;
            else if (tag == "target") ss >> camera.target.x >> camera.target.y >> camera.target.z;
        } else if (currentSection == "ENVIRONMENT") {
            if (tag == "clearColor") ss >> env.clearColor.r >> env.clearColor.g >> env.clearColor.b;
            else if (tag == "lightDir") ss >> env.lightDir.x >> env.lightDir.y >> env.lightDir.z;
            else if (tag == "lightColor") ss >> env.lightColor.r >> env.lightColor.g >> env.lightColor.b;
            else if (tag == "ambientLight") ss >> env.ambientLight.r >> env.ambientLight.g >> env.ambientLight.b;
        } else if (currentSection == "RENDER_SETTINGS") {
            int val = 0;
            if (tag == "depthTest") { ss >> val; renderSettings.depthTest = (val != 0); }
            else if (tag == "cullFace") { ss >> val; renderSettings.cullFace = (val != 0); }
            else if (tag == "wireframe") { ss >> val; renderSettings.wireframe = (val != 0); }
            else if (tag == "showVertices") { ss >> val; renderSettings.showVertices = (val != 0); }
            else if (tag == "vertexSize") ss >> renderSettings.vertexSize;
            else if (tag == "showNormals") { ss >> val; renderSettings.showNormals = (val != 0); }
            else if (tag == "normalLength") ss >> renderSettings.normalLength;
            else if (tag == "normalColor") ss >> renderSettings.normalColor.r >> renderSettings.normalColor.g
                                              >> renderSettings.normalColor.b >> renderSettings.normalColor.a;
            else if (tag == "showBoundingBox") { ss >> val; renderSettings.showBoundingBox = (val != 0); }
        } else if (currentSection == "MODEL" && !loadedModels.empty()) {
            auto& curModel = loadedModels.back();
            if (tag == "filePath") {
                std::string objPath;
                std::getline(ss >> std::ws, objPath);
                while (!objPath.empty() && (objPath.back() == '\r' || objPath.back() == ' ' || objPath.back() == '\t')) {
                    objPath.pop_back();
                }
                if (objPath.size() >= 2 && objPath.front() == '"' && objPath.back() == '"') {
                    objPath = objPath.substr(1, objPath.size() - 2);
                }
                
                // Si la ruta original no existe directamente, intentar buscar relativa a la escena
                if (!objPath.empty()) {
                    if (!fs::exists(objPath)) {
                        fs::path candidate = sceneDir / fs::path(objPath).filename();
                        if (fs::exists(candidate)) {
                            objPath = candidate.string();
                        }
                    }
                    if (!curModel->loadFromFile(objPath)) {
                        std::cerr << "Error al cargar modelo en la escena desde: " << objPath << std::endl;
                    }
                }
            } else if (tag == "primitiveConfig") {
                std::string primConfig;
                std::getline(ss >> std::ws, primConfig);
                while (!primConfig.empty() && (primConfig.back() == '\r' || primConfig.back() == ' ' || primConfig.back() == '\t')) {
                    primConfig.pop_back();
                }
                curModel->primitiveConfig = primConfig;
                if (!primConfig.empty()) {
                    std::stringstream pss(primConfig);
                    std::string pType;
                    pss >> pType;
                    if (pType == "cube") {
                        float sz = 1.0f;
                        pss >> sz;
                        auto temp = PrimitiveGenerator::createCube(sz);
                        curModel->meshes = std::move(temp->meshes);
                        curModel->minBounds = temp->minBounds;
                        curModel->maxBounds = temp->maxBounds;
                    } else if (pType == "pyramid") {
                        float w = 1.0f, h = 1.2f;
                        pss >> w >> h;
                        auto temp = PrimitiveGenerator::createPyramid(w, h);
                        curModel->meshes = std::move(temp->meshes);
                        curModel->minBounds = temp->minBounds;
                        curModel->maxBounds = temp->maxBounds;
                    } else if (pType == "sphere") {
                        float r = 0.8f; int sec = 32, stk = 16;
                        pss >> r >> sec >> stk;
                        auto temp = PrimitiveGenerator::createSphere(r, sec, stk);
                        curModel->meshes = std::move(temp->meshes);
                        curModel->minBounds = temp->minBounds;
                        curModel->maxBounds = temp->maxBounds;
                    } else if (pType == "cylinder") {
                        float r = 0.6f, h = 1.2f; int seg = 32;
                        pss >> r >> h >> seg;
                        auto temp = PrimitiveGenerator::createCylinder(r, h, seg);
                        curModel->meshes = std::move(temp->meshes);
                        curModel->minBounds = temp->minBounds;
                        curModel->maxBounds = temp->maxBounds;
                    }
                }
            } else if (tag == "name") {
                std::getline(ss >> std::ws, curModel->name);
            } else if (tag == "position") {
                ss >> curModel->position.x >> curModel->position.y >> curModel->position.z;
            } else if (tag == "rotation") {
                ss >> curModel->rotation.x >> curModel->rotation.y >> curModel->rotation.z;
            } else if (tag == "scale") {
                ss >> curModel->scale.x >> curModel->scale.y >> curModel->scale.z;
            } else if (tag == "diffuseColor") {
                ss >> curModel->diffuseColor.r >> curModel->diffuseColor.g
                   >> curModel->diffuseColor.b >> curModel->diffuseColor.a;
                curModel->setDiffuseColor(curModel->diffuseColor);
            }
        } else if (currentSection == "SUBMESH" && !loadedModels.empty()) {
            auto& curModel = loadedModels.back();
            if (currentSubmeshIdx >= 0 && currentSubmeshIdx < static_cast<int>(curModel->meshes.size())) {
                auto& curMesh = curModel->meshes[currentSubmeshIdx];
                if (tag == "position") {
                    ss >> curMesh.position.x >> curMesh.position.y >> curMesh.position.z;
                } else if (tag == "rotation") {
                    ss >> curMesh.rotation.x >> curMesh.rotation.y >> curMesh.rotation.z;
                } else if (tag == "scale") {
                    ss >> curMesh.scale.x >> curMesh.scale.y >> curMesh.scale.z;
                } else if (tag == "color") {
                    ss >> curMesh.color.r >> curMesh.color.g >> curMesh.color.b >> curMesh.color.a;
                }
            }
        }
    }

    // Actualizar vectores direccionales y matrices de la cámara
    camera.updateCameraVectors();

    // Actualizar vector de modelos
    models = std::move(loadedModels);

    std::cout << "Escena cargada exitosamente: " << filePath << std::endl;
    return true;
}

// ────────────────────────────────────────────
// 3. Exportar Modelo como .obj y .mtl propio
// ────────────────────────────────────────────

bool SceneIO::exportModelToOBJ(const std::string& objFilePath, const Model& model) {
    if (model.meshes.empty()) {
        std::cerr << "Error: El modelo no tiene mallas para exportar." << std::endl;
        return false;
    }

    fs::path objPath(objFilePath);
    std::string baseName = objPath.stem().string();
    std::string mtlFileName = baseName + ".mtl";
    fs::path mtlPath = objPath.parent_path() / mtlFileName;

    // ── Escribir archivo .mtl ──
    std::ofstream mtlFile(mtlPath);
    if (!mtlFile.is_open()) {
        std::cerr << "Error: No se pudo crear el archivo de material: " << mtlPath << std::endl;
        return false;
    }

    mtlFile << "# Material Library exportada por CG Scene Viewer 3D\n";
    mtlFile << "# Total de materiales: " << model.meshes.size() << "\n\n";

    for (size_t i = 0; i < model.meshes.size(); i++) {
        const auto& mesh = model.meshes[i];
        std::string matName = mesh.name.empty() ? ("Material_Submalla_" + std::to_string(i)) : (mesh.name + "_Mat");

        mtlFile << "newmtl " << matName << "\n";
        mtlFile << "Ka 0.2 0.2 0.2\n";
        mtlFile << "Kd " << mesh.color.r << " " << mesh.color.g << " " << mesh.color.b << "\n";
        mtlFile << "Ks 0.0 0.0 0.0\n";
        mtlFile << "d " << mesh.color.a << "\n";
        mtlFile << "illum 2\n\n";
    }
    mtlFile.close();

    // ── Escribir archivo .obj ──
    std::ofstream objFile(objPath);
    if (!objFile.is_open()) {
        std::cerr << "Error: No se pudo crear el archivo OBJ: " << objPath << std::endl;
        return false;
    }

    objFile << std::fixed << std::setprecision(6);
    objFile << "# Wavefront OBJ exportado por CG Scene Viewer 3D\n";
    objFile << "# Modelo: " << model.name << "\n";
    objFile << "mtllib " << mtlFileName << "\n\n";

    glm::mat4 modelMat = model.getModelMatrix();
    unsigned int vertexOffset = 1; // Índices de OBJ son 1-based

    for (size_t meshIdx = 0; meshIdx < model.meshes.size(); meshIdx++) {
        const auto& mesh = model.meshes[meshIdx];
        std::string submeshName = mesh.name.empty() ? ("Submalla_" + std::to_string(meshIdx)) : mesh.name;
        std::string matName = mesh.name.empty() ? ("Material_Submalla_" + std::to_string(meshIdx)) : (mesh.name + "_Mat");

        // Matriz combinada: Transformación global del modelo * Transformación local de la submalla
        glm::mat4 finalMat = modelMat * mesh.getLocalModelMatrix();
        glm::mat3 normalMat = glm::mat3(glm::transpose(glm::inverse(finalMat)));

        objFile << "g " << submeshName << "\n";
        objFile << "usemtl " << matName << "\n";

        // Vértices transformados en coordenadas de mundo
        for (const auto& v : mesh.vertices) {
            glm::vec4 worldPos = finalMat * glm::vec4(v.position, 1.0f);
            objFile << "v " << worldPos.x << " " << worldPos.y << " " << worldPos.z << "\n";
        }

        // Normales transformadas
        for (const auto& v : mesh.vertices) {
            glm::vec3 worldNorm = glm::normalize(normalMat * v.normal);
            objFile << "vn " << worldNorm.x << " " << worldNorm.y << " " << worldNorm.z << "\n";
        }

        // Caras poligonales (triángulos con v//vn)
        for (size_t i = 0; i < mesh.indices.size(); i += 3) {
            unsigned int idx0 = mesh.indices[i] + vertexOffset;
            unsigned int idx1 = mesh.indices[i + 1] + vertexOffset;
            unsigned int idx2 = mesh.indices[i + 2] + vertexOffset;

            objFile << "f " << idx0 << "//" << idx0 << " "
                            << idx1 << "//" << idx1 << " "
                            << idx2 << "//" << idx2 << "\n";
        }

        objFile << "\n";
        vertexOffset += static_cast<unsigned int>(mesh.vertices.size());
    }

    objFile.close();
    std::cout << "Modelo exportado exitosamente a OBJ: " << objFilePath << std::endl;
    std::cout << "Material exportado exitosamente a MTL: " << mtlPath.string() << std::endl;
    return true;
}
