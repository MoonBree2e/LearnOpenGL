#version 330 core
layout (location = 0) in vec2 _Pos;
layout (location = 1) in vec3 _Color;

out VS_OUT {
    vec3 color;
} vs_out;

void main()
{
    gl_Position = vec4(_Pos.x, _Pos.y, 0.0, 1.0); 
    vs_out.color = _Color;
}