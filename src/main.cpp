#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Shader.h"
#include "Model.h"
#include "Camera.h"
#include "PickingFBO.h"
#include "BoundingBox.h"
#include "SceneIO.h"
#include "PrimitiveGenerator.h"

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>

enum class InteractionMode {
    Rotate,     
    Translate,  
    Scale,      
    Navigate    
};

enum class SelectionSpecificity {
    Global, 
    Local   
};
static Camera g_camera(glm::vec3(0.0f, 0.0f, 0.0f), 3.5f);
static InteractionMode g_mode = InteractionMode::Rotate;
static SelectionSpecificity g_specificity = SelectionSpecificity::Global;

static int g_selectedModel = -1;
static int g_selectedMesh = -1;

static bool g_isLeftDragging = false;
static bool g_isRightDragging = false;
static bool g_isMiddleDragging = false;
static double g_lastMouseX = 0.0;
static double g_lastMouseY = 0.0;

static PickingFBO g_pickingFBO;

// Visualización y render
static bool g_wireframe = false;
static bool g_showVertices = false;
static float g_vertexSize = 5.0f;
static bool g_showNormals = false;
static float g_normalLength = 0.06f;
static glm::vec4 g_normalColor(0.0f, 1.0f, 1.0f, 1.0f);
static bool g_showBoundingBox = false;
static bool g_depthTest = true;
static bool g_cullFace = false;

// Primitivas
static int g_selectedPrimitive = 0;
static float g_cubeSize = 1.0f;
static float g_pyramidWidth = 1.0f;
static float g_pyramidHeight = 1.2f;
static float g_sphereRadius = 0.8f;
static int g_sphereSectors = 32;
static int g_sphereStacks = 16;
static float g_cylinderRadius = 0.6f;
static float g_cylinderHeight = 1.2f;
static int g_cylinderSegments = 32;
static glm::vec4 g_primitiveColor(0.8f, 0.8f, 0.8f, 1.0f);

// Barra lateral
static bool g_sidebarOpen = true;
static float g_sidebarWidth = 340.0f;

static inline float getEffectiveSidebarWidth() {
    return g_sidebarOpen ? g_sidebarWidth : 0.0f;
}

std::string openFileDialog() {
#ifdef _WIN32
    char filename[MAX_PATH] = "";
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "OBJ Files\0*.obj\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn)) {
        return std::string(filename);
    }
#endif
    return "";
}

std::string openMaterialFileDialog() {
#ifdef _WIN32
    char filename[MAX_PATH] = "";
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "Material Files (*.mtl;*.mlt)\0*.mtl;*.mlt\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn)) {
        return std::string(filename);
    }
#endif
    return "";
}

std::string saveSceneFileDialog() {
#ifdef _WIN32
    char filename[MAX_PATH] = "mi_escena.scene";
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "Scene Files (*.scene)\0*.scene\0All Files\0*.*\0";
    ofn.lpstrDefExt = "scene";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    if (GetSaveFileNameA(&ofn)) {
        return std::string(filename);
    }
#endif
    return "";
}

std::string openSceneFileDialog() {
#ifdef _WIN32
    char filename[MAX_PATH] = "";
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "Scene Files (*.scene)\0*.scene\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn)) {
        return std::string(filename);
    }
#endif
    return "";
}

std::string exportOBJFileDialog(const std::string& defaultName = "modelo_exportado.obj") {
#ifdef _WIN32
    char filename[MAX_PATH] = "";
    strncpy_s(filename, defaultName.c_str(), sizeof(filename) - 1);
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "Wavefront OBJ (*.obj)\0*.obj\0All Files\0*.*\0";
    ofn.lpstrDefExt = "obj";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    if (GetSaveFileNameA(&ofn)) {
        return std::string(filename);
    }
#endif
    return "";
}

// Callback para redimensionar la ventana
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    if (width > 0 && height > 0) {
        g_pickingFBO.resize(width, height);
    }
}

// Callback para la rueda del ratón (Zoom)
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    g_camera.processMouseScroll(static_cast<float>(yoffset));
}

// Picking
struct PickTarget {
    int modelIndex;
    int meshIndex;
};

void performPicking(int mouseX, int mouseY,
                    int display_w, int display_h,
                    const glm::mat4& view,
                    const glm::mat4& projection,
                    const std::vector<std::unique_ptr<Model>>& models,
                    Shader& pickingShader,
                    PickingFBO& pickingFBO,
                    SelectionSpecificity specificity,
                    int& outSelectedModel,
                    int& outSelectedMesh) {
    if (models.empty() || display_w <= 0 || display_h <= 0) {
        outSelectedModel = -1;
        outSelectedMesh = -1;
        return;
    }

    int sidebarPixels = static_cast<int>(getEffectiveSidebarWidth());
    int sceneW = display_w - sidebarPixels;
    if (sceneW < 1) sceneW = 1;
    pickingFBO.resize(sceneW, display_h);
    pickingFBO.bind();

    glViewport(0, 0, sceneW, display_h);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    pickingShader.use();
    pickingShader.setMat4("view", view);
    pickingShader.setMat4("projection", projection);

    std::map<int, PickTarget> idMap;

    if (specificity == SelectionSpecificity::Global) {
        for (size_t i = 0; i < models.size(); i++) {
            int id = static_cast<int>(i) + 1;
            idMap[id] = { static_cast<int>(i), -1 };

            glm::mat4 parentMatrix = models[i]->getModelMatrix();
            pickingShader.setVec4("pickingColor", PickingFBO::idToColor(id));

            for (const auto& mesh : models[i]->meshes) {
                pickingShader.setMat4("model", parentMatrix * mesh.getLocalModelMatrix());
                mesh.draw();
            }
        }
    } else {
        int currentId = 1;
        for (size_t i = 0; i < models.size(); i++) {
            glm::mat4 parentMatrix = models[i]->getModelMatrix();

            for (size_t j = 0; j < models[i]->meshes.size(); j++) {
                idMap[currentId] = { static_cast<int>(i), static_cast<int>(j) };
                pickingShader.setMat4("model", parentMatrix * models[i]->meshes[j].getLocalModelMatrix());
                pickingShader.setVec4("pickingColor", PickingFBO::idToColor(currentId));
                models[i]->meshes[j].draw();
                currentId++;
            }
        }
    }

    int detectedId = pickingFBO.readPixel(mouseX, mouseY);
    pickingFBO.unbind();

    if (detectedId > 0 && idMap.find(detectedId) != idMap.end()) {
        outSelectedModel = idMap[detectedId].modelIndex;
        outSelectedMesh = idMap[detectedId].meshIndex;
    } else {
        outSelectedModel = -1;
        outSelectedMesh = -1;
    }
}

