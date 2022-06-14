#version 330 core
layout(location=0) in vec3 _Pos;
layout(location=1) in vec3 _Normal;
layout(location=2) in vec2 _TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoords;

void main()
{
    gl_Position = projection * view * model * vec4(_Pos, 1.0f);
    TexCoords = _TexCoords;
}