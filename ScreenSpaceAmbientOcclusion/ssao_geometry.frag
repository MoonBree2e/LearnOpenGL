#version 460 core
layout (location = 0) out vec4 gPositionDepth;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

const float NEAR = 0.1; // Projection matrix's near plane distance
const float FAR = 50.0f; // Projection matrix's far plane distance

float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Back to NDC 
    return (2.0 * NEAR * FAR) / (FAR + NEAR - z * (FAR - NEAR));	
}

void main()
{
    gPositionDepth.xyz = FragPos;
    gPositionDepth.a = LinearizeDepth(gl_FragCoord.z);

    gNormal = normalize(Normal);
    gAlbedoSpec.rgb = vec3(0.95);
}
