

#version 330 core
out vec4 FragColor;

in vec3 FragPos;  
in vec3 Normal;  
in vec2 TexCoords;

uniform mat4 model;
uniform sampler2D modelDiffuse;
uniform sampler2D modelSpecular;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

void main()
{    
    vec3 norm = normalize(Normal);

    // ambient lighting
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor * texture(modelDiffuse, TexCoords).rgb;

    //difuse lighting
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor * texture(modelDiffuse, TexCoords).rgb;

    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 0.7);
    vec3 specular = spec * texture(modelSpecular, TexCoords).rgb;  

    FragColor = vec4(ambient + diffuse, 1.0);
}

