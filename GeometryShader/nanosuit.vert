#version 330 core
layout(location=0) in vec3 _Pos;
layout(location=1) in vec3 _Normal;
layout(location=2) in vec2 _TexCoord;

out vec2 TexCoords;

out VS_OUT {
    vec2 TexCoords;
} vs_out;


uniform mat4 projection;
uniform mat4 model;
uniform mat4 view;

void main()
{
    vs_out.TexCoords = _TexCoord;
    TexCoords = _TexCoord;
    gl_Position = projection * view * model * vec4(_Pos,1.f);
}