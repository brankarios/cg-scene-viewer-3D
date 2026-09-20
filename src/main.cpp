// ── Windows file dialog ──
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

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>

enum class InteractionMode {
    Rotate,     
    Translate,  
    Scale,      
    Navigate    
};

// Variables globales para interacción y callbacks 
static Camera g_camera(glm::vec3(0.0f, 0.0f, 0.0f), 3.5f);
static InteractionMode g_mode = InteractionMode::Rotate;
static int g_selectedModel = 0;

static bool g_isLeftDragging = false;
static bool g_isRightDragging = false;
static bool g_isMiddleDragging = false;
static double g_lastMouseX = 0.0;
static double g_lastMouseY = 0.0;

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

// Callback para redimensionar la ventana
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Callback para la rueda del ratón (Zoom)
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    g_camera.processMouseScroll(static_cast<float>(yoffset));
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
    glfwSwapInterval(1); // VSync activado
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // ── Cargar funciones de OpenGL con GLAD ──
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Error: No se pudo inicializar GLAD" << std::endl;
        return -1;
    }

    // Imprimir información del sistema
    std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GPU:    " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "GLSL:   " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    // ── Compilar shader base ──
    Shader baseShader;
    if (!baseShader.load("shaders/base.vert", "shaders/base.frag")) {
        std::cerr << "Error: No se pudieron cargar los shaders" << std::endl;
        return -1;
    }

    // ── Configurar Dear ImGui ──
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Tema oscuro estilizado
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.GrabRounding = 4.0f;

    // Inicializar backends de ImGui
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // ── Estado de la escena ──
    std::vector<std::unique_ptr<Model>> models;
    glm::vec3 clearColor(0.12f, 0.12f, 0.14f);

    // Iluminación (uniforms del fragment shader)
    glm::vec3 lightDir(-0.2f, -1.0f, -0.3f);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
    glm::vec3 ambientLight(0.2f, 0.2f, 0.2f);

    // ── Habilitar Depth Test ──
    glEnable(GL_DEPTH_TEST);

    // Control de tiempo para movimiento uniforme
    float lastFrameTime = static_cast<float>(glfwGetTime());

    // ── Bucle principal ──
    while (!glfwWindowShouldClose(window)) {
        float currentFrameTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        glfwPollEvents();

        // ── Entrada de teclado para la cámara (WASD + Espacio / Shift) ──
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

            // Atajos de modo con teclas
            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) g_mode = InteractionMode::Rotate;
            if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) g_mode = InteractionMode::Translate;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) != GLFW_PRESS)
                g_mode = InteractionMode::Scale;
            if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) g_mode = InteractionMode::Navigate;
        }

        // ── Manejo del Mouse en el Viewport ──
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        float mouseDeltaX = static_cast<float>(mouseX - g_lastMouseX);
        float mouseDeltaY = static_cast<float>(mouseY - g_lastMouseY);
        g_lastMouseX = mouseX;
        g_lastMouseY = mouseY;

        bool leftDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        bool rightDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
        bool middleDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;

        // Comenzar arrastre solo si no estamos interactuando con ImGui
        if (!io.WantCaptureMouse) {
            if (leftDown && !g_isLeftDragging) g_isLeftDragging = true;
            if (rightDown && !g_isRightDragging) g_isRightDragging = true;
            if (middleDown && !g_isMiddleDragging) g_isMiddleDragging = true;
        }
        if (!leftDown) g_isLeftDragging = false;
        if (!rightDown) g_isRightDragging = false;
        if (!middleDown) g_isMiddleDragging = false;

        // 1. Click derecho arrastrando: Orbitar o Panear Cámara
        if (g_isRightDragging) {
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                g_camera.processMousePan(mouseDeltaX, mouseDeltaY);
            } else {
                g_camera.processMouseOrbit(mouseDeltaX, mouseDeltaY);
            }
        }

        // 2. Click central arrastrando: Panear Cámara
        if (g_isMiddleDragging) {
            g_camera.processMousePan(mouseDeltaX, mouseDeltaY);
        }

        // 3. Click izquierdo arrastrando: Manipular Modelo o Navegar
        if (g_isLeftDragging) {
            bool hasModel = !models.empty() && g_selectedModel >= 0 && g_selectedModel < static_cast<int>(models.size());

            if (g_mode == InteractionMode::Navigate || !hasModel) {
                // Orbitar cámara con click izquierdo en modo navegar
                g_camera.processMouseOrbit(mouseDeltaX, mouseDeltaY);
            } else if (hasModel) {
                auto& model = models[g_selectedModel];

                if (g_mode == InteractionMode::Rotate) {
                    // Rotación intuitiva sobre el eje propio
                    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
                        model->rotation.z += mouseDeltaX * 0.5f;
                    } else {
                        model->rotation.y += mouseDeltaX * 0.5f;
                        model->rotation.x += mouseDeltaY * 0.5f;
                    }
                } else if (g_mode == InteractionMode::Translate) {
                    // Mover en el plano de la cámara
                    float speed = 0.0025f * g_camera.distance;
                    model->position += g_camera.right * (mouseDeltaX * speed) - g_camera.up * (mouseDeltaY * speed);
                } else if (g_mode == InteractionMode::Scale) {
                    // Escalar con el movimiento del ratón
                    float factor = 1.0f + (mouseDeltaX - mouseDeltaY) * 0.008f;
                    model->scale *= factor;
                    model->scale = glm::max(model->scale, glm::vec3(0.005f));
                }
            }
        }

        // ── Iniciar frame de ImGui ──
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ── Panel de control principal ──
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(360, 680), ImGuiCond_FirstUseEver);
        ImGui::Begin("Panel de Control");

        // Info del sistema y estadísticas
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "CG Scene Viewer 3D");
        ImGui::Text("FPS: %.1f (%.2f ms)", io.Framerate, 1000.0f / io.Framerate);
        ImGui::Separator();

        // Cargar modelo
        if (ImGui::Button("Cargar OBJ...", ImVec2(140, 30))) {
            std::string path = openFileDialog();
            if (!path.empty()) {
                auto model = std::make_unique<Model>();
                if (model->loadFromFile(path)) {
                    models.push_back(std::move(model));
                    g_selectedModel = static_cast<int>(models.size()) - 1;
                } else {
                    std::cerr << "Error al cargar: " << path << std::endl;
                }
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Limpiar Todo", ImVec2(120, 30))) {
            models.clear();
            g_selectedModel = 0;
        }

        ImGui::Text("Modelos en escena: %d", static_cast<int>(models.size()));
        ImGui::Separator();

        // ── Barra de Herramientas de Interacción con el Mouse ──
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

        // ── Lista y Transformaciones de Modelos ──
        if (models.empty()) {
            ImGui::TextDisabled("No hay modelos en la escena.");
            ImGui::TextDisabled("Haz clic en 'Cargar OBJ...' para comenzar.");
        } else {
            ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1.0f), "Objetos en Escena:");

            for (size_t i = 0; i < models.size(); i++) {
                auto& model = models[i];
                bool isSelected = (static_cast<int>(i) == g_selectedModel);

                std::string header = (isSelected ? "> " : "  ") + model->name + " (" + std::to_string(model->meshes.size()) + " mallas)";
                if (ImGui::Selectable(header.c_str(), isSelected)) {
                    g_selectedModel = static_cast<int>(i);
                }
            }

            if (g_selectedModel >= 0 && g_selectedModel < static_cast<int>(models.size())) {
                auto& curModel = models[g_selectedModel];
                ImGui::Separator();
                ImGui::Text("Editando: %s", curModel->name.c_str());

                // Transformaciones con sliders y botones rápidos de reset
                ImGui::Text("Posicion:");
                ImGui::DragFloat3("##pos", &curModel->position.x, 0.02f);
                ImGui::SameLine();
                if (ImGui::SmallButton("Reset##pos")) curModel->position = glm::vec3(0.0f);

                ImGui::Text("Rotacion (grados):");
                ImGui::DragFloat3("##rot", &curModel->rotation.x, 1.0f);
                ImGui::SameLine();
                if (ImGui::SmallButton("Reset##rot")) curModel->rotation = glm::vec3(0.0f);

                ImGui::Text("Escala:");
                ImGui::DragFloat3("##scale", &curModel->scale.x, 0.02f, 0.01f, 50.0f);
                ImGui::SameLine();
                if (ImGui::SmallButton("Reset##scale")) curModel->scale = glm::vec3(1.0f);

                // Material y Color Difuso (Kd)
                ImGui::Text("Color Difuso (Kd):");
                if (ImGui::ColorEdit4("##kd", &curModel->diffuseColor.x)) {
                    curModel->setDiffuseColor(curModel->diffuseColor);
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
                }
            }
        }

        ImGui::Separator();

        // ── Ajustes de Cámara ──
        if (ImGui::CollapsingHeader("Camara y Vista", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Distancia: %.2f", g_camera.distance);
            ImGui::Text("Yaw: %.1f deg, Pitch: %.1f deg", g_camera.yaw, g_camera.pitch);

            if (ImGui::Button("Centrar Vista (Reset)", ImVec2(160, 26))) {
                g_camera.reset(glm::vec3(0.0f, 0.0f, 0.0f), 3.5f);
            }
        }

        // ── Ajustes de Render ──
        if (ImGui::CollapsingHeader("Entorno y Luces")) {
            ImGui::ColorEdit3("Color de fondo", &clearColor.x);
            ImGui::SliderFloat3("Dir. Luz", &lightDir.x, -1.0f, 1.0f);
            ImGui::ColorEdit3("Color Luz", &lightColor.x);
            ImGui::ColorEdit3("Luz Ambiente", &ambientLight.x);
        }

        // ── Guía de Controles Rápidos ──
        if (ImGui::CollapsingHeader("Ayuda de Controles (Mouse y Teclado)")) {
            ImGui::BulletText("Click Izquierdo + Arrastrar: Manipular objeto según modo activo (Rotar/Mover/Escalar)");
            ImGui::BulletText("Click Derecho + Arrastrar: Orbitar cámara en 360°");
            ImGui::BulletText("Shift + Click Derecho: Desplazar vista (Pan)");
            ImGui::BulletText("Rueda del Mouse: Acercar / Alejar cámara (Zoom)");
            ImGui::BulletText("Teclas W, A, S, D: Mover cámara libremente");
            ImGui::BulletText("Espacio / Shift: Subir / Bajar cámara");
            ImGui::BulletText("Ctrl + Arrastrar en Rotación: Rotar en eje Z");
        }

        ImGui::End();

        // ── Renderizado ──
        ImGui::Render();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);

        // Limpiar pantalla
        glClearColor(clearColor.x, clearColor.y, clearColor.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Matrices de Vista y Proyección con la Cámara interactiva
        float aspect = (display_h > 0) ? static_cast<float>(display_w) / static_cast<float>(display_h) : 1.0f;
        glm::mat4 view = g_camera.getViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(g_camera.zoom), aspect, 0.1f, 100.0f);

        // Activar shader y enviar uniforms
        baseShader.use();
        baseShader.setMat4("view", view);
        baseShader.setMat4("projection", projection);
        baseShader.setVec3("lightDir", glm::normalize(lightDir));
        baseShader.setVec3("lightColor", lightColor);
        baseShader.setVec3("ambientLight", ambientLight);

        // Dibujar todos los modelos
        for (const auto& model : models) {
            model->draw(baseShader);
        }

        // Dibujar interfaz de ImGui encima de la escena 3D
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // ── Limpieza ──
    models.clear(); // Liberar modelos antes de destruir el contexto OpenGL
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
