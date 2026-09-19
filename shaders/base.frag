//Cambien la Version si es necesario
#version 330 core

//Entradas
in vec3 FragPos;
in vec3 Normal;

//Salida
out vec4 FragColor;

//Propiedades del objeto y la luz
uniform vec4 objectColor;  //Vector 4 para soportar el color difuso (kd) y el canal alfa
uniform vec3 lightDir;     //Dir de la luz
uniform vec3 lightColor;   //Color de la luz
uniform vec3 ambientLight; //Luz ambiental base para que las sombras no sean totalmente negras

void main() {
    //Iluminacion Ambiental
    vec3 ambient = ambientLight * objectColor.rgb;
    
    //Iluminacion Difusa (Modelo Lambert)
    vec3 norm = normalize(Normal);
    vec3 lightDirNorm = normalize(-lightDir);
    float diff = max(dot(norm, lightDirNorm), 0.0);
    vec3 diffuse = diff * lightColor * objectColor.rgb;
    
    //Resultado
    vec3 result = ambient + diffuse;
    
    //Se asigna el color resultante y se preserva el canal alfa original
    FragColor = vec4(result, objectColor.a);
}