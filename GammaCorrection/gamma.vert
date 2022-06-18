#version 330 core
layout(location=0) in vec3 _Pos;
void main()
{
    gl_Position = vec4(_Pos.x, _Pos.y, _Pos.z, 1.0f);
}