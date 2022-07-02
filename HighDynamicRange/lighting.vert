#version 460 core
layout(location = 0) in vec3 _Pos;
layout(location = 1) in vec3 _Normal;
layout(location = 2) in vec2 _TexCoords;


out VS_OUT{
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform bool inverse_normals;

void main()
{
    vs_out.FragPos = vec3(model * vec4(_Pos, 1.f));
    vs_out.TexCoords = _TexCoords;

    vec3 normal = inverse_normals ? -_Normal : _Normal;

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vs_out.Normal = normalize(normalMatrix * normal);

    gl_Position = projection * view * model * vec4(_Pos, 1.f);
}