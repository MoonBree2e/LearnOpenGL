#version 460 core
out float FragColor;
in vec2 TexCoords;

uniform sampler2D gPositionDepth;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

uniform vec3 samples[64];

int kernelSize = 64;
float radius = 1.0;

const vec2 noiseScale = vec2(800.0f/4.0f, 600.0f/4.0f); 

uniform mat4 projection;

void main()
{
    vec3 fragPos = texture(gPositionDepth, TexCoords).xyz;
    vec3 normal = texture(gNormal, TexCoords).rgb;
    vec3 randomVec = texture(texNoise, TexCoords * noiseScale).xyz;
}