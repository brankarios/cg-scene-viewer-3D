//Cambien la Version si es necesario
#version 330 core

//Atributos de entrada
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

//Salidas
out vec3 FragPos;
out vec3 Normal;

//Matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}