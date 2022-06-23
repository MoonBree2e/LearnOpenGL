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
uniform bool shadows;
uniform bool debug;


float closestDepth;

// 一个偏移量方向数组，它们差不多都是分开的，每一个指向完全不同的方向，剔除彼此接近的那些子方向
// 减少采样次数（4 * 4 * 4 = 64)
vec3 sampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

float ShadowCalculation(vec3 fragPos)
{
    vec3 fragToLight = fragPos - lightPos;

    closestDepth = texture(depthMap, fragToLight).r;

    closestDepth *= far_plane;
    float currentDepth = length(fragToLight);

    float shadow = 0.0;
//    float bias = 0.05; 
//    float samples = 4.0;
//    float offset = 0.1;
//    for(float x = -offset; x < offset; x += offset / (samples * 0.5))
//    {
//        for(float y = -offset; y < offset; y += offset / (samples * 0.5))
//        {
//            for(float z = -offset; z < offset; z += offset / (samples * 0.5))
//            {
//                float closestDepth = texture(depthMap, fragToLight + vec3(x, y, z)).r; 
//                closestDepth *= far_plane;   // Undo mapping [0;1]
//                if(currentDepth - bias > closestDepth)
//                    shadow += 1.0;
//            }
//        }
//    }
//    shadow /= (samples * samples * samples);

    float bias = 0.15;
    int samples = 20;
    float viewDistance = length(viewPos - fragPos);
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 25.0; // 距离越远越大
    for(int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(depthMap, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
        closestDepth *= far_plane;
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);


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
    float shadow = shadows ? ShadowCalculation(fs_in.FragPos) : 0.0;                      
    vec3 lighting = (ambient + (1.0-shadow) * (diffuse + specular)) * color;

    FragColor = vec4(lighting, 1.f);

    // Debug
    if(debug) FragColor = vec4(vec3(closestDepth / far_plane), 1.0);
}