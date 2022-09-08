#version 330 core

in vec3 FragPos;
in vec2 TexCoords;

out vec4 FragColor;

uniform vec3 rayOrigin;

uniform vec4 cloudBaseColor;
uniform float sizeAmountRatio;
uniform float densityMultiplier;
uniform vec3 rayColor1;
uniform vec3 rayColor2;
uniform vec3 perlinSeed1;
uniform vec3 perlinSeed2;
uniform vec3 perlinSeed3;

vec3 random_vector(vec3 input_vector)
{
    float input_dot_1 = dot(input_vector, perlinSeed1);
    float input_dot_2 = dot(input_vector, perlinSeed2);
    float input_dot_3 = dot(input_vector, perlinSeed3);

    return (2.0 * sin(vec3(input_dot_1, input_dot_2, input_dot_3) * 43227.59)) - 1.0;
}

float perlin_noise(vec3 input_vector)
{
    vec3 input_floor = floor(input_vector);
    vec3 input_fract = fract(input_vector);

    vec3 vertex_1 = vec3(0.0, 0.0, 0.0);
    vec3 vertex_2 = vec3(1.0, 0.0, 0.0);
    vec3 vertex_3 = vec3(1.0, 0.0, 1.0);
    vec3 vertex_4 = vec3(0.0, 0.0, 1.0);
    vec3 vertex_5 = vec3(0.0, 1.0, 0.0);
    vec3 vertex_6 = vec3(1.0, 1.0, 0.0);
    vec3 vertex_7 = vec3(1.0, 1.0, 1.0);
    vec3 vertex_8 = vec3(0.0, 1.0, 1.0);

    float dot_1 = dot(random_vector(input_floor + vertex_1), input_fract - vertex_1);
    float dot_2 = dot(random_vector(input_floor + vertex_2), input_fract - vertex_2);
    float dot_3 = dot(random_vector(input_floor + vertex_3), input_fract - vertex_3);
    float dot_4 = dot(random_vector(input_floor + vertex_4), input_fract - vertex_4);
    float dot_5 = dot(random_vector(input_floor + vertex_5), input_fract - vertex_5);
    float dot_6 = dot(random_vector(input_floor + vertex_6), input_fract - vertex_6);
    float dot_7 = dot(random_vector(input_floor + vertex_7), input_fract - vertex_7);
    float dot_8 = dot(random_vector(input_floor + vertex_8), input_fract - vertex_8);

    vec3 input_fract_smooth = input_fract * input_fract * (3.0 - (2.0 * input_fract));

    float mix_1 = mix(dot_1, dot_2, input_fract_smooth.x);
    float mix_2 = mix(dot_5, dot_6, input_fract_smooth.x);
    float mix_3 = mix(dot_4, dot_3, input_fract_smooth.x);
    float mix_4 = mix(dot_8, dot_7, input_fract_smooth.x);

    float mix_5 = mix(mix_1, mix_2, input_fract_smooth.y);
    float mix_6 = mix(mix_3, mix_4, input_fract_smooth.y);

    return mix(mix_5, mix_6, input_fract_smooth.z);
}

float fractal_noise(vec3 input_vector)
{
    float fractal_noise = 0.5 * perlin_noise(input_vector);
    fractal_noise += 0.25 * perlin_noise(input_vector * 2.0);
    fractal_noise += 0.125 * perlin_noise(input_vector * 4.0);

    return clamp(fractal_noise, 0.0, 1.0);
}

vec4 ray_march(int steps)
{
    vec4 color = cloudBaseColor;

    vec3 direction = normalize(FragPos - rayOrigin);

    vec3 current_position = FragPos / sizeAmountRatio;

    for (int step = 0; step < steps; ++step)
    {
        float density = fractal_noise(current_position);

        if (density > 0.01)
        {
            vec4 ray_color = vec4(mix(rayColor1, rayColor2, density), density * densityMultiplier);
            ray_color.xyz *= ray_color.w;

            color += ray_color * (1.0 - color.w);
        }

        if (color.w > 0.95) break;

        current_position += direction * 0.1;
    }

    return color;
}

void main()
{
    FragColor = ray_march(1);
}

