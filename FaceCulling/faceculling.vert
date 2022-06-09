#version 330 core

layout(location=0) in vec3 _Pos;
layout(location=1) in vec2 _TexCoord;

out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    TexCoord = _TexCoord;
    gl_Position = projection * view * model * vec4(_Pos,1.0);
}