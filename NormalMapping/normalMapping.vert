#version 430 core
layout(location=0) in vec3 _Position;
layout(location=1) in vec3 _Normal;
layout(location=2) in vec2 _TexCoords;
layout(location=3) in vec3 _Tangent;
layout(location=4) in vec3 _Bitangent;

out VS_OUT{
    vec3 FragPos;
    vec2 TexCoords;
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{
    gl_Position = projection * view * model * vec4(_Position, 1.f);
    vs_out.FragPos = vec3(model * vec4(_Position, 1.f));
    vs_out.TexCoords = _TexCoords;

    mat3 NormalMatrix = transpose(inverse(mat3(model)));
    vec3 T = normalize(NormalMatrix * _Tangent);
    vec3 B = normalize(NormalMatrix * _Bitangent);
    vec3 N = normalize(NormalMatrix * _Normal);

    mat3 TBN = transpose(mat3(T,B,N));
    vs_out.TangentLightPos = TBN * lightPos;
    vs_out.TangentViewPos = TBN * viewPos;
    vs_out.TangentFragPos = TBN * vs_out.FragPos;
}