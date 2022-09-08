#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 rayOrigin;

const float visibility = 1.0;
const float cloud_base = 1.0;
const float cloud_height = 100.0;

void main()
{
    //vec3 vertex_position = vec3(rayOrigin.x + (visibility * aPos.x), cloud_base + (cloud_height * aPos.y), rayOrigin.z + (visibility * aPos.z));

    FragPos = vec3(model * vec4(aPos, 1.0));
    TexCoords = aTexCoords;

    gl_Position = projection * view * vec4(FragPos, 1.0);
}