int main() {
    // Inicializar GLFW
    if (!glfwInit()) {
        std::cerr << "Error: No se pudo inicializar GLFW" << std::endl;
        return -1;
    }

    // Configurar OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Crear ventana
    GLFWwindow* window = glfwCreateWindow(1280, 720, "CG Scene Viewer 3D", nullptr, nullptr);
    if (!window) {
        std::cerr << "Error: No se pudo crear la ventana GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Error: No se pudo inicializar GLAD" << std::endl;
        return -1;
    }

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GPU:    " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "GLSL:   " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    // Shaders
    Shader baseShader;
    if (!baseShader.load("shaders/base.vert", "shaders/base.frag")) {
        std::cerr << "Error: No se pudieron cargar los shaders base" << std::endl;
        return -1;
    }

    Shader pickingShader;
    if (!pickingShader.load("shaders/picking.vert", "shaders/picking.frag")) {
        std::cerr << "Error: No se pudieron cargar los shaders de picking" << std::endl;
        return -1;
    }

    Shader normalsShader;
    if (!normalsShader.load("shaders/normals.vert", "shaders/normals.frag", "shaders/normals.geom")) {
        std::cerr << "Advertencia: No se pudieron cargar los shaders de normales" << std::endl;
    }

    Shader flatShader;
    if (!flatShader.load("shaders/flat.vert", "shaders/flat.frag")) {
        std::cerr << "Advertencia: No se pudieron cargar los shaders planos" << std::endl;
    }

    BoundingBox g_boundingBox;
    g_boundingBox.init();

    g_pickingFBO.init(1280, 720);

    // ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;
    style.ScrollbarRounding = 6.0f;
    style.ChildRounding = 4.0f;
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.WindowPadding = ImVec2(14.0f, 10.0f);
    style.FramePadding = ImVec2(8.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 8.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]           = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_ChildBg]            = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_PopupBg]            = ImVec4(0.10f, 0.10f, 0.13f, 0.96f);
    colors[ImGuiCol_Border]             = ImVec4(0.22f, 0.22f, 0.26f, 0.60f);
    colors[ImGuiCol_FrameBg]            = ImVec4(0.16f, 0.16f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.22f, 0.22f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBgActive]      = ImVec4(0.28f, 0.28f, 0.36f, 1.00f);
    colors[ImGuiCol_TitleBg]            = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgActive]      = ImVec4(0.12f, 0.12f, 0.16f, 1.00f);
    colors[ImGuiCol_MenuBarBg]          = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]        = ImVec4(0.10f, 0.10f, 0.12f, 0.80f);
    colors[ImGuiCol_ScrollbarGrab]      = ImVec4(0.30f, 0.30f, 0.38f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.38f, 0.38f, 0.48f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.45f, 0.45f, 0.56f, 1.00f);
    colors[ImGuiCol_CheckMark]          = ImVec4(0.33f, 0.75f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab]         = ImVec4(0.33f, 0.65f, 1.00f, 0.80f);
    colors[ImGuiCol_SliderGrabActive]   = ImVec4(0.40f, 0.75f, 1.00f, 1.00f);
    colors[ImGuiCol_Button]             = ImVec4(0.18f, 0.18f, 0.24f, 1.00f);
    colors[ImGuiCol_ButtonHovered]      = ImVec4(0.28f, 0.36f, 0.54f, 1.00f);
    colors[ImGuiCol_ButtonActive]       = ImVec4(0.22f, 0.44f, 0.72f, 1.00f);
    colors[ImGuiCol_Header]             = ImVec4(0.18f, 0.24f, 0.36f, 0.80f);
    colors[ImGuiCol_HeaderHovered]      = ImVec4(0.24f, 0.34f, 0.52f, 0.80f);
    colors[ImGuiCol_HeaderActive]       = ImVec4(0.28f, 0.42f, 0.62f, 1.00f);
    colors[ImGuiCol_Separator]          = ImVec4(0.24f, 0.24f, 0.30f, 0.60f);
    colors[ImGuiCol_SeparatorHovered]   = ImVec4(0.33f, 0.55f, 0.82f, 0.80f);
    colors[ImGuiCol_Tab]                = ImVec4(0.16f, 0.18f, 0.24f, 1.00f);
    colors[ImGuiCol_TabHovered]         = ImVec4(0.28f, 0.38f, 0.56f, 0.80f);
    colors[ImGuiCol_TabSelected]        = ImVec4(0.22f, 0.34f, 0.52f, 1.00f);
    colors[ImGuiCol_TextDisabled]       = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    std::vector<std::unique_ptr<Model>> models;
    glm::vec3 clearColor(0.12f, 0.12f, 0.14f);

    glm::vec3 lightDir(-0.2f, -1.0f, -0.3f);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
    glm::vec3 ambientLight(0.2f, 0.2f, 0.2f);

    glEnable(GL_DEPTH_TEST);

    float lastFrameTime = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(window)) {
        float currentFrameTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        glfwPollEvents();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        // Vista y proyección
        float effectiveSidebarW = getEffectiveSidebarWidth();
        float sceneW = static_cast<float>(display_w) - effectiveSidebarW;
        if (sceneW < 1.0f) sceneW = 1.0f;
        float aspect = (display_h > 0) ? sceneW / static_cast<float>(display_h) : 1.0f;
        glm::mat4 view = g_camera.getViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(g_camera.zoom), aspect, 0.1f, 100.0f);

        // Teclado
        if (!io.WantCaptureKeyboard) {
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                g_camera.processKeyboard(CameraMovement::FORWARD, deltaTime);
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                g_camera.processKeyboard(CameraMovement::BACKWARD, deltaTime);
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                g_camera.processKeyboard(CameraMovement::LEFT, deltaTime);
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                g_camera.processKeyboard(CameraMovement::RIGHT, deltaTime);
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
                g_camera.processKeyboard(CameraMovement::UP, deltaTime);
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
                g_camera.processKeyboard(CameraMovement::DOWN, deltaTime);

            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) g_mode = InteractionMode::Rotate;
            if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) g_mode = InteractionMode::Translate;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) != GLFW_PRESS)
                g_mode = InteractionMode::Scale;
            if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) g_mode = InteractionMode::Navigate;
        }

        // Mouse
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        float mouseDeltaX = static_cast<float>(mouseX - g_lastMouseX);
        float mouseDeltaY = static_cast<float>(mouseY - g_lastMouseY);
        g_lastMouseX = mouseX;
        g_lastMouseY = mouseY;

        bool leftDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        bool rightDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
        bool middleDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;

        if (!io.WantCaptureMouse) {
            if (leftDown && !g_isLeftDragging) {
                g_isLeftDragging = true;

                int win_w, win_h;
                glfwGetWindowSize(window, &win_w, &win_h);
                float scaleX = (win_w > 0) ? static_cast<float>(display_w) / win_w : 1.0f;
                float scaleY = (win_h > 0) ? static_cast<float>(display_h) / win_h : 1.0f;
                int sidebarOffset = static_cast<int>(getEffectiveSidebarWidth());
                int fbMouseX = static_cast<int>(mouseX * scaleX) - sidebarOffset;
                int fbMouseY = static_cast<int>(mouseY * scaleY);

                // Picking
                int pickedModel = -1, pickedMesh = -1;
                if (fbMouseX >= 0) {
                    performPicking(fbMouseX, fbMouseY,
                                   display_w, display_h, view, projection,
                                   models, pickingShader, g_pickingFBO, g_specificity,
                                   pickedModel, pickedMesh);
                }

                if (pickedModel >= 0) {
                    g_selectedModel = pickedModel;
                    g_selectedMesh = pickedMesh;
                } else {
                    g_selectedModel = -1;
                    g_selectedMesh = -1;
                }
            }
            if (rightDown && !g_isRightDragging) g_isRightDragging = true;
            if (middleDown && !g_isMiddleDragging) g_isMiddleDragging = true;
        }
        if (!leftDown) g_isLeftDragging = false;
        if (!rightDown) g_isRightDragging = false;
        if (!middleDown) g_isMiddleDragging = false;

        // Orbitar / panear
        if (g_isRightDragging) {
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                g_camera.processMousePan(mouseDeltaX, mouseDeltaY);
            } else {
                g_camera.processMouseOrbit(mouseDeltaX, mouseDeltaY);
            }
        }

        // Pan
        if (g_isMiddleDragging) {
            g_camera.processMousePan(mouseDeltaX, mouseDeltaY);
        }

        // Transformación o navegación
        if (g_isLeftDragging) {
            bool hasModel = !models.empty() && g_selectedModel >= 0 && g_selectedModel < static_cast<int>(models.size());

            if (g_mode == InteractionMode::Navigate || !hasModel) {
                g_camera.processMouseOrbit(mouseDeltaX, mouseDeltaY);
            } else if (hasModel) {
                auto& model = models[g_selectedModel];
                bool isLocalSubmesh = (g_specificity == SelectionSpecificity::Local &&
                                       g_selectedMesh >= 0 &&
                                       g_selectedMesh < static_cast<int>(model->meshes.size()));

                if (isLocalSubmesh) {
                    auto& curMesh = model->meshes[g_selectedMesh];
                    if (g_mode == InteractionMode::Rotate) {
                        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
                            curMesh.rotation.z += mouseDeltaX * 0.5f;
                        } else {
                            curMesh.rotation.y += mouseDeltaX * 0.5f;
                            curMesh.rotation.x += mouseDeltaY * 0.5f;
                        }
                    } else if (g_mode == InteractionMode::Translate) {
                        float speed = 0.0025f * g_camera.distance;
                        glm::mat4 modelRot = glm::rotate(glm::mat4(1.0f), glm::radians(model->rotation.z), glm::vec3(0, 0, 1)) *
                                             glm::rotate(glm::mat4(1.0f), glm::radians(model->rotation.y), glm::vec3(0, 1, 0)) *
                                             glm::rotate(glm::mat4(1.0f), glm::radians(model->rotation.x), glm::vec3(1, 0, 0));
                        glm::mat3 invModelRot = glm::inverse(glm::mat3(modelRot));
                        glm::vec3 worldDelta = g_camera.right * (mouseDeltaX * speed) - g_camera.up * (mouseDeltaY * speed);
                        curMesh.position += invModelRot * worldDelta;
                    } else if (g_mode == InteractionMode::Scale) {
                        float factor = 1.0f + (mouseDeltaX - mouseDeltaY) * 0.008f;
                        curMesh.scale *= factor;
                        curMesh.scale = glm::max(curMesh.scale, glm::vec3(0.005f));
                    }
                } else {
                    if (g_mode == InteractionMode::Rotate) {
                        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
                            model->rotation.z += mouseDeltaX * 0.5f;
                        } else {
                            model->rotation.y += mouseDeltaX * 0.5f;
                            model->rotation.x += mouseDeltaY * 0.5f;
                        }
                    } else if (g_mode == InteractionMode::Translate) {
                        float speed = 0.0025f * g_camera.distance;
                        model->position += g_camera.right * (mouseDeltaX * speed) - g_camera.up * (mouseDeltaY * speed);
                    } else if (g_mode == InteractionMode::Scale) {
                        float factor = 1.0f + (mouseDeltaX - mouseDeltaY) * 0.008f;
                        model->scale *= factor;
                        model->scale = glm::max(model->scale, glm::vec3(0.005f));
                    }
                }
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Pestaña lateral
        const float tabWidth = 26.0f;
        const float tabHeight = 70.0f;
        float currentSidebarWidth = getEffectiveSidebarWidth();
        float tabY = glm::clamp(static_cast<float>(display_h) * 0.40f, 50.0f, static_cast<float>(display_h) - 100.0f);

        ImGui::SetNextWindowPos(ImVec2(currentSidebarWidth, tabY));
        ImGui::SetNextWindowSize(ImVec2(tabWidth, tabHeight));
        ImGuiWindowFlags tabFlags = ImGuiWindowFlags_NoTitleBar
                                  | ImGuiWindowFlags_NoResize
                                  | ImGuiWindowFlags_NoMove
                                  | ImGuiWindowFlags_NoScrollbar
                                  | ImGuiWindowFlags_NoBackground
                                  | ImGuiWindowFlags_NoSavedSettings;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::Begin("##SidebarTab", nullptr, tabFlags);

        ImVec2 tabP0 = ImGui::GetCursorScreenPos();
        ImVec2 tabP1 = ImVec2(tabP0.x + tabWidth, tabP0.y + tabHeight);
        ImGui::InvisibleButton("##TabHandle", ImVec2(tabWidth, tabHeight));
        bool isTabHovered = ImGui::IsItemHovered();
        bool isTabActive = ImGui::IsItemActive();

        static bool s_isDraggingTab = false;
        static float s_dragStartMouseX = 0.0f;

        if (ImGui::IsItemActivated()) {
            s_isDraggingTab = false;
            s_dragStartMouseX = io.MousePos.x;
        }

        if (isTabActive) {
            if (std::abs(io.MousePos.x - s_dragStartMouseX) > 4.0f) {
                s_isDraggingTab = true;
            }
            if (s_isDraggingTab) {
                if (io.MousePos.x < 90.0f) {
                    g_sidebarOpen = false;
                } else {
                    float maxSidebarW = glm::max(220.0f, static_cast<float>(display_w) - 150.0f);
                    g_sidebarWidth = glm::clamp(io.MousePos.x, 220.0f, glm::min(600.0f, maxSidebarW));
                }
            }
        }

        if (ImGui::IsItemDeactivated()) {
            if (!s_isDraggingTab) {
                g_sidebarOpen = !g_sidebarOpen;
            }
            s_isDraggingTab = false;
        }

        if (isTabHovered || isTabActive) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }

        ImDrawList* tabDrawList = ImGui::GetWindowDrawList();
        ImU32 tabBgColor = isTabActive  ? IM_COL32(45, 55, 75, 245) :
                           isTabHovered ? IM_COL32(35, 42, 58, 240) :
                                          IM_COL32(24, 26, 34, 225);
        ImU32 tabBorderColor = (isTabHovered || isTabActive) ? IM_COL32(90, 165, 245, 230) : IM_COL32(60, 68, 85, 180);
        ImU32 tabIconColor   = (isTabHovered || isTabActive) ? IM_COL32(255, 255, 255, 255) : IM_COL32(180, 195, 215, 220);

        tabDrawList->AddRectFilled(tabP0, tabP1, tabBgColor, 6.0f, ImDrawFlags_RoundCornersRight);
        tabDrawList->AddRect(tabP0, tabP1, tabBorderColor, 6.0f, ImDrawFlags_RoundCornersRight, 1.5f);

        const char* tabIconStr = g_sidebarOpen ? "<" : ">";
        ImVec2 tabTextSize = ImGui::CalcTextSize(tabIconStr);
        ImVec2 tabTextPos = ImVec2(tabP0.x + (tabWidth - tabTextSize.x) * 0.5f, tabP0.y + 12.0f);
        tabDrawList->AddText(tabTextPos, tabIconColor, tabIconStr);

        float gripX = tabP0.x + tabWidth * 0.5f;
        float gripStartY = tabP0.y + 34.0f;
        for (int i = 0; i < 3; i++) {
            tabDrawList->AddCircleFilled(ImVec2(gripX, gripStartY + i * 8.0f), 1.8f, tabIconColor);
        }

        if (isTabHovered && !s_isDraggingTab) {
            ImGui::BeginTooltip();
            if (g_sidebarOpen) {
                ImGui::Text("Ocultar menu (Click)");
                ImGui::TextDisabled("Arrastrar para cambiar tamano");
            } else {
                ImGui::Text("Expandir menu (Click o Arrastrar)");
            }
            ImGui::EndTooltip();
        }

        ImGui::End();
        ImGui::PopStyleVar(2);

        // Barra lateral
        if (g_sidebarOpen) {
            currentSidebarWidth = g_sidebarWidth;
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(currentSidebarWidth, static_cast<float>(display_h)));
            ImGuiWindowFlags sidebarFlags = ImGuiWindowFlags_NoMove
                                          | ImGuiWindowFlags_NoResize
                                          | ImGuiWindowFlags_NoCollapse
                                          | ImGuiWindowFlags_NoTitleBar;
            ImGui::Begin("##Sidebar", nullptr, sidebarFlags);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.78f, 1.0f, 1.0f));
        ImGui::Text("CG Scene Viewer 3D");
        ImGui::PopStyleColor();
        ImGui::SameLine(ImGui::GetWindowWidth() - 120.0f);
        float fps = io.Framerate;
        ImVec4 fpsColor = (fps >= 30.0f) ? ImVec4(0.2f, 1.0f, 0.3f, 1.0f) :
                          (fps >= 15.0f) ? ImVec4(1.0f, 0.85f, 0.2f, 1.0f) :
                                           ImVec4(1.0f, 0.3f, 0.2f, 1.0f);
        ImGui::TextColored(fpsColor, "%.0f FPS", fps);
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Cargar OBJ...", ImVec2(-1, 28))) {
            std::string path = openFileDialog();
            if (!path.empty()) {
                auto model = std::make_unique<Model>();
                if (model->loadFromFile(path)) {
                    models.clear();
                    models.push_back(std::move(model));
                    g_selectedModel = 0;
                    g_selectedMesh = -1;
                    g_camera.reset(glm::vec3(0.0f), 3.5f);
                } else {
                    std::cerr << "Error al cargar: " << path << std::endl;
                }
            }
        }

        if (ImGui::Button("Borrar Escena Completa", ImVec2(-1, 28))) {
            models.clear();
            g_selectedModel = -1;
            g_selectedMesh = -1;
            g_camera.reset(glm::vec3(0.0f, 0.0f, 0.0f), 3.5f);
        }

        if (ImGui::CollapsingHeader("Crear Objeto Parametrizable", ImGuiTreeNodeFlags_DefaultOpen)) {
            const char* primItems[] = { "Cubo", "Piramide", "Esfera", "Cilindro" };
            ImGui::Combo("Tipo de Figura", &g_selectedPrimitive, primItems, IM_ARRAYSIZE(primItems));

            if (g_selectedPrimitive == 0) {
                ImGui::SliderFloat("Tamano de Arista", &g_cubeSize, 0.1f, 5.0f, "%.2f");
            } else if (g_selectedPrimitive == 1) {
                ImGui::SliderFloat("Ancho Base", &g_pyramidWidth, 0.1f, 5.0f, "%.2f");
                ImGui::SliderFloat("Altura", &g_pyramidHeight, 0.1f, 5.0f, "%.2f");
            } else if (g_selectedPrimitive == 2) {
                ImGui::SliderFloat("Radio", &g_sphereRadius, 0.1f, 3.0f, "%.2f");
                ImGui::SliderInt("Sectores (Meridianos)", &g_sphereSectors, 6, 64);
                ImGui::SliderInt("Pilas (Anillos)", &g_sphereStacks, 4, 32);
            } else if (g_selectedPrimitive == 3) {
                ImGui::SliderFloat("Radio", &g_cylinderRadius, 0.1f, 3.0f, "%.2f");
                ImGui::SliderFloat("Altura", &g_cylinderHeight, 0.1f, 5.0f, "%.2f");
                ImGui::SliderInt("Segmentos Radiales", &g_cylinderSegments, 6, 64);
            }

            ImGui::ColorEdit4("Color Inicial", &g_primitiveColor.x);

            if (ImGui::Button("Generar y Cargar en Escena", ImVec2(-1, 28))) {
                std::unique_ptr<Model> newModel = nullptr;
                if (g_selectedPrimitive == 0) {
                    newModel = PrimitiveGenerator::createCube(g_cubeSize, g_primitiveColor);
                } else if (g_selectedPrimitive == 1) {
                    newModel = PrimitiveGenerator::createPyramid(g_pyramidWidth, g_pyramidHeight, g_primitiveColor);
                } else if (g_selectedPrimitive == 2) {
                    newModel = PrimitiveGenerator::createSphere(g_sphereRadius, g_sphereSectors, g_sphereStacks, g_primitiveColor);
                } else if (g_selectedPrimitive == 3) {
                    newModel = PrimitiveGenerator::createCylinder(g_cylinderRadius, g_cylinderHeight, g_cylinderSegments, g_primitiveColor);
                }

                if (newModel) {
                    models.clear(); // La escena mantiene un único objeto activo
                    models.push_back(std::move(newModel));
                    g_selectedModel = 0;
                    g_selectedMesh = -1;
                    g_camera.reset(glm::vec3(0.0f), 3.5f);
                }
            }
        }

        if (ImGui::Button("Guardar Escena...", ImVec2(150, 26))) {
            std::string savePath = saveSceneFileDialog();
            if (!savePath.empty()) {
                EnvironmentData env{ clearColor, lightDir, lightColor, ambientLight };
                RenderSettingsData rs{
                    g_depthTest, g_cullFace, g_wireframe,
                    g_showVertices, g_vertexSize,
                    g_showNormals, g_normalLength, g_normalColor,
                    g_showBoundingBox
                };
                SceneIO::saveScene(savePath, g_camera, env, rs, models);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cargar Escena...", ImVec2(-1, 26))) {
            std::string loadPath = openSceneFileDialog();
            if (!loadPath.empty()) {
                EnvironmentData env;
                RenderSettingsData rs;
                if (SceneIO::loadScene(loadPath, g_camera, env, rs, models)) {
                    clearColor = env.clearColor;
                    lightDir = env.lightDir;
                    lightColor = env.lightColor;
                    ambientLight = env.ambientLight;

                    g_depthTest = rs.depthTest;
                    g_cullFace = rs.cullFace;
                    g_wireframe = rs.wireframe;
                    g_showVertices = rs.showVertices;
                    g_vertexSize = rs.vertexSize;
                    g_showNormals = rs.showNormals;
                    g_normalLength = rs.normalLength;
                    g_normalColor = rs.normalColor;
                    g_showBoundingBox = rs.showBoundingBox;

                    if (!models.empty()) {
                        g_selectedModel = 0;
                        g_selectedMesh = -1;
                    }
                }
            }
        }

        if (!models.empty()) {
            if (ImGui::Button("Exportar como OBJ (.obj + .mtl)...", ImVec2(-1, 28))) {
                std::string defaultName = models[0]->name + "_exportado.obj";
                std::string exportPath = exportOBJFileDialog(defaultName);
                if (!exportPath.empty()) {
                    SceneIO::exportModelToOBJ(exportPath, *models[0]);
                }
            }
        }

        if (models.empty()) {
            ImGui::TextDisabled("Ningun modelo cargado");
        } else {
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Modelo cargado: %s", models[0]->name.c_str());
        }
        ImGui::Separator();

        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Herramienta Activa (Mouse):");

        auto modeButton = [](const char* label, InteractionMode mode) {
            bool isActive = (g_mode == mode);
            if (isActive) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.85f, 1.0f));
            }
            if (ImGui::Button(label, ImVec2(75, 26))) {
                g_mode = mode;
            }
            if (isActive) {
                ImGui::PopStyleColor();
            }
        };

        modeButton("Rotar", InteractionMode::Rotate);
        ImGui::SameLine();
        modeButton("Mover", InteractionMode::Translate);
        ImGui::SameLine();
        modeButton("Escalar", InteractionMode::Scale);
        ImGui::SameLine();
        modeButton("Camara", InteractionMode::Navigate);

        ImGui::TextDisabled("Atajos: [R] Rotar, [G] Mover, [S] Escalar, [C] Camara");
        ImGui::Separator();

        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Especificidad de Seleccion:");
        if (ImGui::RadioButton("Global (Objeto completo)", g_specificity == SelectionSpecificity::Global)) {
            g_specificity = SelectionSpecificity::Global;
            g_selectedMesh = -1;
        }
        if (ImGui::RadioButton("Local (Submalla individual)", g_specificity == SelectionSpecificity::Local)) {
            g_specificity = SelectionSpecificity::Local;
        }
        ImGui::TextDisabled("Haz clic izquierdo en la escena 3D para seleccionar.");
        ImGui::Separator();

        if (models.empty()) {
            ImGui::TextDisabled("No hay modelos en la escena.");
            ImGui::TextDisabled("Haz clic en 'Cargar OBJ...' para comenzar.");
        } else {
            ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1.0f), "Objetos en Escena:");

            for (size_t i = 0; i < models.size(); i++) {
                auto& model = models[i];
                bool isModelSelected = (static_cast<int>(i) == g_selectedModel);

                std::string header = (isModelSelected ? "> " : "  ") + model->name + " (" + std::to_string(model->meshes.size()) + " mallas)";
                if (ImGui::Selectable(header.c_str(), isModelSelected)) {
                    g_selectedModel = static_cast<int>(i);
                    if (g_specificity == SelectionSpecificity::Global) {
                        g_selectedMesh = -1;
                    }
                }

                if (g_specificity == SelectionSpecificity::Local && isModelSelected && model->meshes.size() > 1) {
                    ImGui::Indent(20.0f);
                    ImGui::TextDisabled("Submallas del modelo:");
                    for (size_t j = 0; j < model->meshes.size(); j++) {
                        bool isMeshSelected = (isModelSelected && static_cast<int>(j) == g_selectedMesh);
                        std::string meshLabel = (isMeshSelected ? "* " : "  ") + 
                            (model->meshes[j].name.empty() ? ("Submalla " + std::to_string(j)) : model->meshes[j].name);

                        if (ImGui::Selectable(meshLabel.c_str(), isMeshSelected)) {
                            g_selectedModel = static_cast<int>(i);
                            g_selectedMesh = static_cast<int>(j);
                        }
                    }
                    ImGui::Unindent(20.0f);
                }
            }

            if (g_selectedModel >= 0 && g_selectedModel < static_cast<int>(models.size())) {
                auto& curModel = models[g_selectedModel];
                ImGui::Separator();
                ImGui::Text("Objeto Seleccionado: %s", curModel->name.c_str());

                bool isSubmeshSelected = (g_specificity == SelectionSpecificity::Local &&
                                          g_selectedMesh >= 0 &&
                                          g_selectedMesh < static_cast<int>(curModel->meshes.size()));

                if (isSubmeshSelected) {
                    auto& curMesh = curModel->meshes[g_selectedMesh];
                    std::string meshTitle = curMesh.name.empty() ? ("Submalla " + std::to_string(g_selectedMesh)) : curMesh.name;
                    ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "Submalla Seleccionada: %s (Indice: %d)", meshTitle.c_str(), g_selectedMesh);

                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Transformaciones de Submalla:");
                    ImGui::Text("Posicion Local:");
                    ImGui::DragFloat3("##mesh_pos", &curMesh.position.x, 0.02f);
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Reset##mesh_pos")) curMesh.position = glm::vec3(0.0f);

                    ImGui::Text("Rotacion Local (grados):");
                    ImGui::DragFloat3("##mesh_rot", &curMesh.rotation.x, 1.0f);
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Reset##mesh_rot")) curMesh.rotation = glm::vec3(0.0f);

                    ImGui::Text("Escala Local:");
                    ImGui::DragFloat3("##mesh_scale", &curMesh.scale.x, 0.02f, 0.01f, 50.0f);
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Reset##mesh_scale")) curMesh.scale = glm::vec3(1.0f);

                    if (ImGui::Button("Resetear Submalla a Posicion Original")) {
                        curMesh.position = glm::vec3(0.0f);
                        curMesh.rotation = glm::vec3(0.0f);
                        curMesh.scale = glm::vec3(1.0f);
                    }

                    ImGui::Separator();
                    ImGui::Text("Color Difuso de Submalla (Kd):");
                    ImGui::ColorEdit4("##mesh_kd", &curMesh.color.x);

                    if (ImGui::Button("Aplicar Color a Todo el Objeto")) {
                        curModel->setDiffuseColor(curMesh.color);
                    }

                    ImGui::Separator();
                    ImGui::TextDisabled("Transformaciones Globales (Afectan a todo el modelo):");
                }

                if (!isSubmeshSelected) {
                    ImGui::Text("Transformaciones del Objeto Completo:");
                }

                ImGui::Text("Posicion Global:");
                ImGui::DragFloat3("##pos", &curModel->position.x, 0.02f);
                ImGui::SameLine();
                if (ImGui::SmallButton("Reset##pos")) curModel->position = glm::vec3(0.0f);

                ImGui::Text("Rotacion Global (grados):");
                ImGui::DragFloat3("##rot", &curModel->rotation.x, 1.0f);
                ImGui::SameLine();
                if (ImGui::SmallButton("Reset##rot")) curModel->rotation = glm::vec3(0.0f);

                ImGui::Text("Escala Global:");
                ImGui::DragFloat3("##scale", &curModel->scale.x, 0.02f, 0.01f, 50.0f);
                ImGui::SameLine();
                if (ImGui::SmallButton("Reset##scale")) curModel->scale = glm::vec3(1.0f);

                if (!isSubmeshSelected) {
                    ImGui::Separator();
                    ImGui::Text("Color Difuso Global (Kd):");
                    if (ImGui::ColorEdit4("##kd", &curModel->diffuseColor.x)) {
                        curModel->setDiffuseColor(curModel->diffuseColor);
                    }
                }

                if (ImGui::Button("Cargar Material (.mtl / .mlt)...")) {
                    std::string mtlPath = openMaterialFileDialog();
                    if (!mtlPath.empty()) {
                        curModel->loadMaterialFromFile(mtlPath);
                    }
                }

                if (ImGui::Button("Enfocar Camara Aqui")) {
                    g_camera.target = curModel->position;
                    g_camera.reset(curModel->position, g_camera.distance);
                }

                ImGui::SameLine();
                if (ImGui::Button("Eliminar Objeto")) {
                    models.erase(models.begin() + g_selectedModel);
                    if (g_selectedModel >= static_cast<int>(models.size())) {
                        g_selectedModel = static_cast<int>(models.size()) - 1;
                    }
                    g_selectedMesh = -1;
                }
            } else {
                ImGui::Separator();
                ImGui::TextDisabled("Ningun objeto seleccionado.");
                ImGui::TextDisabled("Haz clic sobre un objeto en la escena 3D para seleccionarlo.");
            }
        }

        ImGui::Separator();

        if (ImGui::CollapsingHeader("Modos de Visualizacion", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Modo Wireframe (Alambrico)", &g_wireframe);

            ImGui::Checkbox("Visualizar Vertices", &g_showVertices);
            if (g_showVertices) {
                ImGui::Indent(20.0f);
                ImGui::SliderFloat("Tamano Vertices", &g_vertexSize, 1.0f, 15.0f, "%.1f px");
                ImGui::Unindent(20.0f);
            }

            ImGui::Checkbox("Visualizar Normales", &g_showNormals);
            if (g_showNormals) {
                ImGui::Indent(20.0f);
                ImGui::SliderFloat("Longitud Normales", &g_normalLength, 0.005f, 0.3f, "%.3f");
                ImGui::ColorEdit3("Color Normales", &g_normalColor.x);
                ImGui::Unindent(20.0f);
            }

            ImGui::Checkbox("Visualizar Bounding Box (AABB)", &g_showBoundingBox);
            if (g_showBoundingBox) {
                ImGui::Indent(20.0f);
                if (g_specificity == SelectionSpecificity::Local && g_selectedMesh >= 0) {
                    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.0f, 1.0f), "Caja Amarilla: Submalla activa");
                    ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Caja Verde: Objeto completo");
                } else {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.2f, 1.0f), "Caja Verde: Objeto completo");
                }
                ImGui::Unindent(20.0f);
            }
        }

        ImGui::Separator();

        if (ImGui::CollapsingHeader("Camara y Vista", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Distancia: %.2f", g_camera.distance);
            ImGui::Text("Yaw: %.1f deg, Pitch: %.1f deg", g_camera.yaw, g_camera.pitch);

            if (ImGui::Button("Centrar Vista (Reset)", ImVec2(160, 26))) {
                g_camera.reset(glm::vec3(0.0f, 0.0f, 0.0f), 3.5f);
            }
        }

        if (ImGui::CollapsingHeader("Opciones de Render")) {
            ImGui::Checkbox("Depth Test (GL_DEPTH_TEST)", &g_depthTest);
            ImGui::Checkbox("Back-Face Culling (GL_CULL_FACE)", &g_cullFace);
            ImGui::Separator();
            ImGui::ColorEdit3("Color de fondo", &clearColor.x);
            ImGui::SliderFloat3("Dir. Luz", &lightDir.x, -1.0f, 1.0f);
            ImGui::ColorEdit3("Color Luz", &lightColor.x);
            ImGui::ColorEdit3("Luz Ambiente", &ambientLight.x);
        }

        if (ImGui::CollapsingHeader("Ayuda de Controles (Mouse y Teclado)")) {
            ImGui::BulletText("Click Izquierdo: Seleccionar objeto o submalla (Color Picking)");
            ImGui::BulletText("Click Izquierdo + Arrastrar: Manipular objeto segun modo activo");
            ImGui::BulletText("Click Derecho + Arrastrar: Orbitar camara en 360 grados");
            ImGui::BulletText("Shift + Click Derecho: Desplazar vista (Pan)");
            ImGui::BulletText("Rueda del Mouse: Acercar / Alejar camara (Zoom)");
            ImGui::BulletText("Teclas W, A, S, D: Mover camara libremente");
            ImGui::BulletText("Espacio / Shift: Subir / Bajar camara");
            ImGui::BulletText("Ctrl + Arrastrar en Rotacion: Rotar en eje Z");
        }

            ImGui::End();
        } else {
            ImGui::SetNextWindowPos(ImVec2(static_cast<float>(display_w) - 105.0f, 12.0f));
            ImGuiWindowFlags hudFlags = ImGuiWindowFlags_NoTitleBar
                                      | ImGuiWindowFlags_NoResize
                                      | ImGuiWindowFlags_NoMove
                                      | ImGuiWindowFlags_NoScrollbar
                                      | ImGuiWindowFlags_AlwaysAutoResize
                                      | ImGuiWindowFlags_NoSavedSettings;
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.10f, 0.10f, 0.14f, 0.75f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 4.0f));
            ImGui::Begin("##MiniHUD", nullptr, hudFlags);
            float fps = io.Framerate;
            ImVec4 fpsColor = (fps >= 30.0f) ? ImVec4(0.2f, 1.0f, 0.3f, 1.0f) :
                              (fps >= 15.0f) ? ImVec4(1.0f, 0.85f, 0.2f, 1.0f) :
                                               ImVec4(1.0f, 0.3f, 0.2f, 1.0f);
            ImGui::TextColored(fpsColor, "%.0f FPS", fps);
            ImGui::End();
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor();
        }

        // Render 3D
        ImGui::Render();

        int sidebarPixels = static_cast<int>(getEffectiveSidebarWidth());
        int sceneWidth = display_w - sidebarPixels;
        if (sceneWidth < 1) sceneWidth = 1;

        glViewport(0, 0, display_w, display_h);
        glClearColor(clearColor.x, clearColor.y, clearColor.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glViewport(sidebarPixels, 0, sceneWidth, display_h);

        if (g_depthTest) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }

        if (g_cullFace) {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
        } else {
            glDisable(GL_CULL_FACE);
        }

        if (g_wireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        baseShader.use();
        baseShader.setMat4("view", view);
        baseShader.setMat4("projection", projection);
        baseShader.setVec3("lightDir", glm::normalize(lightDir));
        baseShader.setVec3("lightColor", lightColor);
        baseShader.setVec3("ambientLight", ambientLight);

        for (const auto& model : models) {
            model->draw(baseShader);
        }

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // Vértices
        if (g_showVertices && !models.empty()) {
            flatShader.use();
            flatShader.setMat4("view", view);
            flatShader.setMat4("projection", projection);
            flatShader.setVec4("flatColor", glm::vec4(1.0f, 0.85f, 0.2f, 1.0f));

            glPointSize(g_vertexSize);
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);

            for (const auto& model : models) {
                glm::mat4 modelMat = model->getModelMatrix();
                for (const auto& mesh : model->meshes) {
                    flatShader.setMat4("model", modelMat * mesh.getLocalModelMatrix());
                    mesh.draw();
                }
            }

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        // Normales
        if (g_showNormals && !models.empty()) {
            normalsShader.use();
            normalsShader.setMat4("view", view);
            normalsShader.setMat4("projection", projection);
            normalsShader.setFloat("normalLength", g_normalLength);
            normalsShader.setVec4("normalColor", g_normalColor);

            for (const auto& model : models) {
                glm::mat4 modelMat = model->getModelMatrix();
                for (const auto& mesh : model->meshes) {
                    normalsShader.setMat4("model", modelMat * mesh.getLocalModelMatrix());
                    mesh.draw();
                }
            }
        }

        // Bounding box
        if (g_showBoundingBox && !models.empty() && g_selectedModel >= 0 && g_selectedModel < static_cast<int>(models.size())) {
            flatShader.use();
            flatShader.setMat4("view", view);
            flatShader.setMat4("projection", projection);

            auto& selModel = models[g_selectedModel];
            glm::mat4 modelMat = selModel->getModelMatrix();

            bool isLocalSubmesh = (g_specificity == SelectionSpecificity::Local &&
                                   g_selectedMesh >= 0 &&
                                   g_selectedMesh < static_cast<int>(selModel->meshes.size()));

            if (isLocalSubmesh) {
                auto& curMesh = selModel->meshes[g_selectedMesh];
                glm::mat4 meshMat = modelMat * curMesh.getLocalModelMatrix();
                g_boundingBox.draw(flatShader, meshMat, curMesh.minBounds, curMesh.maxBounds, glm::vec4(1.0f, 0.9f, 0.0f, 1.0f));
                g_boundingBox.draw(flatShader, modelMat, selModel->minBounds, selModel->maxBounds, glm::vec4(0.2f, 0.7f, 0.2f, 0.4f));
            } else {
                g_boundingBox.draw(flatShader, modelMat, selModel->minBounds, selModel->maxBounds, glm::vec4(0.0f, 1.0f, 0.2f, 1.0f));
            }
        }

        glViewport(0, 0, display_w, display_h);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    models.clear();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
