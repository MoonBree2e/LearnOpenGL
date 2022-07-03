#version 460 core
layout(location = 0) in vec3 _Pos;
layout(location = 1) in vec2 _TexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = _TexCoords;
    gl_Position = vec4(_Pos, 1.0f);
}