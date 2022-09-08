

#version 410 core

in float Height;
in vec2 texCoord;
in vec3 FragPos;

out vec4 FragColor;

uniform sampler2D heightMap;
uniform sampler2D normalMap;
uniform sampler2D textureBlendMap;
uniform sampler2D specularMap;
uniform sampler2D texture3;
uniform sampler2D texture4;
uniform sampler2D texture5;
uniform sampler2D texture6;

uniform mat4 model;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

uniform float ambientStrength;
uniform float diffuseStrength;
uniform float specularStrength;
uniform float shininess;

void main()
{
    float h = (Height + 16)/64.0f;
    vec2 texCoordScaled = 64.0 * texCoord;

    vec4 blendMapColour = texture(textureBlendMap, texCoord);

    float waterTexAmount = 1 - (blendMapColour.r + blendMapColour.g + blendMapColour.b);
    vec4 waterTexColour = texture(texture5, texCoordScaled) * waterTexAmount;

    vec4 rTexColour = texture(texture3, texCoordScaled) * blendMapColour.r;
    vec4 gTexColour = texture(texture4, texCoordScaled) * blendMapColour.g;
    vec4 bTexColour = texture(texture6, texCoordScaled) * blendMapColour.b;

    vec4 totalTexColour = waterTexColour + rTexColour + gTexColour + bTexColour;

    vec3 normal = texture(normalMap, texCoord).rgb;
    normal = normalize(normal * 2.0 - 1.0);  

    // ambient lighting
    vec3 ambient =  lightColor * vec3(totalTexColour);

    //difuse lighting
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor * vec3(totalTexColour);

    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = spec * texture(specularMap, texCoord).rgb;  

    FragColor = vec4(ambientStrength*ambient + diffuseStrength*diffuse + specularStrength*specular, 1.0);
}

