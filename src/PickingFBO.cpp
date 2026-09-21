#include "PickingFBO.h"
#include <iostream>
#include <algorithm>

PickingFBO::~PickingFBO() {
    cleanup();
}

PickingFBO::PickingFBO(PickingFBO&& other) noexcept
    : fbo(other.fbo)
    , texture(other.texture)
    , depthRBO(other.depthRBO)
    , width(other.width)
    , height(other.height) {
    other.fbo = 0;
    other.texture = 0;
    other.depthRBO = 0;
    other.width = 0;
    other.height = 0;
}

PickingFBO& PickingFBO::operator=(PickingFBO&& other) noexcept {
    if (this != &other) {
        cleanup();
        fbo = other.fbo;
        texture = other.texture;
        depthRBO = other.depthRBO;
        width = other.width;
        height = other.height;
        other.fbo = 0;
        other.texture = 0;
        other.depthRBO = 0;
        other.width = 0;
        other.height = 0;
    }
    return *this;
}

void PickingFBO::cleanup() {
    if (depthRBO) {
        glDeleteRenderbuffers(1, &depthRBO);
        depthRBO = 0;
    }
    if (texture) {
        glDeleteTextures(1, &texture);
        texture = 0;
    }
    if (fbo) {
        glDeleteFramebuffers(1, &fbo);
        fbo = 0;
    }
    width = 0;
    height = 0;
}

bool PickingFBO::init(int w, int h) {
    if (w <= 0 || h <= 0) return false;
    cleanup();

    width = w;
    height = h;

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Textura RGBA para almacenar los IDs codificados en color
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    // Renderbuffer para profundidad (Depth Buffer)
    glGenRenderbuffers(1, &depthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRBO);

    // Comprobar que el Framebuffer está completo y válido
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Error: Picking Framebuffer no está completo. Código: " << status << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

void PickingFBO::resize(int w, int h) {
    if (w != width || h != height) {
        init(w, h);
    }
}

void PickingFBO::bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, width, height);
}

void PickingFBO::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int PickingFBO::readPixel(int x, int y) {
    if (fbo == 0 || width <= 0 || height <= 0) return 0;

    // En OpenGL el origen (0,0) está en la esquina inferior izquierda
    int flippedY = height - y - 1;

    x = std::clamp(x, 0, width - 1);
    flippedY = std::clamp(flippedY, 0, height - 1);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    unsigned char pixel[4] = {0, 0, 0, 0};
    glReadPixels(x, flippedY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    return colorToId(pixel[0], pixel[1], pixel[2]);
}

glm::vec4 PickingFBO::idToColor(int id) {
    float r = ((id & 0x000000FF) >> 0) / 255.0f;
    float g = ((id & 0x0000FF00) >> 8) / 255.0f;
    float b = ((id & 0x00FF0000) >> 16) / 255.0f;
    return glm::vec4(r, g, b, 1.0f);
}

int PickingFBO::colorToId(unsigned char r, unsigned char g, unsigned char b) {
    return static_cast<int>(r) + (static_cast<int>(g) << 8) + (static_cast<int>(b) << 16);
}
