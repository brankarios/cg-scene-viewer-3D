#include "Camera.h"
#include <algorithm>
#include <cmath>

Camera::Camera(glm::vec3 startTarget, float startDistance)
    : target(startTarget),
      worldUp(0.0f, 1.0f, 0.0f),
      yaw(0.0f),
      pitch(15.0f),
      distance(startDistance),
      movementSpeed(3.0f),
      mouseSensitivity(0.25f),
      zoom(45.0f) {
    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position, target, up);
}

void Camera::updateCameraVectors() {
    // Calcular la posición esférica relativa al objetivo (modo órbita)
    float pitchRad = glm::radians(pitch);
    float yawRad = glm::radians(yaw);

    glm::vec3 offset;
    offset.x = distance * std::cos(pitchRad) * std::sin(yawRad);
    offset.y = distance * std::sin(pitchRad);
    offset.z = distance * std::cos(pitchRad) * std::cos(yawRad);

    position = target + offset;

    // Vectores direccionales
    front = glm::normalize(target - position);
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}

void Camera::processKeyboard(CameraMovement direction, float deltaTime) {
    float velocity = movementSpeed * deltaTime;
    glm::vec3 flatFront = glm::normalize(glm::vec3(front.x, 0.0f, front.z));
    if (glm::length(flatFront) < 0.001f) {
        flatFront = front;
    }

    if (direction == CameraMovement::FORWARD) {
        target += flatFront * velocity;
    }
    if (direction == CameraMovement::BACKWARD) {
        target -= flatFront * velocity;
    }
    if (direction == CameraMovement::LEFT) {
        target -= right * velocity;
    }
    if (direction == CameraMovement::RIGHT) {
        target += right * velocity;
    }
    if (direction == CameraMovement::UP) {
        target += worldUp * velocity;
    }
    if (direction == CameraMovement::DOWN) {
        target -= worldUp * velocity;
    }

    updateCameraVectors();
}

void Camera::processMouseOrbit(float xoffset, float yoffset) {
    yaw += xoffset * mouseSensitivity;
    pitch += yoffset * mouseSensitivity;

    // Limitar pitch para evitar inversión de cámara
    pitch = std::clamp(pitch, -89.0f, 89.0f);

    updateCameraVectors();
}

void Camera::processMousePan(float xoffset, float yoffset) {
    float panSpeed = 0.002f * distance;
    target -= right * (xoffset * panSpeed);
    target += up * (yoffset * panSpeed);

    updateCameraVectors();
}

void Camera::processMouseScroll(float yoffset) {
    distance -= yoffset * (0.15f * distance);
    distance = std::clamp(distance, 0.2f, 50.0f);

    updateCameraVectors();
}

void Camera::reset(glm::vec3 newTarget, float newDistance) {
    target = newTarget;
    distance = newDistance;
    yaw = 0.0f;
    pitch = 15.0f;
    updateCameraVectors();
}
