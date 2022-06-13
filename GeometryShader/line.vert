#version 330 core

layout(location = 0) in vec2 _Pos;

void main()
{
    gl_Position = vec4(_Pos, 0, 1);
}