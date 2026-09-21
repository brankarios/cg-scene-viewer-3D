#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum class CameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

class Camera {
public:
    // Posición y orientación
    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    // Ángulos de Euler (en grados) y distancia orbital
    float yaw;
    float pitch;
    float distance;

    // Configuración de movimiento
    float movementSpeed;
    float mouseSensitivity;
    float zoom; // Campo de visión (FOV en grados)

    Camera(glm::vec3 startTarget = glm::vec3(0.0f, 0.0f, 0.0f), float startDistance = 3.5f);

    // Retorna la matriz de vista (View Matrix) calculada con LookAt
    glm::mat4 getViewMatrix() const;

    // Movimiento libre con teclado (WASD, Espacio, Shift)
    void processKeyboard(CameraMovement direction, float deltaTime);

    // Orbitar la cámara alrededor del objetivo usando el mouse
    void processMouseOrbit(float xoffset, float yoffset);

    // Desplazamiento panorámico (Pan) moviendo el objetivo y la cámara en el plano de vista
    void processMousePan(float xoffset, float yoffset);

    // Zoom fluido modificando la distancia o el FOV con la rueda del ratón
    void processMouseScroll(float yoffset);

    // Resetear la cámara a los valores por defecto enfocando un objetivo
    void reset(glm::vec3 newTarget = glm::vec3(0.0f, 0.0f, 0.0f), float newDistance = 3.5f);

    // Recalcular vectores y posición de la cámara
    void updateCameraVectors();
};
