#version 330 core

out vec4 FragColor;

uniform vec4 pickingColor;

void main() {
    FragColor = pickingColor;
}
