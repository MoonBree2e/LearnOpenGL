#version 460 core
layout (location = 0) in vec3 _Position;
layout (location = 1) in vec3 _Normal;
layout (location = 2) in vec2 _TexCoords;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    gl_Position = projection * view * model * vec4(_Position, 1.0f);
}