#version 330 core
layout(location = 0) in vec3 _Pos;
layout(location = 1) in vec3 _Color;
layout(location = 2) in vec2 _TexCoord;

out vec3 ourColor;
out vec2 TexCoord;
uniform mat4 transform;

void main()
{
    gl_Position = transform * vec4(_Pos, 1.0f);
    TexCoord = _TexCoord;
}