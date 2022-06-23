#version 430 core
layout(location=0) in vec3 _Position;

uniform mat4 model;

void main()
{
    gl_Position = model * vec4(_Position, 1.f);
}