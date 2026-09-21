#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

class PickingFBO {
public:
    unsigned int fbo = 0;
    unsigned int texture = 0;
    unsigned int depthRBO = 0;
    int width = 0;
    int height = 0;

    PickingFBO() = default;
    ~PickingFBO();

    // No copiar, si mover
    PickingFBO(const PickingFBO&) = delete;
    PickingFBO& operator=(const PickingFBO&) = delete;
    PickingFBO(PickingFBO&& other) noexcept;
    PickingFBO& operator=(PickingFBO&& other) noexcept;

    // Inicializar o redimensionar el framebuffer
    bool init(int w, int h);
    void resize(int w, int h);

    // Activar y desactivar el framebuffer para dibujar
    void bind();
    void unbind();

    // Leer el ID numérico del píxel bajo las coordenadas (x, y) de la ventana
    int readPixel(int x, int y);

    // Funciones utilitarias para convertir entre ID entero y Color vec4
    static glm::vec4 idToColor(int id);
    static int colorToId(unsigned char r, unsigned char g, unsigned char b);

private:
    void cleanup();
};
