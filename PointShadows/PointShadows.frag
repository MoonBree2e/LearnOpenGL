#version 430 core
out vec4 FragColor;

in VS_OUT{
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

uniform sampler2D diffuseTexture;
uniform samplerCube depthMap;

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform float far_plane;

float shadowCalculation(vec3 fragPos)
{
    vec3 fragToLight = fragPos - lightPos;

    float closestDepth = texture(depthMap, fragToLight).r;

    closestDepth *= far_plane;
    float currentDepth = length(fragToLight);

    float bias = 0.05; 
    float shadow = 0.f;
    shadow = currentDepth -  bias > closestDepth ? 1.0 : 0.0;

    return shadow;
}

void main()
{
    vec3 color = texture(diffuseTexture, fs_in.TexCoords).rgb;
    vec3 normal = normalize(fs_in.Normal);
    vec3 lightColor = vec3(1.f);

    vec3 ambient = 0.15 * color;

    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    float diffCoef = max(dot(lightDir, normal), 0.f);
    vec3 diffuse = diffCoef * lightColor;

    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.f;
    vec3 halfwayDir = normalize(lightDir + viewDir);
    spec = pow(max(dot(normal, halfwayDir), 0.f), 64.f);
    vec3 specular = spec * lightColor;
    float shadow = shadowCalculation(fs_in.FragPos);
    vec3 lighting = (ambient + (1.0-shadow) * (diffuse + specular)) * color;

    FragColor = vec4(lighting, 1.f);
}