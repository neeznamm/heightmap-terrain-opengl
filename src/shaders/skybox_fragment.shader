#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

uniform float skyboxIntensity;

void main()
{    
    FragColor = skyboxIntensity*texture(skybox, TexCoords);
}