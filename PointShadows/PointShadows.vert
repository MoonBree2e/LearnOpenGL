#version 430 core
layout(location=0) in vec3 _Position;
layout(location=1) in vec3 _Normal;
layout(location=2) in vec2 _TexCoords;

out vec2 TexCoords;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    gl_Position = projection * view * model * vec4(_Position, 1.f);
    vs_out.FragPos = vec3(model * vec4(_Position, 1.f));
    vs_out.Normal = transpose(inverse(mat3(model))) * _Normal;
    vs_out.TexCoords = _TexCoords;
}