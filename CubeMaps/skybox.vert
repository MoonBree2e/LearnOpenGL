#version 330 core
layout (location = 0) in vec3 _Pos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = _Pos;
    gl_Position = projection * view * vec4(_Pos, 1.0);
    gl_Position = gl_Position.xyww;
}